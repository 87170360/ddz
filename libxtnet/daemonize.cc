#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include "daemonize.h"

static int lock_file(int fd);
///---守护进程
int daemonize()
{
    pid_t pid;
    int fd = -1;
    
    /* already a daemon */
    if (getppid() == 1)
        exit(1);
    
    pid = fork();
    if (pid < 0)
        exit(1);
    else if (pid != 0)
        exit(0);
    
    /* Cancel certain signals */
    signal(SIGCHLD, SIG_DFL); /* A child process dies */
    signal(SIGTSTP, SIG_IGN); /* Various TTY signals */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGHUP, SIG_IGN); /* Ignore hangup signal */
    signal(SIGTERM, SIG_DFL); /* Die on SIGTERM */
    signal(SIGPIPE, SIG_IGN);
    
    /* become session leader */
    if (setsid() < 0)
        exit(1);
    
    /* change working directory */
    //if (chdir("/") < 0)
    //    exit(1);
    
    /* clear our file mode creation mask */
    umask(0);
    
    for (fd = getdtablesize(); fd >= 0; fd--)
        close(fd);

    /* handle standart I/O */
    fd = open("/dev/null", O_RDWR);
    fd = dup(0);
    fd = dup(0);
    
    return 0;
}
///---对文件加锁，只允许一个进程操作文件
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
int single_instance_running(const char* pid_file)
{
    int     fd;
    char    buf[16];
    
    fd = open(pid_file, O_RDWR | O_CREAT, LOCKMODE);
    if (fd < 0) {
        return -1;
    }
	
    if (lock_file(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            return -1;
        }
        close(fd);
        return -1;
    }
    ftruncate(fd, 0);
    snprintf(buf, sizeof(buf), "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
    
    return 0;
}
///---文件加锁
static int lock_file(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return (fcntl(fd, F_SETLK, &fl));
}
