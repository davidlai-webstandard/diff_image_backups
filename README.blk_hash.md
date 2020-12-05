# blk_hash - md5sum each block (4K) of data in a stream and generate a file of hashes

This program reads data from stdin and echoes it to stdout so it can be
used as a transparent pipe.

The stream is blocked into 4K chunks and hashed using md5.  The hashes are
written to a hashfile.

# Usage

* blk_hash -o hashfile
> place hashes in hashfile

* blk_hash -n
> transparent pipe, no hashing, not really very useful

* blk_hash -o /dev/null
> transparent pipe, do hashing but throw away results.  Only useful for
> benchmarking the hashing function.

# Example

  dd ... | blk_hash -o hashfile | cat ...

Insert blk_hash into the pipeline (a typical pipeline would be an image
backup).
The output from dd (or any other process that writes stdout) is piped to blk_hash,
which generates the hashfile.
The stream is echoed downtream to cat (or any other process which consumes stdin)

# Details

The hashfile generated will consist of md5 hash of each 4K (4096 byte) block of
input.  Each md5 hash is 16 bytes (binary).  So, for example, a stream of size 1 Megabyte
(1048576 bytes) will have 256 blocks, and the hashfile generated will be 4096 bytes.

The program leverages libssl for the hashing function.

# Compiling

see the Makefile

The program will compile against 1.0.0 and 1.1.0 openssl.  There were some
significant API changes between them but the code takes care of it.

# testing

Using a sample input file Video.Sample.mp4 size 845734063


* generate hashfile is of correct size, check input and output is the same

We expect a hashfile to be approx 1/256th the size of the image for 
sufficiently large image sizes.

```
$ ./blk_hash -o Video.Sample.hash < Video.Sample.mp4 > Video.Sample.mp4.out
$ ls -l Video.Sample.mp4 Video.Sample.hash Video.Sample.mp4.out
-rw-rw-r-- 1 dlai dlai   3303664 Dec  4 18:59 Video.Sample.hash
-rw-r--r-- 1 dlai dlai 845734063 Dec  4 18:57 Video.Sample.mp4
-rw-rw-r-- 1 dlai dlai 845734063 Dec  4 18:59 Video.Sample.mp4.out
$ md5sum Video.Sample.mp4 Video.Sample.mp4.out
029774ea7c6dbcea3fe62d20d5c48355  Video.Sample.mp4
029774ea7c6dbcea3fe62d20d5c48355  Video.Sample.mp4.out
$ expr 3303664 \* 256
845737984
```

* check for memory leaks

verify during the run that the memory usage stays constant (using top)

* verify the first several checksums using md5sum against the original blocks

```
$ dd if=Video.Sample.mp4 bs=4096 count=1 status=noxfer | md5sum
372e6fc37f35076538b14706098c9562  -
$ od -t x1 Video.Sample.hash | head -1 | awk '{$1=""; print}' | sed 's/ //g'
372e6fc37f35076538b14706098c9562

$ dd if=Video.Sample.mp4 bs=4096 count=1 skip=1 status=noxfer | md5sum
501b590da5b1093ce2855ade5f7e489d  -
$ od -t x1 Video.Sample.hash | tail -n +2 | head -1 | awk '{$1=""; print}' | sed 's/ //g'
501b590da5b1093ce2855ade5f7e489d

$ dd if=Video.Sample.mp4 bs=4096 count=1 skip=2 status=noxfer | md5sum
8ff6f3addfd60d4cbf73b59a75f08728  -
$ od -t x1 Video.Sample.hash | tail -n +3  | head -1 | awk '{$1=""; print}' | sed 's/ //g'
8ff6f3addfd60d4cbf73b59a75f08728
```


* verify the final checksum using md5sum on the final block padded with zeros

Video.Sample.mp4 size 845734063
has 206478 4K blocks plus 175 byte partial block at end

Grab the last (partial) block:

    dd if=Video.Sample.mp4 bs=4096 skip=206478 > Video.Sample.mp4.lastblock

pad the last block with zeros and checksum

    cp Video.Sample.mp4.lastblock Video.Sample.mp4.lastblock.zero_padded
    dd if=/dev/zero bs=1 count=3921 >> Video.Sample.mp4.lastblock.zero_padded
    md5sum Video.Sample.mp4.lastblock.zero_padded
     e794be3db9c3e8a7ce95d53ce0b0e5df

