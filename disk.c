#include <string.h>
#include <sys/time.h>
#include "topline.h"

#define NMAJORS 512
typedef struct
{
    unsigned long rd;
    unsigned long wr;
    const char *name;
    int part;
} bdevstat_t;

static bdevstat_t *bdev[NMAJORS];
static int bdevn[NMAJORS];
static struct timeval t0;

static FILE *ds;

void init_disks()
{
    ds = fopen("/proc/diskstats", "re");
    if (!ds)
        die("Can't open /proc/diskstats: %m\n");
}

static const char *bdprefs[] =
{
    "nvme",
    "sd",
    "hd",
    "mmcblk",
    "loop",
    "nbd",
    "sr",
    // pmem is usually dax, which doesn't update stats here
    0
};

static const char *get_name(const char *inst)
{
    for (const char **pref=bdprefs; *pref; pref++)
        if (!strncmp(inst, *pref, strlen(*pref)))
            return *pref;
    return strdup(inst);
}

void do_disks()
{
    char buf[4096];

    rewind(ds);

    struct timeval t1;
    gettimeofday(&t1, 0);
    unsigned int td = (t1.tv_sec-t0.tv_sec)*1000000+t1.tv_usec-t0.tv_usec;
    if (!td)
        td = 1;
    t0 = t1;

    unsigned int prev_major = -1;

    while (fgets(buf, sizeof(buf), ds))
    {
        char namebuf[64];
        unsigned int major, minor;
        unsigned long rd, wr;

        if (sscanf(buf, "%u %u %63s %*u %*u %*u %lu %*u %*u %*u %lu",
            &major, &minor, namebuf, &rd, &wr) != 5)
        {
            die("A line of /proc/diskstats is corrupted: “%s”\n", buf);
        }

        if (major>=NMAJORS || minor>NMAJORS)
            die("Invalid major:minor : %u:%u\n", major, minor);

        if (!rd && !wr)
            continue;

        if (bdevn[major] <= minor)
        {
            bdevstat_t *newmem = realloc(bdev[major], sizeof(bdevstat_t)*(minor+1));
            if (!newmem)
                die("realloc failed: %m\n");
            memset(newmem+bdevn[major], 0, sizeof(bdevstat_t)*(minor+1-bdevn[major]));
            bdev[major] = newmem;
            bdevn[major] = minor+1;
        }

        bdevstat_t *bs = &bdev[major][minor];
        if (!bs->name)
        {
            bs->name=get_name(namebuf);
            sprintf(namebuf, "/sys/dev/block/%u:%u/partition", major, minor);
            if (!access(namebuf, F_OK))
            {
                bs->part = 1;
                continue;
            }
            bs->part = 0;
        }
        else if (bs->part)
            continue;

        int r = ((int64_t)rd-bs->rd)*RES*1000/td;
        int w = ((int64_t)wr-bs->wr)*RES*1000/td;
        bs->rd = rd;
        bs->wr = wr;

        if (prev_major != major)
        {
            fprintf(log_output, prev_major==-1 ? "%s(" : ")%s(", bs->name);
            prev_major = major;
        }
        write_dual(r, w);
    }
    if (prev_major!=-1)
        fprintf(log_output, ") ");
}
