#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include "topline.h"

FILE* log_output;

int read_proc_int(const char *path)
{
    int fd = open(path, O_RDONLY|O_CLOEXEC);
    if (fd==-1)
        return -1;
    char buf[32];
    int r = read(fd, buf, sizeof(buf));
    if (r<=0 || r>=sizeof(buf))
        return close(fd), -1;
    buf[r]=0;
    int x = atoi(buf);
    close(fd);
    return x;
}

int read_proc_set(const char *path, set_t *set)
{
    int setl=1;
    int set_max=-1;

    int fd = open(path, O_RDONLY|O_CLOEXEC);
    if (fd==-1)
        return -1;
    char buf[16384], *bp=buf;
    int r = read(fd, buf, sizeof(buf));
    if (r<=0 || r>=sizeof(buf))
        return close(fd), -1;
    close(fd);
    buf[r]=0;

    do
    {
        if (setl >= ARRAYSZ(*set))
            return -1;
        if (*bp<'0' || *bp>'9')
            return -1;
        long x = strtol(bp, &bp, 10);
        if (x<0 || x>=MAXCPUS)
            return -1;
        switch (*bp)
        {
        case ',':
            bp++;
        case 0:
        case '\n':
            (*set)[setl].a=(*set)[setl].b=x;
            setl++;
            if (x > set_max)
                set_max=x;
            break;
        case '-':;
            long y = strtol(bp+1, &bp, 10);
            if (*bp==',')
                bp++;
            if (y<0 || y>=MAXCPUS)
                return -1;
            (*set)[setl].a=x;
            (*set)[setl].b=y;
            setl++;
            if (y > set_max)
                set_max=y;
            break;
        default:
            return -1;
        }
    } while (*bp && *bp!='\n');
    close(fd);
    SET_CNT(*set)=setl;
    SET_MAX(*set)=set_max;
    return 0;
}

