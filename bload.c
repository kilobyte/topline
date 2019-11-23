#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#define die(...) do {fprintf(stderr, __VA_ARGS__); exit(1);} while(0)
#define ARRAYSZ(x) (sizeof(x)/sizeof(x[0]))

#define MAXCPUS 4096
static FILE *psf;
static int ncpus;

static int setl, set_max;
static struct { int a; int b; } set[256];

/*
static int read_proc_int(const char *path)
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
*/

static int read_proc_set(const char *path)
{
    setl=0;
    set_max=-1;

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
        if (setl >= ARRAYSZ(set))
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
            set[setl].a=set[setl].b=x;
            setl++;
            if (x > set_max)
                set_max=x;
            break;
        case '-':;
            long y = strtol(bp+1, &bp, 10);
            if (y>=MAXCPUS)
                return -1;
            set[setl].a=x;
            set[setl].b=y;
            setl++;
            if (y > set_max)
                set_max=y;
            break;
        default:
            return -1;
        }
    } while (*bp && *bp!='\n');
    close(fd);
    return 0;
}

static const char *single[9] = {"_", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
static const uint8_t bx[5] = {0, 0x40, 0x44, 0x46, 0x47};
static const uint8_t by[5] = {0, 0x80, 0xA0, 0xB0, 0xB8};

static void write_ht(int x, int y)
{
    if (!x && !y)
        return (void)printf("_");
    uint8_t ch = bx[x] + by[y];
    printf("\xe2%c%c", (ch>>6)+0xA0, (ch&0x3F)|0x80);
}

static struct { unsigned long u; unsigned long s; } prev[MAXCPUS];

static void do_line()
{
    unsigned int cpul[MAXCPUS];
    for (int i=0; i<ncpus; i++)
        cpul[i]=-1;

    char buf[4096];
    rewind(psf);
    if (!fgets(buf, sizeof(buf), psf))
        die("fgets(/proc/stat): %m\n");
    unsigned long dummy;
    if (sscanf(buf, "cpu %lu ", &dummy)!=1)
        die("bad format of /proc/stat\n");
    while (1)
    {
        if (!fgets(buf, sizeof(buf), psf))
            die("fgets(/proc/stat): %m\n");
        unsigned int c;
        unsigned long t[10];
        if (sscanf(buf, "cpu%u %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                   &c, &t[0], &t[1], &t[2], &t[3], &t[4],
                   &t[5], &t[6], &t[7], &t[8], &t[9])!=11)
            break;
        if (c >= MAXCPUS)
            continue;

        unsigned long sum = 0;
        for (int i=0; i<ARRAYSZ(t); i++)
            sum += t[i];
        if (!sum)
            continue;

        unsigned long use = sum-t[3]-t[4]-t[7]; // idle+iowait+steal
        unsigned long du = use - prev[c].u;
        unsigned long ds = sum - prev[c].s;
        prev[c].u = use;
        prev[c].s = sum;
        if (du>ds)
            die("overflow\n");

#define RES 64
        cpul[c] = du*RES/(ds?ds:1);
///        printf("> %u %u %lu\n", c, cpul[c], ds);
    }

    if (1)
    {
        unsigned int ml = 4;
        for (int i=0; i<ncpus; i+=2)
        {
            if (cpul[i]==(unsigned)-1)
                printf("o");
            else if (!cpul[i])
                printf("_");
            else
                write_ht((cpul[i]*ml+RES/2)/RES, (cpul[i+1]*ml+RES/2)/RES);
        }
    }
    else
    {
        unsigned int ml = 8;
        for (int i=0; i<ncpus; i++)
        {
            if (cpul[i]==(unsigned)-1)
                printf("o");
            else if (!cpul[i])
                printf("_");
            else
                printf("%s", single[(cpul[i]*ml+RES/2)/RES]);
        }
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    if (read_proc_set("/sys/devices/system/cpu/present"))
        die("can't get list of CPUs\n");
    ncpus = set_max+1;
    psf = fopen("/proc/stat", "re");
    if (!psf)
        die("fopen(/proc/stat): %m\n");
    do_line();
    while (1)
    {
        sleep(1);
        do_line();
    }
    fclose(psf);
    return 0;
}
