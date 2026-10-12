# Lab 5. The File System with Your Own Hands

Ion Moroșanu, FAF-242. Ubuntu 26.04 virtual machine, user `wancea` (not root), work folder `~/os-lab5`.
The full report with screenshots is `lab6/Lab5-6_IonMorosanu_FAF-242.docx` (Labs 5 and 6 combined).

## Part 1. The tree and the file types

```
$ ls -ld /etc /etc/passwd /bin /dev/null /tmp
lrwxrwxrwx   1 root root     7 Apr 20 08:46 /bin -> usr/bin
crw-rw-rw-   1 root root  1, 3 Oct 12 16:57 /dev/null
drwxr-xr-x 142 root root 12288 Oct  5 12:43 /etc
-rw-r--r--   1 root root  2772 Sep 29 12:31 /etc/passwd
drwxrwxrwt  15 root root   380 Oct 12 17:01 /tmp
$ file /usr/bin/ls /etc/passwd /bin
/usr/bin/ls: symbolic link to ../lib/cargo/bin/coreutils/ls
/etc/passwd: ASCII text
/bin:        symbolic link to usr/bin
$ du -sh /var/log 2>/dev/null ; du -sh /usr 2>/dev/null
221M    /var/log
5.2G    /usr
```

1. `/bin` l, symbolic link. `/dev/null` c, character device. `/etc` d, directory. `/etc/passwd` -, regular file. `/tmp` d, directory (with the sticky bit t).
2. `/bin` is a symbolic link to `usr/bin` (merged /usr). Even `/usr/bin/ls` is a link, to the Rust coreutils of Ubuntu 26.04.
3. Character device: `/dev/null` (1, 3), also `/dev/console` (5, 1). Block device: `/dev/sda` (8, 0), its partitions `/dev/sda1` and `/dev/sda2`, and `/dev/sr0` (11, 0).
4. `/usr` (5.2 GB) is about 24 times bigger than `/var/log` (221 MB).

## Part 2. Inodes and links

```
$ stat notes.txt
  File: notes.txt
  size: 6               Blocks: 8          IO Block: 4096   regular file
Device: 8,2     Inode: 263824      Links: 1
$ ln notes.txt hard.txt ; ls -li notes.txt hard.txt
263824 -rw-rw-r-- 2 wancea wancea 6 Oct 12 17:05 hard.txt
263824 -rw-rw-r-- 2 wancea wancea 6 Oct 12 17:05 notes.txt
$ rm notes.txt ; cat hard.txt ; ls -li hard.txt
hello
263824 -rw-rw-r-- 1 wancea wancea 6 Oct 12 17:05 hard.txt
$ ln -s hard.txt soft.txt ; ls -li hard.txt soft.txt
263824 -rw-rw-r-- 1 wancea wancea 6 Oct 12 17:05 hard.txt
263607 lrwxrwxrwx 1 wancea wancea 8 Oct 12 17:05 soft.txt -> hard.txt
$ rm hard.txt ; cat soft.txt
cat: soft.txt: No such file or directory
$ mkdir d ; ls -ld d            ->  drwxrwxr-x 2 ...
$ mkdir d/x d/y ; ls -ld d      ->  drwxrwxr-x 4 ...
$ echo x > one.txt ; ls -l one.txt ; du -h one.txt     ->  2 bytes, 4.0K
$ truncate -s 1G big.img ; ls -lh big.img ; du -h big.img  ->  1.0G, 0
```

1. `Inode: 263824` is the inode number. `Links: 1` is how many names (directory entries) the file has.
2. `rm` calls unlink: it removes one name and decrements the link count. The inode and its data are freed only when the count is 0 and nobody has the file open. `hard.txt` was a second name of the same inode, so the data was still there.
3. A symbolic link stores a path, not an inode. After `hard.txt` was removed the path leads nowhere (a dangling link), although `soft.txt` itself still exists. Its size is 8 bytes, the length of the text `hard.txt`.
4. 2 links = the entry `d` in `~/os-lab5` + the entry `.` inside `d`. Each subdirectory has `..` pointing back to `d`, so `x` and `y` add one each: 4.
5. Space is allocated in whole 4 KB blocks, so 2 bytes use one block (4.0K). `truncate` only writes the size into the inode and allocates no blocks: a sparse file whose holes read as zeros and use no space.

## Part 3. Permissions

