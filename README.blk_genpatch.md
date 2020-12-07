# blk_genpatch - geneate a patchfile

blk_genpatch analyzes an image (block device or file) and compares it
against a reference hashset.  It generates a patchset consisting only
of the blocks that changed with respect to the reference.  This patchset
is useful for patching the original image and making it equal to the
current image.

# usage

Usage: blk_genpatch -i image -h hashfile [-o patchfile] [-v] [-a percentage] [-z]

reads image and compares its block hashes against the
hashes in hashfile.  It will output a patchfile or
stdout if patchfile is '-'.

If -o  is not specified then no patchfile is produced.  This only
useful to get a count of how many blocks have changed.

-v reports the statistics to stderr which include how many blocks changed.

-a specifies a percentage (simple integer from 1-99) threshold for aborting.  If
the percentage of changed blocks exceeds this percentage, the program will abort early
and exit with status 2 and a message with "Abort:".
For example A script running this program can monitor the
exit status or check stderr for "Abort:" and adjust its backup strategy.

-z = ignore size differences, otherwise unmatched hashfile and image size will exit with status 2 and an appripriate Abort: message.
The truncated patchfile generated is of no use and should be discarded.



# testing

* test a zero length image file and hashfile produces empty patchfile

```
$ cp /dev/null 0b
$ ./blk_genpatch -i 0b -h 0b -o patch.0b -v
Wrote 0 patch blocks / 0 total blocks = 0 percent
$ ls -l patch.0b
-rw-rw-r-- 1 dlai dlai 0 Dec  1 22:44 patch.0b
```

* test a image file and matching hashfile produces empty patchfile

```
$ dd if=Sample.Movie.mp4 bs=65536 | ./blk_hash -o Sample.Movie.mp4.hash > /dev/null
$ ls -l Sample.Movie.mp4 Sample.Movie.mp4.hash
-rw-r--r-- 1 dlai dlai 845734063 Nov 28 23:35 Sample.Movie.mp4
-rw-rw-r-- 1 dlai dlai   3303664 Dec  1 22:58 Sample.Movie.mp4.hash
$ ./blk_genpatch -i Sample.Movie.mp4 -h Sample.Movie.mp4.hash -o Sample.Movie.mp4.patch -v
Wrote 0 patch blocks / 206479 total blocks = 0 percent
$ ls -l Sample.Movie.mp4.patch
-rw-rw-r-- 1 dlai dlai 0 Dec  1 22:59 Sample.Movie.mp4.patch
```

* test a image file with a single block change produces expected 8K patchfile

```
$ cp Sample.Movie.mp4 Sample.Movie.mp4.plus1a
$ echo -n "a" >> Sample.Movie.mp4.plus1a
$ cp Sample.Movie.mp4 Sample.Movie.mp4.plus1b
$ echo -n "b" >> Sample.Movie.mp4.plus1b
$ dd if=Sample.Movie.mp4.plus1a bs=65536 | ./blk_hash -o Sample.Movie.mp4.plus1a.hash > /dev/null
$ dd if=Sample.Movie.mp4.plus1b bs=65536 | ./blk_hash -o Sample.Movie.mp4.plus1b.hash > /dev/null
$ ls -l Sample.Movie.mp4.plus1a.hash Sample.Movie.mp4.plus1b.hash
-rw-rw-r-- 1 dlai dlai 3303664 Dec  1 23:04 Sample.Movie.mp4.plus1a.hash
-rw-rw-r-- 1 dlai dlai 3303664 Dec  1 23:04 Sample.Movie.mp4.plus1b.hash

$ md5sum Sample.Movie.mp4.plus1a.hash Sample.Movie.mp4.plus1b.hash
5700675e98cdd4f56a8f8b7f0784648b  Sample.Movie.mp4.plus1a.hash
0b928e92d44f1be5f2e72b01f69777dd  Sample.Movie.mp4.plus1b.hash

$ od -t x8 Sample.Movie.mp4.plus1a.hash > Sample.Movie.mp4.plus1a.hash.odx8
$ od -t x8 Sample.Movie.mp4.plus1b.hash > Sample.Movie.mp4.plus1b.hash.odx8
$ diff Sample.Movie.mp4.plus1a.hash.odx8 Sample.Movie.mp4.plus1b.hash.odx8
< 14464340 446ccd4a9674f5cc 03f8a7117e113f96
---
> 14464340 f2f537be97823bd0 5cdccf84f426f8d4
$ tail -5 Sample.Movie.mp4.plus1a.hash.odx8
14464260 d7d01028d95201fd fc83d34bfa48c8b4
14464300 478671f2b5b11aa2 2c5360c3b489a78d
14464320 6fdb9d44c8299ff8 9b5b13c927032766
14464340 446ccd4a9674f5cc 03f8a7117e113f96
14464360

$ ./blk_genpatch -i Sample.Movie.mp4.plus1b -h Sample.Movie.mp4.plus1a.hash -o Sample.Movie.mp4.a-b.patch -v
Wrote 1 patch blocks / 206479 total blocks = 0 percent
$ ls -l Sample.Movie.mp4.a-b.patch
-rw-rw-r-- 1 dlai dlai 8192 Dec  1 23:09 Sample.Movie.mp4.a-b.patch
```

