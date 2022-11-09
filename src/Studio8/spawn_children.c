/* spawn_children.c modified from LPI pp. 526 */
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>

/* This macro stops 'gcc -Wall' complaining that "control reaches
 * end of non-void function" if we use the following functions to
 * terminate main() or some other non-void function.
 */
#ifdef __GNUC__
#define NORETURN __attribute__ ((__noreturn__))
#else
#define NORETURN
#endif

/* Built on GNU/Linux x86_64 with glibc 2.34 */
static char *ename[] = {
    /*   0 */ "", 
    /*   1 */ "EPERM", "ENOENT", "ESRCH", "EINTR", "EIO", "ENXIO", 
    /*   7 */ "E2BIG", "ENOEXEC", "EBADF", "ECHILD", 
    /*  11 */ "EAGAIN/EWOULDBLOCK", "ENOMEM", "EACCES", "EFAULT", 
    /*  15 */ "ENOTBLK", "EBUSY", "EEXIST", "EXDEV", "ENODEV", 
    /*  20 */ "ENOTDIR", "EISDIR", "EINVAL", "ENFILE", "EMFILE", 
    /*  25 */ "ENOTTY", "ETXTBSY", "EFBIG", "ENOSPC", "ESPIPE", 
    /*  30 */ "EROFS", "EMLINK", "EPIPE", "EDOM", "ERANGE", 
    /*  35 */ "EDEADLK/EDEADLOCK", "ENAMETOOLONG", "ENOLCK", "ENOSYS", 
    /*  39 */ "ENOTEMPTY", "ELOOP", "", "ENOMSG", "EIDRM", "ECHRNG", 
    /*  45 */ "EL2NSYNC", "EL3HLT", "EL3RST", "ELNRNG", "EUNATCH", 
    /*  50 */ "ENOCSI", "EL2HLT", "EBADE", "EBADR", "EXFULL", "ENOANO", 
    /*  56 */ "EBADRQC", "EBADSLT", "", "EBFONT", "ENOSTR", "ENODATA", 
    /*  62 */ "ETIME", "ENOSR", "ENONET", "ENOPKG", "EREMOTE", 
    /*  67 */ "ENOLINK", "EADV", "ESRMNT", "ECOMM", "EPROTO", 
    /*  72 */ "EMULTIHOP", "EDOTDOT", "EBADMSG", "EOVERFLOW", 
    /*  76 */ "ENOTUNIQ", "EBADFD", "EREMCHG", "ELIBACC", "ELIBBAD", 
    /*  81 */ "ELIBSCN", "ELIBMAX", "ELIBEXEC", "EILSEQ", "ERESTART", 
    /*  86 */ "ESTRPIPE", "EUSERS", "ENOTSOCK", "EDESTADDRREQ", 
    /*  90 */ "EMSGSIZE", "EPROTOTYPE", "ENOPROTOOPT", 
    /*  93 */ "EPROTONOSUPPORT", "ESOCKTNOSUPPORT", 
    /*  95 */ "EOPNOTSUPP/ENOTSUP", "EPFNOSUPPORT", "EAFNOSUPPORT", 
    /*  98 */ "EADDRINUSE", "EADDRNOTAVAIL", "ENETDOWN", "ENETUNREACH", 
    /* 102 */ "ENETRESET", "ECONNABORTED", "ECONNRESET", "ENOBUFS", 
    /* 106 */ "EISCONN", "ENOTCONN", "ESHUTDOWN", "ETOOMANYREFS", 
    /* 110 */ "ETIMEDOUT", "ECONNREFUSED", "EHOSTDOWN", "EHOSTUNREACH", 
    /* 114 */ "EALREADY", "EINPROGRESS", "ESTALE", "EUCLEAN", 
    /* 118 */ "ENOTNAM", "ENAVAIL", "EISNAM", "EREMOTEIO", "EDQUOT", 
    /* 123 */ "ENOMEDIUM", "EMEDIUMTYPE", "ECANCELED", "ENOKEY", 
    /* 127 */ "EKEYEXPIRED", "EKEYREVOKED", "EKEYREJECTED", 
    /* 130 */ "EOWNERDEAD", "ENOTRECOVERABLE", "ERFKILL", "EHWPOISON"
};

#define MAX_ENAME   133

#define GN_NONNEG   01      /* value must be >= 0 */
#define GN_GT_0     02      /* value must be > 0 */
                            /* by default, integers are decimal */
#define GN_ANY_BASE 0100    /* can use any base - like strtol(3) */
#define GN_BASE_8   0200    /* value is expressed in octal */
#define GN_BASE_16  0400    /* value is expressed in hexadecimal */

#define BUF_SIZE    500

typedef enum { FALSE, TRUE } Boolean;

static void gnFail(const char *fname, const char *msg, const char *arg, const char *name);
static long getNum(const char *fname, const char *arg, int flags, const char *name);
int getInt(const char *arg, int flags, const char *name);

/* Error handling functions */
void usageErr(const char *format, ...);
NORETURN static void terminate(Boolean useExit3);
static void outputError(Boolean useErr, int err, Boolean flushStdout, const char *format, va_list ap);
void errExit(const char *format, ...);

