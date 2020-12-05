# blk_patch - patch an image with a patchfile

This program takes a patchfile as input and a pointer to an image 
(it can be a block device or a regular file), and applies
the patch.  The result is a modified image file/device.

The patchfile is produced by the program blk_genpatch.

The patchfile can optionally be an input stream, set the patchfile to "-"
to read from stdin.  This allows you to uncompress a patchfile on the
fly, ie:

```
 gunzip -c patchfile.gz | blk_patch -i image_name -p -
```

blk_patch will refuse to patch anything beyond the end of the image.
If presented with a patch for the last block, it will only apply as
many bytes of the last block as is present in the image.  This allows
for patching a partial end block.

blk_patch will ignore a patchblock which begins beyond the end of image. 

blk_patch assumes patch blocks are in monotonically increasing offsets
within the image.

# Usage

blk_patch -i image_name -p patchfile [-v]

image_name is name of the image device or file

patchfile is the name of the patchfile, or '-' to read from stdin

-v = verbose, will output some statistics

# testing

* patch a file with a zero length patch (no change)

```
$ cp /dev/null null.patch
$ cp Sample.Video.mp4 Sample.Video.mp4.tmp1
$ ./blk_patch -i Sample.Video.mp4.tmp1 -p null.patch -v
Number of blocks patched=0
$ md5sum Sample.Video.mp4 Sample.Video.mp4.tmp1
029774ea7c6dbcea3fe62d20d5c48355  Sample.Video.mp4
029774ea7c6dbcea3fe62d20d5c48355  Sample.Video.mp4.tmp1
```

* patch a file 
In this test we patch the above file with an appended 'a' to an appended 'b'

```
$ cp Sample.Video.mp4 Sample.Video.mp4.plus1a
$ echo -n "a" >> Sample.Video.mp4.plus1a
$ cp Sample.Video.mp4 Sample.Video.mp4.plus1b
$ echo -n "b" >> Sample.Video.mp4.plus1b
$ dd if=Sample.Video.mp4.plus1a bs=65536 | ./blk_hash -o Sample.Video.mp4.plus1a.hash > /dev/null
$ dd if=Sample.Video.mp4.plus1b bs=65536 | ./blk_hash -o Sample.Video.mp4.plus1b.hash > /dev/null
$ ls -l Sample.Video.mp4.plus1a.hash Sample.Video.mp4.plus1b.hash
$ ./blk_genpatch -i Sample.Video.mp4.plus1b -h Sample.Video.mp4.plus1a.hash -o Sample.Video.mp4.plus1a-plus1b.patch -v
Wrote 1 patch blocks / 206479 total blocks = 0 percent
$ ls -l Sample.Video.mp4.plus1a-plus1b.patch
-rw-rw-r-- 1 dlai dlai 8192 Dec  3 16:58 Sample.Video.mp4.plus1a-plus1b.patch
$ cp Sample.Video.mp4.plus1a Sample.Video.mp4.plus1a-b
$ md5sum Sample.Video.mp4.plus1a Sample.Video.mp4.plus1a-b Sample.Video.mp4.plus1b
d54ee17d0d3f926315c7f42562972855  Sample.Video.mp4.plus1a
d54ee17d0d3f926315c7f42562972855  Sample.Video.mp4.plus1a-b
cfc36196e2225536090c38e191cafc06  Sample.Video.mp4.plus1b
$ ./blk_patch -i Sample.Video.mp4.plus1a-b -p Sample.Video.mp4.plus1a-plus1b.patch -v
Number of blocks patched=1
$ md5sum Sample.Video.mp4.plus1a Sample.Video.mp4.plus1a-b Sample.Video.mp4.plus1b
d54ee17d0d3f926315c7f42562972855  Sample.Video.mp4.plus1a
cfc36196e2225536090c38e191cafc06  Sample.Video.mp4.plus1a-b
cfc36196e2225536090c38e191cafc06  Sample.Video.mp4.plus1b
```

* more extensive test of patching a much larger image

Setup 2 lvm devices with the 20201130 image (20Gig each)

```
$ lvcreate -l 5120 -n sswp-20201130-26 data
$ lvcreate -l 5120 -n sswp-20201130-29 data
$ dd if=/dev/data/sswp-20201130 of=/dev/data/sswp-20201130-26 bs=65536
$ dd if=/dev/data/sswp-20201130 of=/dev/data/sswp-20201130-29 bs=65536
$ dd if=/dev/data/sswp-20201130 bs=65536 | md5sum
bbc76024eaa8750693b5a452c27f9848  -
$ dd if=/dev/data/sswp-20201130-26 bs=65536 | md5sum
bbc76024eaa8750693b5a452c27f9848  -
$ dd if=/dev/data/sswp-20201130-29 bs=65536 | md5sum
bbc76024eaa8750693b5a452c27f9848  -
```


Now patch back to 20201129 and 20201126

```
$ blk_patch -i /dev/data/sswp-20201130-29 -p sswp-20201130-29.patch -v
Number of blocks patched=97802

$ blk_patch -i /dev/data/sswp-20201130-26 -p sswp-20201130-26.patch -v
Number of blocks patched=183725
```

(the patcher runs very fast - just a few seconds to patch 3% of a 20G image)

Compare

```
$ dd if=/dev/data/sswp-20201130 bs=65536 | md5sum
$ dd if=/dev/data/sswp-20201130-26 bs=65536 | md5sum
$ dd if=/dev/data/sswp-20201126 bs=65536 | md5sum
$ dd if=/dev/data/sswp-20201130-29 bs=65536 | md5sum
$ dd if=/dev/data/sswp-20201129 bs=65536 | md5sum

bbc76024eaa8750693b5a452c27f9848  -
251b9a8c30640d26c392d2101294558c  -
251b9a8c30640d26c392d2101294558c  -
af75f34b99f7fa981cd64589e9f51f9d  -
af75f34b99f7fa981cd64589e9f51f9d  -
```

Looks good.