* test mismatched image and hashfiles exit with error

```
$ dd if=Sample.Movie.mp4 of=Sample.Movie.mp4.half bs=4096 count=103000
$ ls -l Sample.Movie.mp4 Sample.Movie.mp4.half
-rw-r--r-- 1 dlai dlai 845734063 Nov 28 23:35 Sample.Movie.mp4
-rw-rw-r-- 1 dlai dlai 421888000 Dec  1 23:24 Sample.Movie.mp4.half
$ dd if=Sample.Movie.mp4 bs=65536 | ./blk_hash -o Sample.Movie.mp4.hash > /dev/null
$ dd if=Sample.Movie.mp4.half bs=65536 | ./blk_hash -o Sample.Movie.mp4.half.hash > /dev/null
$ ./blk_genpatch -i Sample.Movie.mp4 -h Sample.Movie.mp4.half.hash -o Sample.Movie.mp4.x.patch -v
Image file larger than expected by 423846063 bytes
Error: Abort
$ ls -l Sample.Movie.mp4.x.patch
ls: cannot access Sample.Movie.mp4.x.patch: No such file or directory
$ ./blk_genpatch -i Sample.Movie.mp4.half -h Sample.Movie.mp4.hash -o Sample.Movie.mp4.x.patch -v
Image file smaller than expected by 423849984 bytes
Error: Abort
$ ls -l Sample.Movie.mp4.x.patch
ls: cannot access Sample.Movie.mp4.x.patch: No such file or directory
```

* test image thats completely changed produces patchfile for all blocks

```
$ while true ; do printf "AAAAAAAAAAAAAAAA"; done | dd of=Afile bs=4096 count=10 iflag=fullblock
$ while true ; do printf "BBBBBBBBBBBBBBBB"; done | dd of=Bfile bs=4096 count=10 iflag=fullblock
$ ls -l Afile Bfile
-rw-rw-r-- 1 dlai dlai 40960 Dec  1 23:37 Afile
-rw-rw-r-- 1 dlai dlai 40960 Dec  1 23:37 Bfile
$ ./blk_hash -o Afile.hash < Afile > /dev/null
$ ./blk_hash -o Bfile.hash < Bfile > /dev/null
$ ls -l Afile.hash Bfile.hash
-rw-rw-r-- 1 dlai dlai 160 Dec  1 23:38 Afile.hash
-rw-rw-r-- 1 dlai dlai 160 Dec  1 23:39 Bfile.hash
$ ./blk_genpatch -i Afile -h Bfile.hash -o Bfile-Afile.patch -v
Wrote 10 patch blocks / 10 total blocks = 100 percent
$ ./blk_genpatch -i Bfile -h Afile.hash -o Afile-Bfile.patch -v
Wrote 10 patch blocks / 10 total blocks = 100 percent
[dlai@backup1 blk_md5]$ ls -l Afile-Bfile.patch Bfile-Afile.patch
-rw-rw-r-- 1 dlai dlai 45056 Dec  1 23:40 Afile-Bfile.patch
-rw-rw-r-- 1 dlai dlai 45056 Dec  1 23:42 Bfile-Afile.patch
```

expected size is 11*4096 = 45056 bytes

* test large image with a known percentage change produces patchfile
of size similar to the percentage changed.

lvs on origin server:

