topline
=======

This tool provides a hardcopy (ie, unadorned plain text) graph of per-CPU
load, with hyperthread siblings being kept together, and NUMA node
separation graphically marked.

It is optimized for modern many-core processors -- today, servers often have
north of 100 CPUs (Xeon Scalable with 48 or 56 per socket, CL-AP up to 112
per socket), and even fat desktops reach 64 threads.  Obviously, this makes
**topline**'s display somewhat goofy on mobile stuff.

Disk load is also shown, as % read/write utilization time.

example
=======

A kernel compile on a 64-way 4-node box with 4 NVMe disks and one spinner:
```
nvme(⡆⠀⠀⠀)sd(⠀) (⠀⠀⠀⠀⠀⠀⠀⠀≬⠀⠀⠀⠀⠀⠀⠀⠀≬⣀⣀⣀⣀⣀⣀⣀⣀≬⠀⠀⠀⠀⠀⠀⠀⠀)
nvme(⡇⠀⠀⠀)sd(⠀) (⣄⣀⣀⣄⣀⣄⣀⣄≬⣀⣀⣀⣀⣄⣄⣠⣠≬⣀⣠⣀⣀⣀⣀⣀⣀≬⣀⣄⣀⣀⣀⣄⣀⣀)
nvme(⣇⠀⠀⠀)sd(⠀) (⣀⣀⣀⣀⣀⣀⣀⣀≬⠀⠀⠀⠀⡄⠀⠀⠀≬⣀⠀⠀⠀⠀⠀⠀⠀≬⣀⣀⣀⣀⣀⣀⣀⣀)
nvme(⡀⠀⠀⠀)sd(⠀) (⣶⣶⣶⣶⣶⣶⣶⣶≬⣶⣶⣶⣶⣶⣶⣶⣶≬⣶⣶⣶⣶⣶⣶⣶⣶≬⣶⣾⣶⣶⣶⣶⣶⣶)
nvme(⣀⠀⠀⠀)sd(⠀) (⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿)
nvme(⣀⠀⠀⠀)sd(⠀) (⣿⣿⣿⣷⣿⣿⣾⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿)
nvme(⣀⠀⠀⠀)sd(⠀) (⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣷⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿)
nvme(⣀⠀⠀⠀)sd(⠀) (⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣷)
nvme(⣀⠀⠀⠀)sd(⠀) (⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿)
nvme(⣀⠀⠀⠀)sd(⠀) (⣿⣿⣷⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣾⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿)
nvme(⣀⠀⠀⠀)sd(⠀) (⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿)
nvme(⣀⠀⠀⠀)sd(⠀) (⣿⣿⣿⣿⣿⣿⣿⣿≬⣾⣿⣷⣿⣷⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿≬⣿⣿⣿⣿⣿⣿⣿⣿)
```
Here, after a brief underutilized warm-up bottlenecked on disk read, CPU
parallelization becomes near-perfect, while the primary disk working at
a small fraction of its bandwidth.
