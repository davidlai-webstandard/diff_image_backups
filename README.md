# diff_image_backups
Helper programs for differential image backups of block devices (entire hard drive images)

# blk_hash, blk_genpatch, blk_patch

These 3 programs will assist you in creating a backup system for
large disk images.

# Compiling

see the Makefile

On Linux you will need to have the usual dev tools like the C compiler
and make, etc..

You will also need to load the openssl-devel package for the header
files and libs.
The program will compile against 1.0.0 and 1.1.0 openssl.  There were some
significant API changes between them but the code takes care of it.

optionally make the -dbg version of the program if you want to take a
crack at running the debugger against it.  best of luck.


# Differential Backups

One of the problems with backups of entire disk
images is you will need to store gigantic image files (10Gig - 200 Gig or more).
Image size are on the order  of the size of the hard drives.  If you plan to do a daily backup the
storage needs will soon overwhelm even the largest storage servers.

Some strategies can be employed to lessen the load on the storage requirements.
Compression comes to mind.  But even with compression, lets say you can 
compress to 40%, the amount of storage is still huge.

One thing that can be said of disk images is that from day to day, very little
changes.  As an example here are some details on a 20G disk for a wordpress
server:

```
[root@vs12 ~]# lvs
  LV                         VG     Attr       LSize   Pool Origin Data%  Meta%  Move Log Cpy%Sync Convert
  ss-wp                      vg0    owi-aos---  20.00g
  ss-wp.snap.20201126-060502 vg0    swi-a-s---  10.00g      ss-wp  11.02
  ss-wp.snap.20201127-060503 vg0    swi-a-s---  10.00g      ss-wp  10.65
  ss-wp.snap.20201128-060502 vg0    swi-a-s---  10.00g      ss-wp  10.41
  ss-wp.snap.20201129-060501 vg0    swi-a-s---  10.00g      ss-wp  9.31
  ss-wp.snap.20201130-060502 vg0    swi-a-s---  10.00g      ss-wp  8.03
  ss-wp.snap.20201201-060502 vg0    swi-a-s---  10.00g      ss-wp  6.81
  ss-wp.snap.20201202-060501 vg0    swi-a-s---  10.00g      ss-wp  5.82
  ss-wp.snap.20201203-060502 vg0    swi-a-s---  10.00g      ss-wp  3.22
```

Here the LVM system keep track of changed disk blocks using snapshots.
The Data% column indicates the amount of the snapshot volume consumed
by changed blocks.  The above snapshots done daily (24 hours apart)
for a period of 9 days.

If you notice, the amount of data changed in 9 days is 11% of 10G, or
approx 1.1 Gig over the entire disk (20G) - or approx 5.5%.  More
recent snapshots show less changes (as expected), looking at the above
changes from 20201201 (Dec 1 2020) to 20201202 (Dec 2 2020) is the 
difference between 6.81% and 5.82% = 1% of 10G = 0.5% of the hard drive.
To put it in perspective, in the 24 hour period from Dec 1 to Dec 2, the
amount of data changed in the hard drive is 0.5%, or about 50 Meg.
So it appears wasteful to backup a 20G hard drive image, when backing up
just the change (50 Meg) is adequate.  Storing just the differences
is called "differential backup method".

So if we want to do a daily backup of the hard drive image on Dec 1 and
Dec 2 - we could do the brute force method - store the disk image. The
storage requirement would be 20G + 20G = 40G.  If we do the differential
method the storage will be 20G + 50M, or a savings of 19.95 Gig.  Additional
days save even more.

Using the above differential method and with the above example we can estimate

```
20201126 "full" = 20G
20201127 changes between 20201126 and 20201127: 11.02 - 10.65 = .37% of 10G = 37 Meg
20201128 changes between 20201126 and 20201128 = 61 Meg
20201129 changes between 20201126 and 20201129 = 170 Meg
...
20201203 changes between 20201126 and 20201203 = 780 Meg
```

