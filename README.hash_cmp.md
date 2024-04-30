# hash_cmp - utility program that compares 2 hash files

This program is used to compare 2 hash files, presumably for the same
image, and list out which blocks differ.  It also prints out a summary of
the number of blocks that differ.

While this program isnt needed by a backup process, it may be used to 
diagnose hash files.  Similar functionality, comparing image to hash file,
can be had with the -b flag to blk_genpatch

# Usage

* hash_cmp hashfile1 hashfile2 > list_of_changed_blocks



