/* Copyright (c) 2020 David Lai davidlai@webstandard.com see LICENSE */

#include "blk_utils.h"

#include "blk_md5.h"

#include "blk_imageutils.h"

#include "blk_patchfileutils.h"

// a structure to make it easier to compare md5 hashes which are
// 16 bytes in length
typedef   union {
    unsigned char hash[16];
    struct {
      uint64_t a;
      uint64_t b;
    } hash128;
  } md5hash;

/* we buffer hashfile in 64K chunks too */
#define NUM_HASHES_IN_BUFF  (65536/sizeof(md5hash))
md5hash hashbuf[NUM_HASHES_IN_BUFF]; /* read in hashes in chunks too */

offset_t hashfile_size;



int open_hashfile( char *filename, offset_t *file_size ) 
{
 /* open the hashfile for reading, set the size in file_size, and return the fd */
 /* handle block devices and regular files */
  struct stat stat1;
  int fd;
  if (stat(filename,&stat1)==-1) {
    die(strerror(errno));
  }
  if (! (S_ISREG(stat1.st_mode) ) ) {
    die("cant handle this hash file");
  }
  fd=open(filename,O_RDONLY);
  if (fd==-1) {
    die(strerror(errno));
  }
  if (S_ISREG(stat1.st_mode)) {
    *file_size = stat1.st_size;
  }
  return(fd);
}

int hashfile_fd;


md5hash *next_hash(void)
{
/* return a pointer to the next hash */
/* if there are no more hashes then a NULL pointer is returned */
 static int hashes_processed_in_buffer=0;
 static int hashes_remaining_in_buffer=0;
 int i,j,nread,extra_bytes;

 if (hashes_remaining_in_buffer==0) {
   /* fill buffer with the next chunk of hash */
   hashes_processed_in_buffer=0;
   nread=read(hashfile_fd,hashbuf,sizeof(md5hash)*NUM_HASHES_IN_BUFF);
   if (nread == -1) { die(strerror(errno)); }
   if (nread == 0 ) { return NULL; } /* EOF */
   /* the number of hashes read */
   hashes_remaining_in_buffer=(nread/sizeof(md5hash));
 }
 hashes_remaining_in_buffer--;
 return(&hashbuf[(hashes_processed_in_buffer++)]);
}

// FILE *patchfile;

//  void open_patchfile( char *patch_file_name ) 
//  {
//   // open patchfile for output and set the variable patchfile
//   // if patch_file_name is NULL, it wont be opened and patchfile will be set
//   // to NULL
//   // if patch_file_name is "-", it sets patchfile=stdout
//   if (patch_file_name) {
//     if (strcmp(patch_file_name,"-")==0) {
//        patchfile=stdout;
//     } else {
//        patchfile=fopen(patch_file_name,"w");
//        if (!patchfile) {
//          die("Cant write patchfile");
//        }
//     }
//   } else {
//     patchfile=NULL;
//   }
//  }

//  void close_patchfile( void )
//  {
//     if (patchfile == NULL) { return; }
//     if (patchfile == stdout) { return; } /* dont close stdout */
//     fclose(patchfile);
//  }

// void add_patchblock( offset_t offset, char *datablock )
// {
//   /* add a patch block to the output */
//   /* offset = offset in image for the patch */
//   /* datablock points to the replacement 4K block of data */
//   /* if datablock=NULL then this call is used to end the patchfile */
//   static int patches_in_container=0;
//   int i, j;
//   if (datablock!=NULL) {
//     /* add this patch to the container */
//     offset_block[patches_in_container]=offset;
//     memcpy(&patchblock[BLOCKSIZE * patches_in_container],datablock,BLOCKSIZE);
//     patches_in_container++;
//   }
//   if (datablock==NULL && patches_in_container!=0) {
//     /* end the patchfile */
//     /* zero pad the offset block if necessary */
//     for(i=patches_in_container; i< NUM_PATCHES_PER_CONTAINER; i++) {
//          offset_block[i]=0;
//     }
//   }
//   if (patches_in_container==NUM_PATCHES_PER_CONTAINER ||
//       (datablock==NULL && patches_in_container!=0)) {
//       /* write out the current container */
//       if (NUM_PATCHES_PER_CONTAINER != fwrite(offset_block, sizeof(offset_t), NUM_PATCHES_PER_CONTAINER, patchfile)) {
//          die("error writing patchfile");
//       }
//       if (patches_in_container != fwrite(patchblock,BLOCKSIZE,patches_in_container, patchfile)) {
//          die("error writing patchfile");
//       }
//       patches_in_container=0;
//    }
//    if (datablock==NULL) { close_patchfile(); }
// }
// 


