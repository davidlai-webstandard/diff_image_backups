/* Copyright (c) 2020 David Lai davidlai@webstandard.com see LICENSE */

#include "blk_utils.h"

#include "blk_md5.h"

#include "blk_imageutils.h"

#include "blk_patchfileutils.h"



// int image_fd;


void usage(char *argv[])
{
fprintf(stderr,"Usage: %s -i image -p patchfile [-v]\n", argv[0]);
fprintf(stderr," patches image using the patchfile\n");
fprintf(stderr," read patchfile from stdin if patchfile is '-'.\n");
fprintf(stderr," -v reports the statistics to stderr\n");
exit(1);
}

//  // status=patch_image(start_offset,start_patchblock_p,number_of_contiguous_patchblocks);
//  // status = 0 OK, status = 1 OK but hit end of the image
//  // status = -1 error, attempt write beyond end of image, write error, etc...
//  int patch_image (
//     offset_t start_offset,  // offset in image of start of patch
//     char *start_patchblock_p,  // pointer to buffer of patch blocks
//     int  number_blocks  // number of 4K blocks
//    )
//  {
//    // strategy: seek to the offset within the image and write the patch
//    // special case: do not patch beyond the end of the image, its possible
//    // that last block of the image is a partial block
//    // in that case only use as much of the patchblock as the size of the
//    // last block.
//    // otherwise any attempt to patch beyond the end of the image will
//    // return an error.
//    size_t size_of_patch;
//    int status=0;
//    off64_t  seek_rtrn;
//    ssize_t  write_rtrn;
//    // verify that the patch will not go beyond the end of the image
//    size_of_patch = number_blocks*BLOCKSIZE;
//    if (start_offset + size_of_patch > image_size ) {
//      // check for final partial block
//      if (start_offset + size_of_patch - image_size < BLOCKSIZE) {
//         // this is ok, we will adjust the patch to fit the image
//         status=1;
//         size_of_patch = image_size - start_offset;
//      } else {
//         // attempt to patch beyond end of image
//         status = -1;
//      }
//    }
//    if (status >= 0) {
//  	seek_rtrn=lseek64(image_fd, start_offset, SEEK_SET);
//          if (seek_rtrn == -1) {
//            // seek failure
//  	  return(-1);
//          }
//          write_rtrn = write(image_fd, start_patchblock_p, size_of_patch);
//          if (write_rtrn != size_of_patch) {
//            // write failure
//            return(-1);
//          }
//    }
//    return(status);
//  }

int main(int argc, char *argv[])
{
 int i;
 char *image_file_name=NULL;
 char *patch_file_name=NULL;
 int verbose=0;
 // init the workspace
 offset_t current_offset;   /* current offset in image file being processed */
 uint64_t   total_patches_processed = 0;
 offset_t *offset_block_p; /* pointer to offset block of container */
 char *patchblock_p;	   /* pointer to the patchblock */
 int num_patches_in_container;
 int num_patches_processed;
 offset_t first_offset, last_offset;
 offset_t image_size;
 offset_t start_offset;
 char *start_patchblock_p;
 int number_of_contiguous_patchblocks;
 int status;

 for (i=1; i<argc; i++) {
  if (strcmp(argv[i],"-i")==0) {
     i++;
     if (!(image_file_name=argv[i])) { usage(argv); }
     continue;
  }
  if (strcmp(argv[i],"-p")==0) {
     i++;
     if (!(patch_file_name=argv[i])) { usage(argv); }
     continue;
  }
  if (strcmp(argv[i],"-v")==0) {
     verbose++;
     continue;
  }
  usage(argv);
 }
 if (!patch_file_name || !image_file_name) { usage(argv); }
 
 // open the image file for writing and get the size of the image file
 image_size = open_image(image_file_name,1);

 // open the patchfile
 open_patchfile_read(patch_file_name);
 
 // process loop
 total_patches_processed=0;
 while(1) {
   // read in a container
   num_patches_in_container=read_patchfile_container(&offset_block_p,&patchblock_p);
   if (num_patches_in_container == 0) { break; }
   num_patches_processed=0;
   // assume we will always process the first patch block
   // calculate number of contiguous blocks to process
   while(num_patches_processed < num_patches_in_container) {
     start_offset=offset_block_p[num_patches_processed];
     start_patchblock_p=&patchblock_p[num_patches_processed*BLOCKSIZE];
     current_offset=offset_block_p[num_patches_processed];
     for(i=num_patches_processed+1; i<num_patches_in_container; i++) {
      if (offset_block_p[i]!=current_offset+BLOCKSIZE) {
        /* not contiguous */
        break;
      }
      current_offset=offset_block_p[i];
     }
     /* calculate number of blocks */
     number_of_contiguous_patchblocks=i-num_patches_processed;
     status=patch_image(start_offset,start_patchblock_p,number_of_contiguous_patchblocks,image_size);
     if (status==-1) {
          die("Error patching, write error or attempt patch beyond end of image");
     }
     // update pointers and counters
     num_patches_processed+=number_of_contiguous_patchblocks;
     total_patches_processed+=number_of_contiguous_patchblocks;
     if (status == 1) {
        // we already hit the end of the image, ignore the remaining
        // patches
        break;
     }
   }
   // at this point the entire container is processed, loop and get next
   // container
 }
 if (verbose) {
    fprintf(stderr,"Number of blocks patched=%llu\n",total_patches_processed);
 }
 close_patchfile_read();
 close_image();
}

