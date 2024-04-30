#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MD5_HASH_SIZE 16

typedef   union {
    unsigned char hash[MD5_HASH_SIZE];
    struct {
      uint64_t a;
      uint64_t b;
    } hash128;
  } md5hash;


#define NUM_HASHES_IN_BUFF  (65536/sizeof(md5hash))
md5hash hashbuf1[NUM_HASHES_IN_BUFF];
md5hash hashbuf2[NUM_HASHES_IN_BUFF];

int usage( char *msg ) {
   fprintf(stderr,"Error: %s\n",msg);
   fprintf(stderr,"\n Usage:\n");
   fprintf(stderr,"   hash_cmp file1 file2\n");
   fprintf(stderr,"     where file1 and hashfile2 are the 2 hash files to compare\n");
   fprintf(stderr,"     its assumed they are the same size and contain md5 hashes (16 bytes per hash)\n");
  exit(2);
}


int main(int argc, char *argv[])
{
 char *hf1, *hf2;
 int f1, f2;
 int nf1, nf2;
 uint64_t blocknum=0;
 uint64_t num_mismatch=0;
 int i;
 hf1=argv[1];
 hf2=argv[2];
 if (!hf1 || !hf2) { usage("missing filenames"); }
 if ((f1=open(hf1,O_RDONLY))==-1) { usage("cant read hashfile1"); }
 if ((f2=open(hf2,O_RDONLY))==-1) { usage("cant read hashfile2"); }

 while (1) {
    nf1=read(f1,hashbuf1,sizeof(hashbuf1));
    nf2=read(f2,hashbuf2,sizeof(hashbuf2));
    if (nf1 != nf2 ) { usage("hash files size differs"); }
    if (nf1 == 0 ) { fprintf(stderr,"num_mismatch %llu\n",num_mismatch); exit(0); } // done
    if (nf1 == -1) { usage("error reading hashfile"); }
    for (i=0; i<(nf1/sizeof(md5hash)); i++ ) {
      if( hashbuf1[i].hash128.a != hashbuf2[i].hash128.a ||
          hashbuf1[i].hash128.b != hashbuf2[i].hash128.b ) {
           printf("%llu\n",blocknum);
           num_mismatch++;
      }
      blocknum++;
    }
 }
}