```
  ss-wp.snap.20201124-060502 vg0    swi-a-s---  10.00g      ss-wp  11.02        
  ss-wp.snap.20201125-060501 vg0    swi-a-s---  10.00g      ss-wp  10.49        
  ss-wp.snap.20201126-060502 vg0    swi-a-s---  10.00g      ss-wp  9.58         
  ss-wp.snap.20201127-060503 vg0    swi-a-s---  10.00g      ss-wp  9.15         
  ss-wp.snap.20201128-060502 vg0    swi-a-s---  10.00g      ss-wp  8.73         
  ss-wp.snap.20201129-060501 vg0    swi-a-s---  10.00g      ss-wp  7.27         
  ss-wp.snap.20201130-060502 vg0    swi-a-s---  10.00g      ss-wp  5.37         
  ss-wp.snap.20201201-060502 vg0    swi-a-s---  10.00g      ss-wp  3.10         
```

Note that from 20201130 snap to 20201129 snap theres approx 1.9% change
from 20201130 to 20201126 theres about 4.2% change
This is based on a 10G snapshot, so the amount if change is
about

20201130 to 20201129: 1.9% of 10G = 190 Meg

20201130 to 20201126: 4.2% of 10G = 420 Meg


On backup server where the images are stored:

```
[root@backup ss-wp]# ls -l
total 56499236
-rw-rw-r--. 1 backup backup 7166669604 Nov 30 20:11 ss-wp.snap.20201123-060502.dd.gz
-rw-r--r--. 1 backup backup   83886080 Dec  2 01:15 ss-wp.snap.20201123-060502.hash
-rw-rw-r--. 1 backup backup 7163566418 Nov 30 20:03 ss-wp.snap.20201124-060502.dd.gz
-rw-r--r--. 1 backup backup   83886080 Dec  2 01:15 ss-wp.snap.20201124-060502.hash
-rw-rw-r--. 1 backup backup 7158356616 Nov 30 19:55 ss-wp.snap.20201125-060501.dd.gz
-rw-r--r--. 1 backup backup   83886080 Dec  2 01:15 ss-wp.snap.20201125-060501.hash
-rw-rw-r--. 1 backup backup 7168540047 Nov 30 19:47 ss-wp.snap.20201126-060502.dd.gz
-rw-r--r--. 1 backup backup   83886080 Dec  2 01:15 ss-wp.snap.20201126-060502.hash
-rw-rw-r--. 1 backup backup 7155009754 Nov 30 19:39 ss-wp.snap.20201127-060503.dd.gz
-rw-r--r--. 1 backup backup   83886080 Dec  2 01:15 ss-wp.snap.20201127-060503.hash
-rw-rw-r--. 1 backup backup 7149387372 Nov 30 19:31 ss-wp.snap.20201128-060502.dd.gz
-rw-r--r--. 1 backup backup   83886080 Dec  2 01:15 ss-wp.snap.20201128-060502.hash
-rw-rw-r--. 1 backup backup 7121497825 Nov 30 19:23 ss-wp.snap.20201129-060501.dd.gz
-rw-r--r--. 1 backup backup   83886080 Dec  2 01:16 ss-wp.snap.20201129-060501.hash
-rw-rw-r--. 1 backup backup 7101058683 Nov 30 18:17 ss-wp.snap.20201130-060502.dd.gz
-rw-r--r--. 1 backup backup   83886080 Dec  2 01:16 ss-wp.snap.20201130-060502.hash
```

Create the devices for this test

```
# lvcreate -l 5120 -n sswp-20201130 data
# lvcreate -l 5120 -n sswp-20201129 data
# lvcreate -l 5120 -n sswp-20201126 data
```

restore the images to the newly created LVMs

```
#  gunzip -c ss-wp.snap.20201130-060502.dd.gz | dd bs=65536 of=/dev/data/sswp-20201130
#  gunzip -c ss-wp.snap.20201129-060501.dd.gz | dd bs=65536 of=/dev/data/sswp-20201129
#  gunzip -c ss-wp.snap.20201126-060502.dd.gz | dd bs=65536 of=/dev/data/sswp-20201126
```

verify the hashes are the same

