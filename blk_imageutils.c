/* Copyright (c) 2020 David Lai davidlai@webstandard.com see LICENSE */

// assorted utilities for handling image files/devices

#include "blk_utils.h"

#include "blk_imageutils.h"

char image_buff[NUM_BLOCKS_64K * BLOCKSIZE]; /* read image in chunks of NUM_BLOCKS */

int image_fd;

offset_t open_image( char *filename, int write_flag ) 
{
 /* open the image for reading, set image_fd, return size */
 /* handle block devices and regular files */
 /* if write_flag is nonzero, then the file is open for read/write */
 /*  note: the file must already exist, this wont create a file */
  struct stat stat1;
  int fd;
  offset_t rtrn;
  if (strcmp(filename,"-")==0) {
     /* special case read stream stdin, ignore size */
     image_fd=STDIN_FILENO;
     return 0;
  }
  if (stat(filename,&stat1)==-1) {
    die(strerror(errno));
  }
  if (! (S_ISREG(stat1.st_mode) || S_ISBLK(stat1.st_mode))) {
    die("cant handle this image file");
  }
  fd=open(filename,(write_flag?O_RDWR : O_RDONLY));
  if (fd==-1) {
    die(strerror(errno));
  }
  if (S_ISBLK(stat1.st_mode)) {
    if (ioctl(fd,BLKGETSIZE64,&rtrn)==-1) {
      die(strerror(errno));
    }
  }
  if (S_ISREG(stat1.st_mode)) {
    rtrn = stat1.st_size;
  }
  image_fd = fd;
  return rtrn;
}



char *next_image_block(int *p_size)
{
/* return a pointer to the next image block (4096 bytes) */
/* if its a partial block then it will be zero padded */
/* if there are no more blocks then a NULL pointer is returned */
/* if p_size is not NULL, then it will be set to the actual number of
   bytes read.  Normally 4096 but can be smaller for a partial block */
 static int blocks_processed_in_buffer=0;
 static int blocks_remaining_in_buffer=0;
 static int nread;
 int i,j,extra_bytes;

 if (blocks_remaining_in_buffer==0) {
   /* fill buffer with the next chunk of image */
   blocks_processed_in_buffer=0;
   nread=read(image_fd,image_buff,sizeof(image_buff));
   if (nread == -1) { die(strerror(errno)); }
   if (nread == 0 ) { return NULL; } /* EOF */
   /* the number of blocks read */
   blocks_remaining_in_buffer=(nread/BLOCKSIZE);
   /* see if there is a partial end block */
   extra_bytes = nread - (blocks_remaining_in_buffer * BLOCKSIZE);
   if (extra_bytes) {
     /* pad out the last block and bump the number of blocks available */
     j=blocks_remaining_in_buffer*BLOCKSIZE;
     for(i=extra_bytes; i< BLOCKSIZE; i++) { image_buff[j+i]=(char)0; }
     blocks_remaining_in_buffer++;
   }
 }
 if (p_size) { *p_size = (nread >= BLOCKSIZE ? BLOCKSIZE : nread); }
 blocks_remaining_in_buffer--;
 nread-=BLOCKSIZE;
 return(&image_buff[(blocks_processed_in_buffer++)*BLOCKSIZE]);
}


// status=patch_image(start_offset,start_patchblock_p,number_of_contiguous_patchblocks, image_size);
// status = 0 OK, status = 1 OK but hit end of the image
// status = -1 error, attempt write beyond end of image, write error, etc...
int patch_image (
   offset_t start_offset,  // offset in image of start of patch
   char *start_patchblock_p,  // pointer to buffer of patch blocks
   int  number_blocks,  // number of 4K blocks
   offset_t image_size   // size of the image
  )
{
  // strategy: seek to the offset within the image and write the patch
  // special case: do not patch beyond the end of the image, its possible
  // that last block of the image is a partial block
  // in that case only use as much of the patchblock as the size of the
  // last block.
  // otherwise any attempt to patch beyond the end of the image will
  // return an error.
  size_t size_of_patch;
  int status=0;
  off64_t  seek_rtrn;
  ssize_t  write_rtrn;
  // verify that the patch will not go beyond the end of the image
  size_of_patch = number_blocks*BLOCKSIZE;
  if (start_offset + size_of_patch > image_size ) {
    // check for final partial block
    if (start_offset + size_of_patch - image_size < BLOCKSIZE) {
       // this is ok, we will adjust the patch to fit the image
       status=1;
       size_of_patch = image_size - start_offset;
    } else {
       // attempt to patch beyond end of image
       status = -1;
    }
  }
  if (status >= 0) {
	seek_rtrn=lseek64(image_fd, start_offset, SEEK_SET);
        if (seek_rtrn == -1) {
          // seek failure
	  return(-1);
        }
        write_rtrn = write(image_fd, start_patchblock_p, size_of_patch);
        if (write_rtrn != size_of_patch) {
          // write failure
          return(-1);
        }
  }
  return(status);
}

/* close the image, useful only when image is being written */
void close_image(void) {
  if (image_fd) { close(image_fd); } /* dont close stdin == 0 */
}