static const char *single[9] = {" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
static const uint8_t bx[5] = {0, 0x40, 0x44, 0x46, 0x47};
static const uint8_t by[5] = {0, 0x80, 0xA0, 0xB0, 0xB8};

static inline int step(int x, int ml)
{
    if (x>=RES || x<0)
        return ml;

#define SMIN 5
#define SMAX 900
    if (x<SMIN)
        return 0;
    if (x>=SMAX)
        return ml;

    return (x-SMIN)*(ml-1)/(SMAX-SMIN)+1;
}

void write_single(int x)
{
    fprintf(log_output, "%s", single[step(x, 8)]);
}

void write_dual(int x, int y)
{
    if (x>RES)
        x=RES;
    if (y>RES)
        y=RES;
    x = step(x, 4);
    y = step(y, 4);
    uint8_t ch = bx[x] + by[y];
    fprintf(log_output, "\xe2%c%c", (ch>>6)+0xA0, (ch&0x3F)|0x80);
}

static void do_line(int quiet)
{
    FILE *out;
    if (quiet)
    {
        out = log_output;
        if (!(log_output = fopen("/dev/null", "w")))
            die("Can't open /dev/null: %m\n");
    }

    do_disks();
    do_cpus();

    if (quiet)
    {
        fclose(log_output);
        log_output = out;
        return;
    }

    fprintf(log_output, "\n");
    fflush(log_output);
}

static struct linebuf
{
    int fd;
    int len;
    FILE *destf;
    char buf[1024];
} linebuf[2];

static void copy_line(struct linebuf *restrict lb)
{
    int len = lb->len;
    int r = read(lb->fd, lb->buf+len, sizeof(lb->buf)-len);
    if (r==-1)
        die("read: %m\n");
    else if (!r)
        return (void)(lb->fd=0);

    len+=r;
    char *start=lb->buf, *nl;
    while ((nl=memchr(start, '\n', len)))
    {
        nl++;
        fwrite(start, 1, nl-start, lb->destf);
        len-= nl-start;
        start=nl;
    }
    if (len >= sizeof(lb->buf)/2)
    {
        // break the overlong line
        fwrite(start, 1, len, lb->destf);
        fputc('\n', lb->destf);
        lb->len=0;
    }
    else
    {
        memmove(lb->buf, start, len);
        lb->len=len;
    }
}

static volatile int done;

static void sigchld(__attribute__((unused)) int dummy)
{
    done = 1;
}

static int out_lines;
static int child_pid;
static struct timeval interval={1,0};

static void do_args(char **argv)
{
    argv++;
    while (*argv && **argv=='-')
    {
        if (!strcmp(*argv, "-l")
            || !strcmp(*argv, "--line-output")
            || !strcmp(*argv, "--linearize"))
        {
            out_lines=1;
            argv++;
            continue;
        }

        if (!strncmp(*argv, "-i", 2)
            || !strcmp(*argv, "--interval"))
        {
            char *arg = (*argv)[1]=='i' && (*argv)[2] ?
                *argv+2 : *++argv;
            if (!arg || !*arg)
                die("Missing argument to -i\n");

            char *rest;
            double in = strtod(arg, &rest);
            if (arg == rest)
                die("Invalid argument to -i ｢%s｣\n", arg);
            if (*rest)
            {
                if (!strcmp(rest, "s"))
                    ;
                else if (!strcmp(rest, "m"))
                    in*=60;
                else if (!strcmp(rest, "h"))
                    in*=60*60;
                else if (!strcmp(rest, "d"))
                    in*=60*60*24;
                else if (!strcmp(rest, "w"))
                    in*=60*60*24*7;
                else if (!strcmp(rest, "ms"))
                    in/=1000;
                else if (!strcmp(rest, "us") || !strcmp(rest, "µs") || !strcmp(rest, "μs"))
                    in/=1000000;
                else
                    die("Invalid suffix to -i ｢%s｣ in ｢%s｣\n", rest, arg);
            }

            int64_t i = in*1000000;
            if (i<=0)
                die("Interval in -i must be positive.\n");

            interval.tv_sec = i/1000000;
            interval.tv_usec = i%1000000;

            argv++;
            continue;
        }

        if (!strncmp(*argv, "-o", 2) || !strcmp(*argv, "--output"))
        {
            char *arg = (*argv)[1]=='o' && (*argv)[2] ?
                *argv+2 : *++argv;
            if (!arg || !*arg)
                die("Missing argument to -o\n");

            FILE *f = fopen(arg, "we");
            if (!f)
                die("Can't write to ｢%s｣: %m\n", arg);
            log_output = f;

            argv++;
            continue;
        }

        if (!strcmp(*argv, "--"))
            break;

        die("Unknown option: '%s'\n", *argv);
    }

    if (out_lines && !*argv)
        die("-l given but no program to run.\n");
    // -l and -o together are of little use, but as programs behave differently
    // when piped, not outright useless.

    if (*argv)
    {
        int s[2], e[2];
        if (out_lines && (pipe2(s, O_CLOEXEC) || pipe2(e, O_CLOEXEC)))
            die("pipe2: %m\n");
        if ((child_pid=fork()) < 0)
            die("fork: %m\n");
        if (!child_pid)
        {
            if (out_lines && (dup2(s[1], 1)==-1 || dup2(e[1], 2)==-1))
                die("dup2: %m\n");
            execvp(*argv, argv);
            die("Couldn't run ｢%s｣: %m\n", *argv);
        }

        if (out_lines)
        {
            close(s[1]);
            close(e[1]);
            linebuf[0].fd = s[0]; linebuf[0].destf=stdout;
            linebuf[1].fd = e[0]; linebuf[1].destf=stderr;
        }
    }
}

int main(int argc, char **argv)
{
    log_output = stdout;
    init_cpus();
    init_disks();

    signal(SIGCHLD, sigchld);
    do_args(argv);

    do_line(1);

    struct timeval delay=interval;
    while (!done)
    {
        if (!delay.tv_sec && !delay.tv_usec)
        {
            do_line(0);
            delay = interval;
        }

        long fds=0;
#define NFDS (sizeof(fds)*8)
        for (int i=0; i<ARRAYSZ(linebuf); i++)
            if (linebuf[i].fd && linebuf[i].fd<NFDS)
                fds|=1L<<linebuf[i].fd;
        if (select(fds?NFDS:0, (void*)&fds, 0, 0, &delay)==-1)
        {
            if (errno!=EINTR)
                die("select: %m\n");
        }
        else for (int i=0; i<ARRAYSZ(linebuf); i++)
            if (linebuf[i].fd && linebuf[i].fd<sizeof(fds)*8 && fds&1<<linebuf[i].fd)
                copy_line(&linebuf[i]);
    }

    if (child_pid)
    {
        int ret;
        if (waitpid(child_pid, &ret, 0) == child_pid)
        {
            if (WIFSIGNALED(ret))
                sigobit(ret);
            return WIFEXITED(ret)? WEXITSTATUS(ret) : WTERMSIG(ret)+128;
        }
    }
    return 0;
}