void usage(char *argv[])
{
fprintf(stderr,"Usage: %s -i image -h hashfile [-o patchfile] [-v] [-a percentage] [-z]\n", argv[0]);
fprintf(stderr," reads image and compares its block hashes against the\n");
fprintf(stderr," hashes in hashfile.  It will output a patchfile or\n");
fprintf(stderr," stdout if patchfile is '-'.  If -o  is not specified\n");
fprintf(stderr," then no patchfile is produced.  -v reports the statistics\n");
fprintf(stderr," to stderr which include how many blocks changed.\n");
fprintf(stderr," -a specifies a percentage (simple integer from 1-99) for aborting early.  If\n");
fprintf(stderr," the percentage of changed blocks\n");
fprintf(stderr," exceeds this percentage, the program will abort early\n");
fprintf(stderr," and exit with status 2.\n");
fprintf(stderr," -z = ignore size differences, otherwise unmatched \n");
fprintf(stderr," hashfile and image size will exit with status 2.\n");
exit(1);
}

int main(int argc, char *argv[])
{
 int i;
 char *image_file_name=NULL;
 char *hash_file_name=NULL;
 char *patch_file_name=NULL;
 int verbose=0;
 int abort_percent=0;
 int allow_size_difference=0;
 uint64_t num_hashes,size_diff;
 md5hash image_hash;
 // init the workspace
 offset_t current_offset;   /* current offset in image file being processed */
 current_offset=0;
 uint64_t  abort_threshold = UINT64_MAX;
 uint64_t   num_patches = 0;
 char *image_block;
 md5hash *old_hash_p;
 offset_t image_size;

 for (i=1; i<argc; i++) {
  if (strcmp(argv[i],"-i")==0) {
     i++;
     if (!(image_file_name=argv[i])) { usage(argv); }
     continue;
  }
  if (strcmp(argv[i],"-h")==0) {
     i++;
     if (!(hash_file_name=argv[i])) { usage(argv); }
     continue;
  }
  if (strcmp(argv[i],"-o")==0) {
     i++;
     if (!(patch_file_name=argv[i])) { usage(argv); }
     continue;
  }
  if (strcmp(argv[i],"-v")==0) {
     verbose++;
     continue;
  }
  if (strcmp(argv[i],"-z")==0) {
     allow_size_difference++;
     continue;
  }
  if (strcmp(argv[i],"-a")==0) {
     i++;
     if(sscanf(argv[i],"%d",&abort_percent) != 1) { usage(argv); }
     continue;
  }
  usage(argv);
 }
 if (!hash_file_name || !image_file_name) { usage(argv); }
 if (abort_percent < 0 || abort_percent>99) { usage(argv); }
 
 // open the image file and get the size of the image file
 image_size = open_image(image_file_name,0);

 // open the hashfile and get its size
 hashfile_fd=open_hashfile(hash_file_name, &hashfile_size);
 
 // calculate the expected size of the hashfile and compare
 num_hashes=(image_size/BLOCKSIZE);
 if (image_size % BLOCKSIZE) { num_hashes++; } /* partial block at end */
 if (hashfile_size < num_hashes * sizeof(md5hash)) {
    size_diff=image_size - ((hashfile_size / sizeof(md5hash))*BLOCKSIZE);
    fprintf(stderr,"Image file larger than expected by %llu bytes\n",size_diff);
    if (!allow_size_difference) { die("Abort: Size Mismatch"); }
 } else if (hashfile_size >  num_hashes * sizeof(md5hash)) {
    size_diff=((hashfile_size/ sizeof(md5hash))*BLOCKSIZE)-image_size;
    fprintf(stderr,"Image file smaller than expected by %llu bytes\n",size_diff);
    if (!allow_size_difference) { die("Abort: Size Mismatch"); }
 }
 
 

 if (abort_percent) {
    /* number of patch blocks threshold */
    abort_threshold = ( num_hashes * abort_percent / 100 ) + 1;
 }

 /* initialize the hashing function */
 md5_begin();

 /* open the patchfile */
 open_patchfile_write(patch_file_name);
 
 // process loop
 while(1) {
   image_block = next_image_block(NULL);
   old_hash_p=next_hash();
   if (image_block == NULL || old_hash_p == NULL ) {
     /* done */
     add_patchblock(0,NULL); /* close the patchfile at end */
     md5_end();  /* cleanup the hashing function */
     if (verbose) {
       fprintf(stderr,"Wrote %llu patch blocks / %llu total blocks = %d percent\n", num_patches, num_hashes, (num_hashes==0)?0:(int)((num_patches * 100)/num_hashes));
     }
     exit(0);
   }
   md5_block(image_block,BLOCKSIZE,&(image_hash.hash[0]));
   /* compare current hash to old hash */
   if (old_hash_p->hash128.a != image_hash.hash128.a ||
       old_hash_p->hash128.b != image_hash.hash128.b ) {
       /* mismatch, add to the patch */
       num_patches++;
       if (num_patches > abort_threshold) {
	/* abort, patchfile too large */
        md5_end();
        fprintf(stderr,"Abort: threshold %llu patches exceeded\n",abort_threshold);
	exit(2);
       }
       add_patchblock(current_offset, image_block);
   }
   current_offset+=BLOCKSIZE;
 }

}

