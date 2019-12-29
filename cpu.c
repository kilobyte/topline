#include <string.h>
#include "topline.h"

static FILE *psf;
static int ncpus, ht;
static int cpuorder[MAXCPUS], cpunodes[MAXCPUS];
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

    int lastnode=-1;
    fprintf(log_output, "(");

    if (ht)
    {
        for (int i=0; i<ncpus; i+=2)
        {
            int a=cpuorder[i];
            int b=cpuorder[i+1];
            int n=cpunodes[a]; // inexact with odd HT, but meh
            if (n!=lastnode)
            {
                if (lastnode!=-1)
                    fprintf(log_output, "≬");
                lastnode=n;
            }

            if (cpul[a]==-1 && cpul[b]==-1)
                fprintf(log_output, "o");
            else
                write_dual(cpul[a], cpul[b]);
        }
    }
    else
    {
        for (int i=0; i<ncpus; i++)
        {
            int a=cpuorder[i];
            int n=cpunodes[a];
            if (n!=lastnode)
            {
                if (lastnode!=-1)
                    fprintf(log_output, "≬");
                lastnode=n;
            }

            if (cpul[a]==-1)
                fprintf(log_output, "o");
            else
                write_single(cpul[a]);
        }
    }

    fprintf(log_output, ")");
}

void init_cpus()
{
    set_t set;
    if (read_proc_set("/sys/devices/system/cpu/present", &set))
        die("can't get list of CPUs\n");
    ncpus = SET_MAX(set)+1;

    memset(cpunodes, -1, sizeof(cpunodes));
    uint8_t taken[MAXCPUS];
    memset(taken, 0, sizeof(taken));
    int ntaken=0;

    int max_node;
    if (read_proc_set("/sys/devices/system/node/has_cpu", &set))
        max_node=-1;
    else
    {
        max_node=SET_MAX(set);
        for (int n=0; n<=max_node; n++)
        {
            char path[64];
            sprintf(path, "/sys/devices/system/node/node%d/cpulist", n);
            if (!read_proc_set(path, &set))
            {
                SET_ITER(i, set)
                    cpunodes[i]=n;
                SET_ITER(i, set)
                {
                    if (taken[i])
                        continue;
                    sprintf(path, "/sys/devices/system/cpu/cpu%d/topology/thread_siblings_list", i);
                    set_t sib;
                    if (!read_proc_set(path, &sib))
                        SET_ITER(j, sib)
                            if (!taken[j])
                            {
                                taken[j]=1;
                                cpuorder[ntaken++]=j;
                            }
                }
            }
        }
    }

    for (int i=0; i<MAXCPUS; i++)
        if (!taken[i])
            cpuorder[ntaken++]=i;

    ht = read_proc_int("/sys/devices/system/cpu/smt/active")==1;
    psf = fopen("/proc/stat", "re");
    if (!psf)
        die("fopen(/proc/stat): %m\n");
}