```
$ chmod 600 s.txt ; ls -l s.txt        ->  -rw------- 1 wancea wancea 7 ... s.txt
$ chmod 000 s.txt ; cat s.txt          ->  cat: s.txt: Permission denied
$ chmod u+r s.txt ; cat s.txt          ->  secret
$ umask ; touch new.txt ; mkdir newdir ; ls -ld new.txt newdir
0002
-rw-rw-r-- 1 wancea wancea    0 Oct 12 17:06 new.txt
drwxrwxr-x 2 wancea wancea 4096 Oct 12 17:06 newdir
$ chmod u=r,go= box  ; ls box ; cat box/f
f
cat: box/f: Permission denied
$ chmod u=x,go= box  ; ls box ; cat box/f
ls: cannot open directory 'box': Permission denied
hi
$ ls -l /usr/bin/passwd ; ls -ld /tmp
-rwsr-xr-x 1 root root 93640 Feb  2  2026 /usr/bin/passwd
drwxrwxrwt 15 root root 780 Oct 12 17:05 /tmp
$ cat /etc/shadow                       ->  cat: /etc/shadow: Permission denied
$ ls -l /etc/shadow                     ->  -rw-r----- 1 root shadow 1312 Oct  1 08:42 /etc/shadow
```

1. 600 = `rw-------` (private file, an SSH private key). 644 = `rw-r--r--` (ordinary readable file, `/etc/passwd`). 755 = `rwxr-xr-x` (program or public folder, `/usr/bin/ls`, `/etc`). 700 = `rwx------` (private folder, `~/.ssh`).
2. The kernel uses only the class that matches: for the owner it checks only the owner bits, and with 000 they deny reading. Ownership only gives the right to chmod, which is why `chmod u+r` worked without root.
3. umask 0002 removes write for others. Files start from 666 and folders from 777, so `new.txt` is 664 and `newdir` is 775.
4. On a folder, r lets you read the list of names (ls works), x lets you pass through the folder to an entry whose name you know (cat box/f works). r without x: names but no access. x without r: access but no listing.
5. s (setuid) on `passwd`: the program runs as its owner, root, whoever starts it. It needs this to rewrite `/etc/shadow`, which normal users cannot even read. t (sticky) on `/tmp`: everyone can create files there, but only the owner of a file (or root) can delete or rename it.

## Part 4. Disks and the kernel as files

```
$ lsblk   (snap loop devices left out)
sda      8:0    0    25G  0 disk
├─sda1   8:1    0     1G  0 part /boot/efi
└─sda2   8:2    0  23.9G  0 part /
sr0     11:0    1  50.7M  1 rom  /run/media/wancea/VBox_GAs_7.2.20
$ df -h | grep sda2         ->  /dev/sda2  24G  8.0G  15G  36% /
$ mount | grep " / "        ->  /dev/sda2 on / type ext4 (rw,relatime)
$ grep -c processor /proc/cpuinfo   ->  4
$ head -3 /proc/meminfo
MemTotal:        2175032 kB
MemFree:          252524 kB
MemAvailable:    1009312 kB
$ head -5 /proc/self/status  ->  Name: head
$ ls -l /proc/self/fd        ->  0, 1, 2 -> /dev/pts/0 ; 3 -> /proc/8308/fd
```

1. Disk `sda` (25 GB): `sda1` 1 GB, EFI partition at `/boot/efi`; `sda2` 23.9 GB at `/`, file system ext4. (`sr0` is the Guest Additions CD, `loop0..18` are snaps.)
2. The root file system is 36% full: 8.0 GB used, 15 GB free of 24 GB (ext4 keeps 5% reserved for root).
3. 4 processors and MemTotal 2175032 kB (about 2.1 GB). This is the virtual machine, which had 4 virtual CPUs and 2.5 GB of RAM for this session; the firmware and the kernel keep the rest.
4. `head`: `/proc/self` always points to the process that reads it, and here that process was `head` itself. In the same way `ls -l /proc/self/fd` showed the descriptors of `ls`.

## Stretch (a) and (d)

```
$ gzip -c /etc/passwd > notes.txt ; file notes.txt ; xxd notes.txt | head -1
notes.txt: gzip compressed data, was "passwd", last modified: Tue Sep 29 12:31:14 2026, from Unix, original size modulo 2^32 2772
00000000: 1f8b 0808 92af bb6a 0003 7061 7373 7764  .......j..passwd
```

(a) `1f 8b` is the gzip magic number, `08` the method deflate, `08` the flag "original name stored". `92 af bb 6a` is the modification time (little endian 0x6abbaf92 = 29 Sep 2026 12:31:14), then `00`, `03` (Unix) and the name `passwd`. `file` reads these bytes and ignores the `.txt` extension.

```
$ truncate -s 64M disk.img ; mkfs.ext4 disk.img
Creating filesystem with 16384 4k blocks and 16384 inodes
Creating journal (1024 blocks): done
$ sudo mount -o loop disk.img /mnt ; df -h /mnt ; df -i /mnt ; sudo umount /mnt
/dev/loop19      56M  152K   52M   1% /mnt
/dev/loop19    16384    12 16372    1% /mnt
```

(d) mkfs created 16384 inodes, one for every 4 KB of the image. df shows only 56 MB because the inode table (16384 x 256 B = 4 MB) and the journal (4 MB) are already taken. 12 inodes are in use from the start: 1 to 10 are reserved (2 is the root folder, 8 the journal), 11 is `lost+found` and 12 is the orphan file (checked with `debugfs -R "stat <12>" disk.img`).