compare against the final checksum in the hashfile

    od -t x1 Video.Sample.hash | tail -2 | awk '{$1=""; print}' | sed 's/ //g'
    e794be3db9c3e8a7ce95d53ce0b0e5df


matches

* test a 0 length input file

```
  $ dd if=/dev/null | ./blk_hash -o hashfile0 | dd of=outfile0
  $ ls -l hashfile0 outfile0
  -rw-rw-r-- 1 dlai dlai 0 Nov 29 11:07 hashfile0
  -rw-rw-r-- 1 dlai dlai 0 Nov 29 11:07 outfile0
```

* test a simple input file < 4K 

```
$ echo "abc" | ./blk_hash -o hashfile4 | dd of=outfile4
$ ls -l hashfile4 outfile4
-rw-rw-r-- 1 dlai dlai 16 Nov 29 11:08 hashfile4
-rw-rw-r-- 1 dlai dlai  4 Nov 29 11:08 outfile4
$ od -c outfile4
0000000   a   b   c  \n
0000004
$ cp outfile4 outfile4.padded
$ dd if=/dev/zero bs=1 count=4092 >> outfile4.padded
$ ls -l outfile4.padded
-rw-rw-r-- 1 dlai dlai 4096 Nov 29 11:10 outfile4.padded
$ md5sum outfile4.padded
a47172aea996a7d92203724901e8c09d  outfile4.padded
$ od -t x1 hashfile4
0000000 a4 71 72 ae a9 96 a7 d9 22 03 72 49 01 e8 c0 9d
```

* test a input file exactly 4K

```
$ dd if=Video.Sample.mp4 bs=4096 count=1 of=Video.Sample.mp4.1block
$ ls -l Video.Sample.mp4.1block
-rw-rw-r-- 1 dlai dlai 4096 Nov 29 11:12 Video.Sample.mp4.1block
dd if=Video.Sample.mp4.1block bs=65536 | ./blk_hash -o hashfile1 | dd of=outfile1
$ ls -l hashfile1 outfile1
-rw-rw-r-- 1 dlai dlai   16 Nov 29 11:13 hashfile1
-rw-rw-r-- 1 dlai dlai 4096 Nov 29 11:13 outfile1
$ md5sum outfile1 Video.Sample.mp4.1block
372e6fc37f35076538b14706098c9562  outfile1
372e6fc37f35076538b14706098c9562  Video.Sample.mp4.1block
$ od -t x1 hashfile1
0000000 37 2e 6f c3 7f 35 07 65 38 b1 47 06 09 8c 95 62
```

* test program openssl 1.0.0 and openssl 1.1.1

# Benchmarking

## Pure read speed:

```
$ dd if=Video.Sample.mp4 bs=65536 of=/dev/null
845734063 bytes (846 MB) copied, 0.416848 s, 2.0 GB/s
$ dd if=Video.Sample.mp4 bs=65536 of=/dev/null
845734063 bytes (846 MB) copied, 0.41883 s, 2.0 GB/s
$ dd if=Video.Sample.mp4 bs=65536 of=/dev/null
845734063 bytes (846 MB) copied, 0.418879 s, 2.0 GB/s
$ dd if=Video.Sample.mp4 bs=1048576 of=/dev/null
845734063 bytes (846 MB) copied, 0.687879 s, 1.2 GB/s
$ dd if=Video.Sample.mp4 bs=1048576 of=/dev/null
845734063 bytes (846 MB) copied, 0.688345 s, 1.2 GB/s
$ dd if=Video.Sample.mp4 bs=1048576 of=/dev/null
845734063 bytes (846 MB) copied, 0.688756 s, 1.2 GB/s
```

It appears the 64K block reads faster

## Read and write with no hashing