```
#  dd if=/dev/data/sswp-20201130 bs=65536 | blk_hash -o sswp-20201130.hash > /dev/null
#  dd if=/dev/data/sswp-20201129 bs=65536 | blk_hash -o sswp-20201129.hash > /dev/null
#  dd if=/dev/data/sswp-20201126 bs=65536 | blk_hash -o sswp-20201126.hash > /dev/null
```

Compare against the hashes generated from origin:

generate hashes on origin (example with copy to backup server)
```
 for i in ss-wp.snap.20201130-060502 ss-wp.snap.20201129-060501; do
  dd if=/dev/vg0/$i bs=65536 | blk_hash -o $i.hash | gzip -c -2 | ssh backup@backup "cat - > /backup/origin/ss-wp/$i.dd.gz"
 done
```

generate hashes on origin (example without copy):

```
 for i in ss-wp.snap.20201130-060502 ss-wp.snap.20201129-060501; do
   time dd if=/dev/vg0/$i bs=65536 | blk_hash -o $i.hash.n > /dev/null
 done
```


compare
```
#  md5sum sswp-20201130.hash ss-wp.snap.20201130-060502.hash
8b87f25652d0416b8c257be4d8de3294  sswp-20201130.hash
8b87f25652d0416b8c257be4d8de3294  ss-wp.snap.20201130-060502.hash
#  md5sum sswp-20201129.hash ss-wp.snap.20201129-060501.hash
865e1a0aeeeeab9067c2e99fd5ad4669  sswp-20201129.hash
865e1a0aeeeeab9067c2e99fd5ad4669  ss-wp.snap.20201129-060501.hash
#  md5sum sswp-20201126.hash ss-wp.snap.20201126-060502.hash
901897a520855b75077f761cd18a6b97  sswp-20201126.hash
901897a520855b75077f761cd18a6b97  ss-wp.snap.20201126-060502.hash
```


Now generate the patch files

```
# time blk_genpatch -i /dev/data/sswp-20201129 -h sswp-20201130.hash -o sswp-20201130-29.patch -v
Wrote 97802 patch blocks / 5242880 total blocks = 1 percent
real    1m1.165s
```

```
# time blk_genpatch -i /dev/data/sswp-20201126 -h sswp-20201130.hash -o sswp-20201130-26.patch -v
Wrote 183725 patch blocks / 5242880 total blocks = 3 percent
real    1m3.119s
```

It appears the runtime didnt vary much, about 1 min for a 20G image.
Looking at the above sizes 

97802 blocks = 390 Meg  (We were expecting 190 Meg)

183725 blocks = 734 Meg (We were expecting 420 Meg)

Hmm - I dont know how to account for the despcrepency.  Perhaps LVM keeps
track of smaller change units, maybe 512 byte sectors,



Check patch size and compressed size
```
# ls -l sswp-20201130-29.patch sswp-20201130-26.patch
-rw-r--r--. 1 root root 754008064 Dec  2 01:42 sswp-20201130-26.patch
-rw-r--r--. 1 root root 401383424 Dec  2 01:45 sswp-20201130-29.patch

# gzip -c -2 < sswp-20201130-29.patch > sswp-20201130-29.patch.gz2
# gzip -c -2 < sswp-20201130-26.patch > sswp-20201130-26.patch.gz2
# ls -l sswp-20201130-29.patch.gz2 sswp-20201130-26.patch.gz2
-rw-r--r--. 1 root root 245577960 Dec  2 01:47 sswp-20201130-26.patch.gz2
-rw-r--r--. 1 root root 116376210 Dec  2 01:47 sswp-20201130-29.patch.gz2
```

Compression took about 10 sec, and compressed file approx 33% size of 
original.

* test abort with low thershold

The patch for sswp-20201130-26.patch above is about 3.5%, set a 2% threshold
and run the genpatch

```
# time blk_genpatch -i /dev/data/sswp-20201126 -h sswp-20201130.hash -o sswp-20201130-26.patch2 -a 2
Abort threshold 104858 patches exceeded
real    0m47.028s

# ls -l sswp-20201130-26.patch2
-rw-r--r--. 1 root root 428654592 Dec  2 02:05 sswp-20201130-26.patch2
```

Looks about right, size of patch created is 428654592/754008064
and runtime is shorter.  and 428654592 is very close to 2% of the 
image size of 21474836480.  Of course the patchfile is not useable and
should be deleted.




