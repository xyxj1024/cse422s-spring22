/* Based on "LKD, pp.338-339" */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/errno.h>

#define NOT_FINISHED 0
#define FINISHED 1

/* An array of kernel threads to be spawned: */
static struct task_struct **prime_tasks;
/* Global time stamp variables zero-initialized explicitly: */
static struct timespec prime_init_ts = {0, 0}, prime_first_ts = {0, 0}, prime_second_ts = {0, 0};

/* Keeps track of how many times a thread has "crossed out" a non-prime number: */
static unsigned long *prime_cnt;
/* Range within which primes will be computed: */
atomic_t *prime_val;
/* Current position that is being processed: */
volatile unsigned long prime_pos;
/* Whether or not the computation of prime numbers has finished in each thread: */
atomic_t is_finished;
/* Counter for how many threads still need to arrive: */
volatile unsigned long remaining_threads;
/* Possible states of barrier synchronization: */
static enum barrier_states {
    NOT_REACHED,
    FIRST_REACHED,
    SECOND_REACHED
} bar_state;
/* Statically initialize the spin locks: */
static DEFINE_SPINLOCK(bar_lock);
static DEFINE_SPINLOCK(prime_lock_1);
static DEFINE_SPINLOCK(prime_lock_2);

static unsigned long num_threads = 1;
static unsigned long upper_bound = 10;
module_param(num_threads, ulong, 0644);
module_param(upper_bound, ulong, 0644);

/**
 * The critical section.
 * (a) Safely stores the value of the global position variable `i_pos' in
 *     a local variable, and then advance the global position variable until
 *     it either reaches another non-zero value in the array or exceeds the
 *     last position in the array;
 * (b) If the position in the local variable is greater than the last element
 *     in the array, then the function should simply return; otherwise
 * (c) Safely go to each number in the array that is a larger multiple of
 *     the local position variable, set each of those larger multiples to
 *     zero, and increment the counter `p_cnt'.
 */
static void select_and_mark(unsigned long *cntr)
{
    unsigned long i;         /* Iterator variable */
    unsigned long local_pos; /* Local position variable */
    unsigned long p;         /* Corresponding value */

    while (1) {
        /* 
         * Safely store the value of the global position variable in a
         * local variable and then advance the global position variable:
         */
        spin_lock(&prime_lock_1);
        local_pos = prime_pos;
        prime_pos++;
        if (prime_pos == upper_bound - 1) break;
        while ((prime_pos < upper_bound - 1) && 
               (atomic_read(&prime_val[prime_pos]) == 0)) {
            prime_pos++;
        }
        spin_unlock(&prime_lock_1);

        /* 
         * If the value corresponding to the local position variable
         * is greater than the last element, then the function should
         * simply return:
         */
        if (local_pos >= upper_bound - 1) break;
        else { /* Safely mark larger multiples of the local position variable
                * to zero while incrementing its counter:
                */
            p = local_pos + 2;
            for (i = local_pos + p; i < upper_bound - 1; i += p) {
                spin_lock(&prime_lock_2);
                atomic_set(&prime_val[i], 0);
                spin_unlock(&prime_lock_2);
                (*cntr)++;
            }
        }
    }

    return;
}

/**
 * The barrier must work correctly twice.
 * The k-th thread that makes its first arrival at the barrier
 * must wait for the other n-k threads to arrive.
 * The k-th thread that makes its second arrival at the barrier
 * must wait for the other n-k threads to finish.
 */
static int barrier_spinlock(enum barrier_states state)
{
    if ((state > SECOND_REACHED) || (state < FIRST_REACHED))
    {
        printk(KERN_ALERT "Provided barrier state invalid!\n");
        return -EINVAL;
    }

    spin_lock(&bar_lock);
    remaining_threads--;
    /* All threads either make their first arrival or finish. */
    if (remaining_threads == (SECOND_REACHED - state) * num_threads)
    {
        if (state == FIRST_REACHED)
            ktime_get_ts(&prime_first_ts); /* The last thread has reached the first barrier. */
        else if (state == SECOND_REACHED)
            ktime_get_ts(&prime_second_ts); /* All threads have completed processing. */
    }
    spin_unlock(&bar_lock);
    while (remaining_threads > (SECOND_REACHED - state) * num_threads);

    return 0;
}

/**
 * @cntr - tracks how many non-prime numbers has crossed out.
 * This function run by each spawned thread sequentially:
 * (a) calls a function that performs barrier synchronization
 *     with the other threads,
 * (b) calls a function that repeatedly marks non-prime numbers
 *     in the array until the entire array is processed,
 * (c) calls the barrier function again,
 * (d) updates the atomic variable to indicate that all threads
 *     have finished processing.
 */
static int prime_threadfn(void *cntr)
{
    barrier_spinlock(FIRST_REACHED);
    select_and_mark((unsigned long *)cntr);
    barrier_spinlock(SECOND_REACHED);
    atomic_set(&is_finished, FINISHED);
    return 0;
}

static struct timespec prime_interval(struct timespec *ts1, struct timespec *ts2)
{
    struct timespec interval = {0, 0};
    if ((*ts1).tv_sec > (*ts2).tv_sec)
    {
        printk(KERN_ALERT "Function arguments invalid!\n");
        return *ts2;
    }
    interval.tv_sec = (*ts2).tv_sec - (*ts1).tv_sec;
    interval.tv_nsec = (*ts2).tv_nsec - (*ts1).tv_nsec;
    if (interval.tv_nsec < 0)
    {
        interval.tv_sec--;
        interval.tv_nsec += 1000000000L;
    }
    return interval;
}

