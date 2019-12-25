#include "bload.h"

static FILE *psf;
static int ncpus, ht;

static struct { unsigned long u; unsigned long s; } prev[MAXCPUS];

void do_cpus()
{
    int cpul[MAXCPUS];
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

        cpul[c] = du*RES/(ds?ds:1);
///        printf("> %u %u %lu\n", c, cpul[c], ds);
    }

    if (ht)
    {
        for (int i=0; i<ncpus; i+=2)
        {
            if (cpul[i]==-1 && cpul[i+1]==-1)
                printf("o");
            else
                write_ht(cpul[i], cpul[i+1]);
        }
    }
    else
    {
        for (int i=0; i<ncpus; i++)
        {
            if (cpul[i]==-1)
                printf("o");
            else
                write_single(cpul[i]);
        }
    }
}

void init_cpus()
{
    set_t set;
    if (read_proc_set("/sys/devices/system/cpu/present", &set))
        die("can't get list of CPUs\n");
    ncpus = set[0].b+1;
    ht = read_proc_int("/sys/devices/system/cpu/smt/active")==1;
    psf = fopen("/proc/stat", "re");
    if (!psf)
        die("fopen(/proc/stat): %m\n");
}
