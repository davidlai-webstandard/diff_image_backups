.PHONY: all clean

all: blk_hash blk_genpatch blk_patch

all-dbg: blk_hash-dbg blk_genpatch-dbg blk_patch-dbg

blk_hash: blk_hash.c blk_md5.o blk_utils.o blk_imageutils.o
	cc -O2 -o blk_hash blk_hash.c blk_md5.o blk_utils.o blk_imageutils.o -lssl -lcrypto

blk_hash-dbg: blk_hash.c blk_md5-dbg.o blk_utils-dbg.o blk_imageutils-dbg.o
	cc -O2 -o blk_hash blk_hash.c blk_md5-dbg.o blk_utils-dbg.o blk_imageutils-dbg.o -lssl -lcrypto

blk_genpatch: blk_genpatch.c blk_md5.o blk_utils.o blk_patchfileutils.o blk_imageutils.o
	cc -O2 -o blk_genpatch blk_genpatch.c blk_md5.o blk_utils.o blk_patchfileutils.o blk_imageutils.o -lssl -lcrypto

blk_genpatch-dbg: blk_genpatch.c blk_md5-dbg.o blk_utils-dbg.o blk_patchfileutils-dbg.o blk_imageutils-dbg.o 
	cc -g -o blk_genpatch-dbg blk_genpatch.c blk_md5-dbg.o blk_utils-dbg.o blk_patchfileutils-dbg.o blk_imageutils-dbg.o -lssl -lcrypto

blk_patch: blk_patch.c blk_md5.o blk_utils.o blk_patchfileutils.o blk_imageutils.o
	cc -O2 -o blk_patch blk_patch.c blk_md5.o blk_utils.o blk_patchfileutils.o blk_imageutils.o -lssl -lcrypto

blk_patch-dbg: blk_patch.c blk_md5-dbg.o blk_utils-dbg.o blk_patchfileutils-dbg.o blk_imageutils-dbg.o
	cc -g -o blk_patch-dbg blk_patch.c blk_md5-dbg.o blk_utils-dbg.o blk_patchfileutils-dbg.o blk_imageutils-dbg.o -lssl -lcrypto

blk_md5.o: blk_md5.c blk_md5.h blk_utils.h
	cc -O2 -c blk_md5.c

blk_md5-dbg.o: blk_md5.c blk_md5.h blk_utils.h
	cc -g -c blk_md5.c -o blk_md5-dbg.o

blk_utils.o: blk_utils.c blk_utils.h
	cc -O2 -c blk_utils.c

blk_utils-dbg.o: blk_utils.c blk_utils.h
	cc -g -c blk_utils.c -o blk_utils-dbg.o

blk_patchfileutils.o: blk_patchfileutils.c blk_patchfileutils.h blk_utils.h
	cc -O2 -c blk_patchfileutils.c

blk_patchfileutils-dbg.o: blk_patchfileutils.c blk_patchfileutils.h blk_utils.h
	cc -g -c blk_patchfileutils.c -o blk_patchfileutils-dbg.o

blk_imageutils.o: blk_imageutils.c blk_imageutils.h blk_utils.h
	cc -O2 -c blk_imageutils.c

blk_imageutils-dbg.o: blk_imageutils.c blk_imageutils.h blk_utils.h
	cc -g -c blk_imageutils.c -o blk_imageutils-dbg.o


clean:
	rm -f blk_imageutils.o  blk_md5.o  blk_patchfileutils.o  blk_utils.o
	rm -f blk_imageutils-dbg.o blk_md5-dbg.o blk_patchfileutils-dbg.o
	rm -f blk_utils-dbg.o
	rm -f blk_hash blk_genpatch blk_patch
	rm -f blk_hash-dbg blk_genpatch-dbg blk_patch-dbg
	rm -f hash_cmp

hash_cmp: hash_cmp.c
	cc -O2 -o hash_cmp hash_cmp.c