If you calculate the storage requirement we get 22.29 Gig for 9 days.
Compare this to 9 days of full image backups = 180 Gig.

We can easily rebuild the entire disk image for any day within the 9
days above by taking the "full" image, and applying only the changes
through to the date, example: we can recreate the image of 20201129
by using the full image taken 20201126 and applying a the 170 Meg
"patch" for 20201129.  Basically the rebuild strategy is to use
the most recent full image and applying the patch between the time
the full image was taken and the time of the patch.

These 3 programs help with creating these backup sets.  We analyze the
disk images looking for changes, then generate a patchfile consisting only
of the changed disk blocks.  These patchfiles can be stored on a backup
server and consume a tiny fraction of the full disk image (for most worksets).
A program is also included that will apply these patchsets to rebuild any
image on any date.

The programs here break up the disk image into 4K blocks (this is a typical
size of a disk block on modern hard drives).  Each block is hashed using md5
to produce a hashset for the image.

Side note: Yes I know md5 is compromised for high security applications, however is
still very adequate for examining disk blocks for changes.

These hashsets are compared to check for disk blocks that changed.  Only
the changed disk blocks are copied to a patchset, which can be saved.
When we want to rebuild an image we apply the patchset against an image to
regenerate the desired image.


* **blk_hash** - this program generates a hashset file for an image, use this when
 storing a "full" backup image.  We will call this image the origin image and
the hashset the origin hashset.

* **blk_genpatch** - this program compares a new image against an origin hashset and
 creates a patchset file consisting only of the changed disk blocks.

* **blk_patch** - this program patches an origin image with a patchset to regenerate
 the image which corresponds to the patchset

# Other programs considered

