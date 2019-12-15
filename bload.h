#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#define RES 1000
#define MAXCPUS 4096

#define die(...) do {fprintf(stderr, __VA_ARGS__); exit(1);} while(0)
#define ARRAYSZ(x) (sizeof(x)/sizeof(x[0]))

int read_proc_int(const char *path);

void write_ht(int x, int y);

void init_disks(void);
void do_disks(void);