```
$ dd if=Video.Sample.mp4 of=Video.Sample.mp4.2 bs=65536
845734063 bytes (846 MB) copied, 10.7779 s, 78.5 MB/s
$ dd if=Video.Sample.mp4 of=Video.Sample.mp4.2 bs=65536
845734063 bytes (846 MB) copied, 11.695 s, 72.3 MB/s
$ dd if=Video.Sample.mp4 of=Video.Sample.mp4.2 bs=65536
845734063 bytes (846 MB) copied, 10.8424 s, 78.0 MB/s
$ dd if=Video.Sample.mp4 of=Video.Sample.mp4.2 bs=65536
845734063 bytes (846 MB) copied, 8.89644 s, 95.1 MB/s
$ dd if=Video.Sample.mp4 of=Video.Sample.mp4.2 bs=65536
845734063 bytes (846 MB) copied, 11.7518 s, 72.0 MB/s

$ dd if=Video.Sample.mp4 of=Video.Sample.mp4.2 bs=1048576
845734063 bytes (846 MB) copied, 13.3234 s, 63.5 MB/s
$ dd if=Video.Sample.mp4 of=Video.Sample.mp4.2 bs=1048576
845734063 bytes (846 MB) copied, 15.3441 s, 55.1 MB/s
$ dd if=Video.Sample.mp4 of=Video.Sample.mp4.2 bs=1048576
845734063 bytes (846 MB) copied, 11.8472 s, 71.4 MB/s
$ dd if=Video.Sample.mp4 of=Video.Sample.mp4.2 bs=1048576
845734063 bytes (846 MB) copied, 12.4784 s, 67.8 MB/s
$ dd if=Video.Sample.mp4 of=Video.Sample.mp4.2 bs=1048576
845734063 bytes (846 MB) copied, 12.1382 s, 69.7 MB/s
```

It appears the 64K blocks are slightly faster, note that writing is
significantly slower than reading.  Note: the read speeds are affected
by caching, so they should be taken with a grain of salt.


## Modify blk_hash.c for 64K buffers and 1M buffers

```
dd if=Video.Sample.mp4 bs=65536 | ./blk_hash.64k -o Video.Sample.hash6.64k | dd bs=65536 of=trmp4.6.64k
845734063 bytes (846 MB) copied, 10.7504 s, 78.7 MB/s
845734063 bytes (846 MB) copied, 10.954 s, 77.2 MB/s
845734063 bytes (846 MB) copied, 11.4876 s, 73.6 MB/s


dd if=Video.Sample.mp4 bs=1048576 | ./blk_hash -o Video.Sample.hash.1m | dd bs=1048576 of=trmp4.1m
845734063 bytes (846 MB) copied, 10.4673 s, 80.8 MB/s
845734063 bytes (846 MB) copied, 11.642 s, 72.6 MB/s
845734063 bytes (846 MB) copied, 10.6635 s, 79.3 MB/s
```

Speeds for the 2 block sizes are similar.

conclusion: Go with 64K blocking

## test transparent pipe to see how it slows

```
$ dd if=Video.Sample.mp4 of=/dev/null bs=65536
845734063 bytes (846 MB) copied, 9.17169 s, 92.2 MB/s
845734063 bytes (846 MB) copied, 0.434709 s, 1.9 GB/s
845734063 bytes (846 MB) copied, 0.419563 s, 2.0 GB/s
845734063 bytes (846 MB) copied, 0.418383 s, 2.0 GB/s
```

Note the first result actually reads the file, the next 3 are cached.

```
$ time dd if=Video.Sample.mp4 bs=65536 | cat - > /dev/null
845734063 bytes (846 MB) copied, 5.44051 s, 155 MB/s
845734063 bytes (846 MB) copied, 1.18026 s, 717 MB/s
845734063 bytes (846 MB) copied, 1.23159 s, 687 MB/s
845734063 bytes (846 MB) copied, 1.18182 s, 716 MB/s
845734063 bytes (846 MB) copied, 1.23457 s, 685 MB/s
```

This tests a transparent pipe data sink, the pipeline slows things
down a bit.

Now insert the hashing program

```
$ time dd if=Video.Sample.mp4 bs=65536 | ./blk_hash -n > /dev/null
845734063 bytes (846 MB) copied, 1.17691 s, 719 MB/s
845734063 bytes (846 MB) copied, 1.20505 s, 702 MB/s
845734063 bytes (846 MB) copied, 1.18243 s, 715 MB/s
```

Similar speeds to cat.

Try adding hashing but throw away the results

```
$ time dd if=Video.Sample.mp4 bs=65536 | ./blk_hash -o /dev/null  > /dev/null
845734063 bytes (846 MB) copied, 2.80192 s, 302 MB/s
845734063 bytes (846 MB) copied, 2.79948 s, 302 MB/s
845734063 bytes (846 MB) copied, 2.79559 s, 303 MB/s
```


A bit slower because of the hashing

```
$ time dd if=Video.Sample.mp4 bs=65536 | ./blk_hash -o hashfile1  > /dev/null
845734063 bytes (846 MB) copied, 2.83096 s, 299 MB/s
845734063 bytes (846 MB) copied, 2.86007 s, 296 MB/s
845734063 bytes (846 MB) copied, 2.87692 s, 294 MB/s
```

Writing out the hashfile has insignificant impact on the speed