int main(int argc, char *argv[])
{
    int numChildren, j;
    pid_t childPid;
    double time_spent = 0.0;
    clock_t begin, end;

    if ((argc > 1) && (strcmp(argv[1], "--help") == 0))
        usageErr("./%s <numChildren>\n", argv[0]);
    
    numChildren = (argc > 1) ? getInt(argv[1], GN_GT_0, "num-children") : 1;

    setbuf(stdout, NULL);   /* make stdout unbuffered */

    begin = clock();

    for (j = 0; j < numChildren; j++) {
        switch (childPid = fork()) {
        case -1:
            errExit("fork");
        case 0:
            printf("%d child\n", j);
            _exit(EXIT_SUCCESS);
        default:
            printf("%d parent\n", j);
            wait(NULL);     /* wait for children to terminate */
            break;
        }
    }

    end = clock();

    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    printf("The elapsed time is %.9f seconds.\n", time_spent);

    exit(EXIT_SUCCESS);
}

/* gnFail: prints a diagnostic message that contains a function name ('fname'),
 *         the value of a command-line argument ('arg'), the name of that
 *         command-line argument ('name'), and a diagnostic error message ('msg').
 */
static void gnFail(const char *fname, const char *msg, const char *arg, const char *name)
{
    fprintf(stderr, "%s error", fname);
    if (name != NULL)
        fprintf(stderr, " (in %s)", name);
    fprintf(stderr, ": %s\n", msg);
    if (arg != NULL && *arg != '\0')
        fprintf(stderr, "        offending text: %s\n", arg);

    exit(EXIT_FAILURE);
}

/* getNum: converts a numeric command-line argument ('arg') into a long integer,
 *         returned as the function result. 'flags' is a bit mask of flags controlling
 *         how the conversion is done and what diagnostic checks are performed on the
 *         numeric result; see get_num.h for details.
 */
static long getNum(const char *fname, const char *arg, int flags, const char *name)
{
    long res;
    char *endptr;
    int base;

    if (arg == NULL || *arg == '\0')
        gnFail(fname, "null or empty string", arg, name);

    base = (flags & GN_ANY_BASE) ? 0 : (flags & GN_BASE_8) ? 8 :
                        (flags & GN_BASE_16) ? 16 : 10;

    errno = 0;
    res = strtol(arg, &endptr, base);
    if (errno != 0)
        gnFail(fname, "strtol() failed", arg, name);

    if (*endptr != '\0')
        gnFail(fname, "nonnumeric characters", arg, name);

    if ((flags & GN_NONNEG) && res < 0)
        gnFail(fname, "negative value not allowed", arg, name);

    if ((flags & GN_GT_0) && res <= 0)
        gnFail(fname, "value must be > 0", arg, name);

    return res;
}

/* getInt: converts a numeric command-line argument string to an integer. */
int getInt(const char *arg, int flags, const char *name)
{
    long res;

    res = getNum("getInt", arg, flags, name);

    if (res > INT_MAX || res < INT_MIN)
        gnFail("getInt", "integer out of range", arg, name);

    return res;
}

/* usageErr: prints a command usage error message and terminate the process. */
void usageErr(const char *format, ...)
{
    va_list argList;

    fflush(stdout); /* flush any pending stdout */

    fprintf(stderr, "Usage: ");
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);

    fflush(stderr); /* in case stderr is not line-buffered */
    exit(EXIT_FAILURE);
}

/* terminate: */
NORETURN static void terminate(Boolean useExit3)
{
    char *s;

    /* Dump core if EF_DUMPCORE environment variable is defined and 
     * is a nonempty string; otherwise call exit() or _exit(),
     * depending on the value of 'useExit3'.
     */
    s = getenv("EF_DUMPCORE");

    if ((s != NULL) && (*s != '\0'))
        abort();
    else if (useExit3)
        exit(EXIT_FAILURE);
    else
        _exit(EXIT_FAILURE);
}

/* outputError: diagnoses 'errno' error by:
 *              1. outputting a string containing the error name
 *                 (if available in 'ename' array) corresponding
 *                 to the value in 'err', along with the corresponding
 *                 error message from strerror(), and
 *              2. outputting the caller-supplied error message
 *                 specified in 'format' and 'ap'.
 */
static void outputError(Boolean useErr, int err, Boolean flushStdout, const char *format, va_list ap)
{
    char buf[BUF_SIZE], userMsg[BUF_SIZE], errText[BUF_SIZE];

    vsnprintf(userMsg, BUF_SIZE, format, ap);

    if (useErr)
        snprintf(errText, BUF_SIZE, " [%s %s]", (err > 0 && err <= MAX_ENAME) ? ename[err] : "?UNKNOWN?", strerror(err));
    else
        snprintf(errText, BUF_SIZE, ":");
    
    snprintf(buf, BUF_SIZE, "ERROR%s %s\n", errText, userMsg);

    if (flushStdout)
        fflush(stdout); /* Flush any pending stdout */
    fputs(buf, stderr);
    fflush(stderr);     /* In case stderr is not line-buffered */
}

/* errExit: displays error message including 'errno' diagnostic,
            and terminate the process.
 */
void errExit(const char *format, ...)
{
    va_list argList;

    va_start(argList, format);
    outputError(TRUE, errno, TRUE, format, argList);
    va_end(argList);

    terminate(TRUE);
}
