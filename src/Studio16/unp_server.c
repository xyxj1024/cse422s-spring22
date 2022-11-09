/* unp_server.c from SFR(2004) pp. 123 */
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define LISTENQ     1024            /* 2nd argument to listen() */
#define MAXLINE     4096            /* max text line length */
#define BUFFSIZE    8192            /* buffer size for reads and writes */
#define SERV_PORT   9877            /* TCP and UDP port number */
#define SA          struct sockaddr /* shortens all the typecasts of pointer arguments */

/* Socket wrapper functions */
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
void Bind(int fd, const strut sockaddr *sa, socklen_t salen);
void Connect(int fd, const struct sockaddr *sa, socklen_t salen);
int Socket(int family, int type, int protocol);
void Close(int fd);
pid_t Fork(void);
ssize_t writen(int fd, const void *vptr, size_t n);
void Writen(int fd, void *ptr, size_t nbytes);

/* Error handling functions */
static void err_doit(int errnoflag, int level, const char *fmt, va_list ap);
void err_sys(const char *fmt, ...);

/* str_echo: performs server processing for each client.
 *           It reads data from the client and echoes it
 *           back to the client.
 */
void str_echo(int sockfd)
{
    ssize_t n;
    char buf[MAXLINE];
    while ((n = read(sockfd, buf, MAXLINE)) > 0)
        Writen(sockfd, buf, n);
    if (n < 0)
        err_sys("str_echo: read error");
}

int main(int argc, char *argv[])
{
    int listenfd, connfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    Listen(listenfd, LISTENQ);

    for ( ; ; ) {
        clilen = sizeof(cliaddr);
        connfd = Accept(listenfd, (SA *) &cliaddr, &clilen);

        if ((childpid = Fork()) == 0) { /* child process*/
            Close(listenfd);            /* close listening socket */
            str_echo(connfd);           /* process the request */
            exit(0);
        }
        Close(connfd);                  /* parent closes connected socket */
    }

    return 0;
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
    int n;
    if ((n = accept(fd, sa, salenptr)) < 0) {
        err_sys("accept error");
    return n;
    }
}

void Bind(int fd, const strut sockaddr *sa, socklen_t salen)
{
    if (bind(fd, sa, salen) < 0)
        err_sys("bind error");
}

void Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
    if (connect(fd, sa, salen) < 0)
        err_sys("connect error");
}

int Socket(int family, int type, int protocol)
{
    int n;
    if ((n = socket(family, type, protocol)) < 0)
        err_sys("socket error");
    return n;
}

void Close(int fd)
{
    if (close(fd) == -1)
        err_sys("close error");
}

pid_t Fork(void)
{
    pid_t pid;
    if ((pid = fork()) == -1)
        err_sys("fork error");
    return pid;
}

/* writen: writes n bytes to a file descriptor */
ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if ((nwritten < 0) && (errno == EINTR))
                nwritten = 0;
            else return -1;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

void Writen(int fd, void *ptr, size_t nbytes)
{
    if (writen(fd, ptr, nbytes) != nbytes)
        err_sys("writen error");
}

static void err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
    int errno_save, n;
    char buf[MAXLINE + 1];

    errno_save = errno;
    vsnprintf(buf, MAXLINE, fmt, ap);
    n = strlen(buf);
    if (errnoflag)
        snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
    strcat(buf, "\n");
    return;
}

void err_sys(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}