static void prime_print(void)
{
    unsigned long i, num_prime = 0, num_marked = 0;
    struct timespec init_ts = {0, 0}, prime_ts = {0, 0}, total_ts = {0, 0};

    printk(KERN_INFO "There are %lu threads and the largest integer being processed is %lu.\n", num_threads, upper_bound);

    for (i = 0; i < upper_bound - 1; i++)
    {
        if (atomic_read(&prime_val[i]) != 0)
        {
            //printk(KERN_CONT "%d ", atomic_read(&prime_val[i]));
            num_prime++;
            //if (num_prime % 10 == 0)
                //printk(KERN_CONT "\n");
        }
    }
    printk(KERN_INFO "There are %lu primes and %lu non-primes within [2, %lu].\n", num_prime, (upper_bound - num_prime - 1), upper_bound);

    for (i = 0; i < num_threads; i++)
    {
        num_marked += prime_cnt[i];
    }
    printk(KERN_INFO "There are %lu unnecessary crossing out.\n", num_marked - (upper_bound - num_prime - 1));

    init_ts = prime_interval(&prime_init_ts, &prime_first_ts);
    prime_ts = prime_interval(&prime_first_ts, &prime_second_ts);
    total_ts = prime_interval(&prime_init_ts, &prime_second_ts);
    printk(KERN_INFO "Time spent on setting up the module: %09ld.%09ld seconds.\n", init_ts.tv_sec, init_ts.tv_nsec);
    printk(KERN_INFO "Time spent on prime computation: %09ld.%09ld seconds.\n", prime_ts.tv_sec, prime_ts.tv_nsec);
    printk(KERN_INFO "Total time spent: %09ld.%09ld seconds.\n", total_ts.tv_sec, total_ts.tv_nsec);
}

static int prime_init(void)
{
    unsigned long i;

    /* Initialization time-stamped before doing anything else: */
    ktime_get_ts(&prime_init_ts);

    prime_val = NULL;
    prime_cnt = NULL;
    prime_tasks = NULL;
    prime_pos = 0;
    remaining_threads = num_threads * SECOND_REACHED;
    bar_state = NOT_REACHED;
    atomic_set(&is_finished, FINISHED);

    printk(KERN_ALERT "Lab 2: Kernel Module Concurrent Memory Use\n");

    if ((num_threads < 1) || (upper_bound < 2))
    {
        printk(KERN_ALERT "User-specified module parameter invalid!\n");
        if (num_threads < 1)
        {
            printk(KERN_ALERT "num_threads must be greater than or equal to 1!\n");
        }
        else
            printk(KERN_ALERT "upper_bound must be greater than or equal to 2!\n");
        num_threads = 0;
        upper_bound = 0;
        return -EINVAL;
    }

    prime_val = (atomic_t *)kmalloc((upper_bound - 1) * sizeof(atomic_t), GFP_KERNEL);
    if (prime_val == NULL)
    {
        printk(KERN_ALERT "kmalloc for prime_val failed!\n");
        prime_val = NULL;
        return -ENOMEM;
    }

    prime_cnt = (unsigned long *)kmalloc(num_threads * sizeof(unsigned long), GFP_KERNEL);
    if (prime_cnt == NULL)
    {
        printk(KERN_ALERT "kmalloc for prime_cnt failed!\n");
        kfree(prime_val);
        prime_val = NULL;
        prime_cnt = NULL;
        return -ENOMEM;
    }

    for (i = 0; i < upper_bound - 1; i++)
    {
        atomic_set(&prime_val[i], i + 2);
    }

    for (i = 0; i < num_threads; i++)
    {
        prime_cnt[i] = 0;
    }

    prime_tasks = (struct task_struct **)kmalloc(num_threads * sizeof(struct task_struct *), GFP_KERNEL);
    if (prime_tasks == NULL)
    {
        printk(KERN_ALERT "kmalloc for prime_tasks failed!\n");
        kfree(prime_val);
        kfree(prime_cnt);
        prime_val = NULL;
        prime_cnt = NULL;
        return -ENOMEM;
    }

    atomic_set(&is_finished, NOT_FINISHED);
    for (i = 0; i < num_threads; i++)
    {
        prime_tasks[i] = kthread_run(prime_threadfn, (void *)&prime_cnt[i], "kernel thread [%lu]", i);
        if (IS_ERR(prime_tasks[i]))
        {
            printk(KERN_ERR "The %lu-th kernel thread cannot be spawned!\n", i);
            return -EPERM;
        }
    }

    return 0;
}

static void prime_exit(void)
{
    if (atomic_read(&is_finished) != FINISHED)
    {
        printk(KERN_ALERT "Processing not completed!\n");
        return;
    }

    prime_print();

    /* Clean up the pointers after use: */
    kfree(prime_val);
    kfree(prime_cnt);
    kfree(prime_tasks);
    prime_val = NULL;
    prime_cnt = NULL;
    prime_tasks = NULL;

    printk(KERN_ALERT "Out, out, brief candle!\n");
    return;
}

module_init(prime_init);
module_exit(prime_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("x.xingjian");
MODULE_DESCRIPTION("CSE 422S Lab 2");
