#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#define RES 1000
#define MAXCPUS 4096

#define die(...) do {fprintf(stderr, __VA_ARGS__); exit(1);} while(0)
#define ARRAYSZ(x) (sizeof(x)/sizeof(x[0]))

typedef struct set { int a; int b; } set_t[256];
#define SET_CNT(s) ((s)[0].a)
#define SET_MAX(s) ((s)[0].b)
#define SET_ITER(i,s) \
    for (int e##__LINE_=1; e##__LINE_<SET_CNT(s); e##__LINE_++) \
        for (int (i)=(s)[e##__LINE_].a; (i)<=(s)[e##__LINE_].b; (i)++)

int read_proc_int(const char *path);
int read_proc_set(const char *path, set_t *set);

void write_single(int x);
void write_dual(int x, int y);

void init_disks(void);
void do_disks(void);
void init_cpus(void);
void do_cpus(void);