bdsync ( https://github.com/rolffokkens/bdsync ) - works great, however 
 its requires the original image to generate a patchfile.  It doesnt cache
 the hashset, which basically means that the initial full image must be 
 re-read to generate a patchset.  2 points against:
> * you cant compress the full sized image, it has to be available uncompressed
> * you need to re-read and re-hash the entire full sized image each time you generate a patchset - doubling the amount of reads, and it adds up when dealing with images that are hundreds of Gig in size.

But dont get me wrong, bdsync is an excellent piece of software and 
is my inspiration for this project.  And I believe the original goal of bdsync
is for copying images, which it excels at.
Kudos to its creator Rolf Fokkens.

rsync - great for creating diffsets of filesystems, but terrible at 
 creating diffsets for very large files.  And basically unuseable for
 block devices.

several other projects that do similar things, but they all fell short
on one criteria or another.

* https://vog.github.io/bscp/ - I found it to be too slow.

* an interesting project lvmsync ( https://github.com/mpalmer/lvmsync )
uses LVM metadata
to get the blocks that changed instead of scanning the entire image and
hashing it.  Could be useful for a "incremental" backup strategy.
Unfortunately it requires a version of ruby I wasnt able to install
on some older servers.

This set of programs written in simple C.  needs openssl libraries.  Code
works with the old API (openssl 1.0 series) and newer API (openssl 1.1.0+).

# putting it all together

Suppose you have a block device which represents a hard drive.  Such a 
scenario may be a LVM which is on a host server, which is passed into
a guest VM running under libvirt.  You want to backup this hard drive
on a daily schedule and keep 30 days worth of backups - in case you need
to roll back.  For this example lets assume this hard drive is 10 Gig.

On day 1 you want to backup a full image of the hard drive as a 
starting point.  A typical backup flow may look sort-of like this:

* snapshot the LVM
* dd if=path_to_snapshot | ssh backup_server "cat - >day01_full_image"
* delete the snapshot

Throw in a little compression (to save on network transfer and some storage)

* snapshot the LVM
* dd if=path_to_snapshot | gip -c | ssh backup_server "cat - >day01_full_image.gz"
* delete the snapshot

We put in the hashset generator into the pipeline to create a hashfile that
we can refer to in future backup runs:

* snapshot the LVM
* dd if=path_to_snapshot | blk_hash -o day01.hash | gip -c | ssh backup_server "cat - >day01_full_image.gz"
* delete the snapshot

Ok the above generates a hash file day01.hash which conains hash signatures for
each block of the day01 full image.  We keep this file around.  On the
backup server we have the full image saved (10 Gig uncompressed, about 2-5 Gig compressed)

On day 2, we will backup only the differences, which typically are a tiny
fraction of the entire disk image.  We generate a patchset by comparing the
current image block hashes to the hashfile from day 1 we saved.

* snapshot the LVM
* blk_genpatch -i path_to_snapshot -h day01.hash -o - | gip -c | ssh backup_server "cat - >day02.patch.gz"
* delete the snapshot

The above saves just the patchset (compressed) on the backup server.

On days 3 thru 30 repeat the procedure like day 2.

The result is that after 30 days, you will have on the backup server
one large full image from day 1, and 29 small patchsets for days 2 thru 30.
Backing up a typical harddrive using this method 
can save a lot of backup storage.

Now lets take a look at a restore operation.  Suppose we discover on 
day 29 that our VM isnt running correctly, and further analysis
shows it hasnt been running correctly since day 25.
We would like to restore the disk image to a snapshot of day 24 to fix
the problem.

So we grab a copy of the full image on day 1, and the patchset from 
day 24 and do the restore by makeing a new block device using the 
full image, then patching in the day 24 patchset to that block device.
The result is a block device with a full day 24 image.  Such a workflow
may look like this:

* lvcreate -L 10G ... (to make a new block device of 10G - the size of the image)
* gunzip -c < day01_full_image.gz | dd of=path_to_new_device
* gunzip -c < day24.patch.gz | blk_patch -i path_to_new_device -p -

The result of the above will place the day24 disk image in the new device.
The device can be dropped into a VM and booted.

# Rolling backups

If you want to do rolling backups, you might want to do something like
create a new full image every 30 days, and rebase the patchsets on the
most recent full image on all other days.  On the backup server if you
want to delete older backups, you can safely delete the patchsets that
are older than your restore window.  However you cant delete the full 
image that is referred to by any patchset within the restore window - thats
because you still need that full image to rebuild a copy of the image
snapshot for that day of interest.

You can safely delete any full image once all patchsets that refer to it
are deleted or are out of the restore window.

Lets illustrate, if we do a 30 day
backup schedule with 1 full and 29 patchsets.  We will have this on the
backup server after 90 days if we dont delete anything:

```
01 FULL 01
02 patchset 01-02
03 patchset 01-03
... (26 more patchsets)
29 patchset 01-29
30 FULL 30
31 patchset 30-31
32 patchset 30-32
... (a bunch more patchsets)
43 patchset 30-43
44 patchset 30-44
45 patchset 30-45
... (a bunch more patchsets)
59 patchset 30-59
60 FULL 60
61 patchset 60-61
62 patchset 60-62
... (a bunch more patchsets)
89 patchset 60-90
90 FULL 90
```

Suppose we have a 45 day restore window.  On day 90 we would like to
be able to restore back to day 45, but we dont need to restore back
anything further.  So we can safely delete all patchsets older than day 45:

```
01 FULL 01
02 patchset 01-02        <-- this can be deleted
03 patchset 01-03        <-- this can be deleted
... (26 more patchsets)        <-- this can be deleted
29 patchset 01-29        <-- this can be deleted
30 FULL 30
31 patchset 30-31        <-- this can be deleted
32 patchset 30-32        <-- this can be deleted
... (a bunch more patchsets)        <-- this can be deleted
43 patchset 30-43        <-- this can be deleted
44 patchset 30-44        <-- this can be deleted
45 patchset 30-45
... (a bunch more patchsets)
59 patchset 30-59
60 FULL 60
61 patchset 60-61
62 patchset 60-62
... (a bunch more patchsets)
89 patchset 60-90
90 FULL 90
```

This gives:

```
01 FULL 01
30 FULL 30
45 patchset 30-45
... (a bunch more patchsets)
59 patchset 30-59
60 FULL 60
61 patchset 60-61
62 patchset 60-62
... (a bunch more patchsets)
89 patchset 60-90
90 FULL 90
```

Now we can safely delete any *UNREFERENCED* FULL images
that are older than day 45 too.

```
01 FULL 01             <-- this is unreferenced and can be deleted
30 FULL 30             <-- this cant be deleted, its still referenced
45 patchset 30-45
59 patchset 30-59
60 FULL 60             <-- this cant be deleted, more recent than day 45
61 patchset 60-61
62 patchset 60-62
... (a bunch more patchsets)
89 patchset 60-90
90 FULL 90             <-- this cant be deleted, more recent than day 45
```

This now gives the minimum set of files we need to keep to be able to
guarantee our restore window.

```
30 FULL 30
45 patchset 30-45
... (a bunch more patchsets)
59 patchset 30-59
60 FULL 60
61 patchset 60-61
62 patchset 60-62
... (a bunch more patchsets)
89 patchset 60-90
90 FULL 90
```


# Design considerations

* We break the image into 4K blocks, which is typical of a modern disk block
device.

* blk_hash is designed as a transparant pipe - which makes it easy to insert
into the pipeline of your backups.

* you will need to keep the hashfile around for as many days as you need to
refer to the initial full image it was based on to continue to generate
patchsets as diffs off that image.  You dont need to access the original
image, only the hashset.

* patchsets for typical read-mostly workloads are small; ie. single digit percentages of the full image.  This backup strategy works well to save
storage space.  However 
certain write intensive workloads might do large amounts of disk updates.
In these cases the patchsets can become large over time.  A rule of thumb
I like to use is to create a new full image (and corresponding hashfile) 
when the patchset gets to about 10% of the size of the image.
The program blk_genpatch has a -a flag to assist with management. 
Specify -a abort_percentage (example: -a 10) and the blk_genpatch program
will give up and abort if the size of the patchset exceeds that percentage
of the image.  It will exit with exit code 2.  You can design your backup
script to detect this, discard the partially generated patchset, and to start 
over by generating a full image instead.

* some types of heavily memory cached activities, such as database, might
not give a consistent disk image if snapshotted when its busy.  Which leads
to a disk image that doesnt accurately represent the state of the database
at the time of the snapshot.  You might want to employ additional backup
strategies in these situations IN ADDITION to the disk image backups.

* If the image is resized, then the old backups will no longer represent the
actual new image size.  Recommend generating a full image backup and a 
new hashset right away.  In fact these programs will not work correctly
if there are unmatched sizes between the image, hashset, and patchset.
Some situations are detected and error or warning messages issued.

* I found that for the above sample backup pipelines, these settings
work well (on a 64-bit linux machine with Intel CPU):
> * gzip level 2 (gzip -2) for compression gives good speed (mostly from less
traffic over the network), and the
size of the compressed file isnt that much larger than the default level 6.  You might want to use higher levels or other compression programs if you want to reduce storage space further at the expense of possibly slower backups and/or heavier CPU usage.  Experiment if you're concerned.
> * Got extra CPU on the backup server?   An interesting storage optimization may involve a much costlier 
compressor (examples: xz, lzma, etc...) as a post-process job on the backup server to further compress
the backup files.
> * a blocksize of 64K (65536) works well for dd in the pipeline.  In fact some parts of the code use 64K blocking as this seems pretty good for speed.  example:
```
dd if=blk_device bs=65536 | blk_hash ...
```
> * specifying a faster cipher (if available) for ssh may speed things a bit.  Example: -c blowfish.  However keep in mind modern ssh has removed some of the faster ciphers due to security concerns.  You may want to consider a non-secure transport (eg: rsh, ssh -c none) if your network to the backup server is otherwise secured, example: a private switched network isolated from the internet.

* these programs are designed to work with both regular file images as well as block devices.  You can for example, use these programs to generate patchsets for large files (multiple gigabytes and up) which have small amounts of changes.  But be aware these programs are designed to be used on these files only if they dont change in size.

* everything is designed with 64 bits - so it can scale up to files of block devices of hundreds of terabytes and more.  The basic algorithm scales linearly not logarithmically so do expect longer runtimes.

* accidental loss of the hashset representing the most recent full image will deter
generating any patchsets.  However it isnt too hard to regnerate the
hashset from the image on the backup server.
Example:
```
gunzip -c < day01_full_image.gz | blk_hash -o day01.hash >/dev/null 
```
* You may also consider that when backing up full images, backup the hashset as well.

# File formats

## hashset file

The hashfile is simply a list of md5 hashes for each block of the image.
A block is 4096 bytes, the size of an md5 hash is 16 bytes.  The size of the hashfile
is exactly 16 bytes times the number of blocks in the image.  If the image contains a
partial block at the end, it will be zero padded for the md5 computation.
So we expect the hashfile to be approximately 1/256 the size of the original image.

## patchset file

A patchfile consists of zero or more containers.

Each container contains an offset block followed by one or more patch
blocks.  Each block is exactly 4K.

The offset block holds offsets, where each offset is 8-bytes.  Up to 512
offsets will fit into this 4K block.  Each offset is a 64-bit offset value
(unsigned long long) representing the offset
in the image of a patch block.  If there are less than 512 offsets then
the block is zero padded up to the full 4K size.

The remainder of the container holds a sequence of patch blocks.  Each patch
block contains the 4K block of an image at the corresponding offset in
the offset block in sequential order.  Since there are at most 512 offsets
in the offset block, there will be a maximum of 512 patch blocks in a
container.
Thus the maximum size of a container
is 513 blocks (1 offset block + 512 patch blocks), which in real terms
is slighly more than 2 megabytes.

Since each offset is a 8 byte unsigned integer representing a byte offset into
the image for a patch block, the maximum size of an image supported is
2^64 or approx 18 million terabytes.  This ought to cover most typical
applications for the forseeable future.

Offsets are placed into the offset block in sequential order matching the
order in which patch blocks are added to the container.  Since we are using
4K blocks we would expect each offset to be divisible by 4096, although there
is no validation in the programs.

If there is a patch to the last (possibly partial) block of an image, only
as many bytes of the patch required to patch the partial block will be used.
The remainder of the patch block (which is likely only to contain zeros
anyways) will be ignored.

If an offset is beyond the end of the image it will be ignored (possibly 
flagged as an error).

By normalizing patch blocks and patchfiles to 4K size we may be able to
get some performance gains 

* aligned blocks for data transfer to and from disk
* write and read entire logical block 4K (may help with SSDs and newer hard drives)


# patchfile examples

In the degenerate case that the image and updated image are identical, there
will be 0 patch blocks and 0 offsets - therefore the patchfile will have
0 containers.  The patchfile will be zero sized.

In the case where we have only 1 block changed, the patchfile will contain
1 offset and 1 patch block.  Since we have to zero pad the offset block to a
full 4096 bytes the container will have 2 blocks - total 8192 bytes.

If a patchfile requires more than 512 patch blocks, then additional containers
are needed.  Containers are written to the patchfile one after another.  

As an example, in the case where there is 513 changed blocks, the first 512 patch blocks 
will fit into the first container, along with an offset block consisting
of 512 offsets; thus the first container will ba 513 blocks.

Patch #513 is placed
in the 2nd container, along with its offset in the offset block (zero padded).
The entire patchfile will be 515 blocks:

```
container1 = 513 blocks
container2 = 2 blocks

container1 = offset block with 512 entries + 512 patch blocks
container2 = offset block with 1 entry and 511 zeros + 1 patch block

patchfile = container1 + container2 = 515 blocks
```

# Further reading

There is a README file for each of the programs if you want to read further.
There are test cases and performance studies to bore you to no end.
Enjoy.

