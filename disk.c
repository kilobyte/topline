#include <string.h>
#include <time.h>
#include "topline.h"

typedef struct
{
    unsigned long rd;
    unsigned long wr;
    const char *name;
    int minor;
    short major;
    char part;
} bdevstat_t;

static bdevstat_t bdev[4096];
static struct timespec t0;

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
    "fd",
    "md",
    "dm",
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

    struct timespec t1;
    if (clock_gettime(CLOCK_MONOTONIC, &t1))
        die("Broken clock: %m\n");
    int64_t td = (t1.tv_sec-t0.tv_sec)*NANO + t1.tv_nsec-t0.tv_nsec;
    if (!td)
        td = 1;
    t0 = t1;

    unsigned int prev_major = -1;
    bdevstat_t *bs = &bdev[-1];

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

        if (major > 65535)
            die("Invalid major:minor : %u:%u\n", major, minor);

        if (!rd && !wr)
            continue;

#define BDEND &bdev[ARRAYSZ(bdev)]
        if (bs < BDEND && bs[1].major==major && bs[1].minor==minor)
            bs++;
        else for (bs = bdev; bs < BDEND; bs++)
            if ((bs->major==major && bs->minor==minor) || !bs->name)
                break;
        if (bs >= BDEND)
            die("Too many block devices.\n");
        if (!bs->name)
        {
            bs->major = major;
            bs->minor = minor;
            if (!strncmp(namebuf, "mmcblk", 6) && strstr(namebuf, "boot"))
            {
                // Early boot partitions are not marked as such.
                bs->name = "mmcboot";
                bs->part = 1;
                continue;
            }

            if (!strncmp(namebuf, "mtdblock", 8))
            {
                // Raw legacy MTD -- special uses only.
                bs->name = "mtd";
                bs->part = 1;
                continue;
            }

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

        int r = ((int64_t)rd-bs->rd)*RES*1000000/td;
        int w = ((int64_t)wr-bs->wr)*RES*1000000/td;
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
