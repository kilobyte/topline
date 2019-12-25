#include "bload.h"

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
        if (x>=MAXCPUS)
            return -1;
        switch (*bp)
        {
        case ',':
            bp++;
        case 0:
            set[setl]->a=set[setl]->b=x;
            setl++;
            if (x > set_max)
                set_max=x;
            break;
        case '-':;
            long y = strtol(bp+1, &bp, 10);
            if (y>=MAXCPUS)
                return -1;
            set[setl]->a=x;
            set[setl]->b=y;
            setl++;
            if (y > set_max)
                set_max=y;
            break;
        default:
            return -1;
        }
    } while (*bp && *bp!='\n');
    close(fd);
    set[0]->a=setl;
    set[0]->b=set_max;
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
    printf("%s", single[step(x, 8)]);
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
    printf("\xe2%c%c", (ch>>6)+0xA0, (ch&0x3F)|0x80);
}

static void do_line()
{
    do_disks();
    do_cpus();
    printf("\n");
}

int main(int argc, char **argv)
{
    init_cpus();
    init_disks();

    do_line();
    while (1)
    {
        sleep(1);
        do_line();
    }
    return 0;
}
