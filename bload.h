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

int read_proc_int(const char *path);
int read_proc_set(const char *path, set_t *set);

void write_single(int x);
void write_ht(int x, int y);

void init_disks(void);
void do_disks(void);
void init_cpus(void);
void do_cpus(void);
