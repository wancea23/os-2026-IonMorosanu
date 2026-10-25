# Lab 1. The OS as a Resource Manager

Ion Moroșanu, FAF-242. Ubuntu 26.04 virtual machine (VirtualBox 7.2, 4 CPUs, 4096 MB, 25 GB disk), user `wancea`,
work folder `~/os-lab1`. The full report with the screenshots is `Lab1_IonMorosanu_FAF-242.docx` in this folder.
The outputs below are copied from those screenshots.

```
wancea
Linux OS-Lab1-2 7.0.0-34-generic #34-Ubuntu SMP PREEMPT_DYNAMIC Wed Sep  2 14:29:37 UTC 2026 x86_64 GNU/Linux
 10:51:04 up 1 min,  1 user,  load average: 3.85, 1.48, 0.54
```

## Part 1. Files and directories

`ls -l note.txt` before and after `chmod 600 note.txt`:

```
-rw-rw-r-- 1 wancea wancea 24 Oct  1 10:53 note.txt
-rw------- 1 wancea wancea 24 Oct  1 10:53 note.txt
```

1. The files belong to the user `wancea` and to the group `wancea` (the private group Ubuntu makes for every user).
2. After `chmod 600`, note.txt shows `-rw-------`: `-` regular file, `rw-` the owner may read and write (not execute),
   `---` the group has no rights, `---` the other users have no rights.
3. `/etc` holds the configuration files of the whole system (for example NetworkManager, adduser.conf).
   `/home` holds one personal folder for every user, here `/home/wancea`.

Interesting: `/bin`, `/lib`, `/lib64` and `/sbin` are symbolic links to `usr/bin`, `usr/lib`, `usr/lib64` and `usr/sbin`,
and `/proc` and `/sys` have the size 0, because the kernel makes their content when it is read.

## Part 2. Processes

```
Tasks: 245 total,   1 running, 244 sleeping,   0 stopped,   0 zombie
```

From `/proc/5550/status` of a background `sleep 300`:

```
Name:   sleep
State:  S (sleeping)
Pid:    5550
PPid:   4228
VmSize:    16112 kB
```

1. PID 1 is `systemd`, the init process: the first program the kernel starts after boot. It starts the services and adopts
   every process whose parent has ended.
2. About 245 processes were running on the idle VM (top: 245 tasks).
3. `State: S (sleeping)`: sleep only waits for its timer and does not use the CPU.

Interesting: `kill <PID>` typed literally failed, because bash reads `<` as an input redirection.

## Part 3. Memory

```
               total        used        free      shared  buff/cache   available
Mem:           3.3Gi       1.4Gi       348Mi        34Mi       1.8Gi       1.9Gi
Swap:             0B          0B          0B
MemTotal:        3460392 kB
VmRSS:       224 kB
```

1. 3.3 GiB in total (MemTotal 3460392 kB), 348 MiB completely free, 1.9 GiB available (the 1.8 GiB of disk cache can be given back).
2. Swap is disk space where the kernel moves memory pages that were not used for a while when the RAM is full. None is configured (0B).
3. A bare `sleep` uses only 224 kB of real RAM, although its virtual size is about 16 MB. A small surprise: the kernel
   loads a page only when the process touches it (demand paging).

## Part 4. Devices and storage

```
sda      8:0    0    25G  0 disk
├─sda1   8:1    0     1G  0 part /boot/efi
└─sda2   8:2    0  23.9G  0 part /
/dev/sda2 on / type ext4 (rw,relatime)
crw-------  1 root   tty       5,   1 Oct  1 10:49 console
```

1. `/` is mounted on `/dev/sda2`, ext4.
2. `/dev/console` is a character device (5, 1) that stands for the system console.
3. "Everything is a file": disks, terminals, kernel information and even processes are reached through names in the same tree
   and are read and written with the same operations as ordinary files.

## Synthesis

The OS manages four resources: **files** (seen with `ls -l`), **processes** (`ps -ef`, `top`), **memory** (`free -h`) and
**devices** (`lsblk`). Each of these commands only reads what the kernel already keeps about that resource.
