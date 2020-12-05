/* Copyright (c) 2020 David Lai davidlai@webstandard.com see LICENSE */

#include "blk_utils.h"
#include "blk_md5.h"


#if OPENSSL_VERSION_NUMBER >= 0x10100000L
// code for version 1.1.0 or greater
// openssl version 1.1.0+ changed the way EVP_MD_CTX is defined
#define USE_EVP_MD_CTX_NEW_FREE
#else
// code for 1.0.x or lower
// EVP_MD_CTX is a standard data type
#endif



#ifdef USE_EVP_MD_CTX_NEW_FREE
EVP_MD_CTX *mdctx;
#define POINTER_TO_mdctx mdctx
#else
EVP_MD_CTX mdctx;
#define POINTER_TO_mdctx &mdctx
#endif

void md5_begin(void) {
#ifdef USE_EVP_MD_CTX_NEW_FREE
mdctx = EVP_MD_CTX_new();
#endif

EVP_MD_CTX_init(POINTER_TO_mdctx);
}

void md5_end(void) {
#ifdef USE_EVP_MD_CTX_NEW_FREE
EVP_MD_CTX_free(mdctx);
#endif
}

/* returns the number of bytes of hash computed */
unsigned int md5_block(
  unsigned char *data,	/* pointer to the data to be hashed */
  size_t size, 		/* size of the data */
  unsigned char *p_hash	/* pointer to where the hash will be put */
 ) 
{
   unsigned int rtrn;
   EVP_DigestInit_ex(POINTER_TO_mdctx, EVP_md5(), NULL);
   EVP_DigestUpdate(POINTER_TO_mdctx, data, size );
   EVP_DigestFinal_ex(POINTER_TO_mdctx, p_hash, &rtrn);
   return rtrn;
}

