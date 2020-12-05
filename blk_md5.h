/* Copyright (c) 2020 David Lai davidlai@webstandard.com see LICENSE */

#include <openssl/evp.h>

extern void md5_begin(void);

extern void md5_end(void);

extern unsigned int md5_block( unsigned char *, size_t, unsigned char *); 

#define MD5_HASH_SIZE  16
#define NUM_MD5_64K    (65536 / MD5_HASH_SIZE)  // number of hashes in 64K

