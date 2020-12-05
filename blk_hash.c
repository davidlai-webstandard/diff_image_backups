/* Copyright (c) 2020 David Lai davidlai@webstandard.com see LICENSE */

#include "blk_utils.h"
#include "blk_imageutils.h"
#include "blk_md5.h"

// #define NUM_BLOCKS  256  /* number of blocks in buffer 1 Meg buffer */
// #define NUM_BLOCKS  16  /* number of blocks in buffer 64K buffer */

// unsigned char inbuff[NUM_BLOCKS_64K * BLOCKSIZE]; /* read input in chunks of NUM_BLOCKS */
unsigned char hashbuf[MD5_HASH_SIZE * NUM_MD5_64K]; /* buffer hashes */

void usage(char *argv[])
{
fprintf(stderr,"Usage: %s -o hashfile\n", argv[0]);
fprintf(stderr," place hashes in hashfile\n");
fprintf(stderr,"Usage: %s -n\n", argv[0]);
fprintf(stderr," transparent pipe, no hashing\n");
fprintf(stderr,"Usage: %s -o /dev/null\n", argv[0]);
fprintf(stderr," transparent pipe, do hashing but throw away results\n");
       exit(1);
}

int main(int argc, char *argv[])
{


FILE *hashfile;
int nread; // size of block, may be smaller than BLOCKSIZE for partial block
int hash_counter;
unsigned char *p_hashbuf;
int do_hashing=0;
char *hashfile_name;
char *p_inbuff;


if(argc == 2 && strcmp(argv[1],"-n")==0) {
   hashfile=NULL;   
} else if(argc != 3 || strcmp(argv[1],"-o")) {
       usage(argv);
} else {
       hashfile_name=argv[2];
}

if (hashfile_name) {
 /* attempt to open the hashfile */
 hashfile=fopen(argv[2],"w");
 if (!hashfile) {
  fprintf(stderr,"Cant write file %s\n", argv[2]);
  usage(argv);
 }
 do_hashing=1;
}

md5_begin();
open_image("-",0);  // special case open stdin, ignore image size

p_hashbuf=&hashbuf[0];
hash_counter=0;
while(p_inbuff = next_image_block(&nread)) {
 if (do_hashing) {
  /* hash each 4K block read */
   p_hashbuf += md5_block(p_inbuff,BLOCKSIZE,p_hashbuf);
   hash_counter++;
   if (hash_counter == NUM_MD5_64K) {
      /* write out the buffer */
      fwrite(&hashbuf, sizeof(hashbuf[0]), (p_hashbuf-hashbuf), hashfile);
      hash_counter=0;
      p_hashbuf=&hashbuf[0];
   }
 }
 fwrite(p_inbuff, 1 ,nread, stdout);

} /* while */


/* now close out */
if (do_hashing) {
  // write out any collected hash
  if (hash_counter) {
     fwrite(&hashbuf, sizeof(hashbuf[0]), (p_hashbuf-hashbuf), hashfile);
  }
  fclose(hashfile);
}

md5_end();

exit(0);

}
