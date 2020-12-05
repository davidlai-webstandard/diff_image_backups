/* Copyright (c) 2020 David Lai davidlai@webstandard.com see LICENSE */

// utilities for handling a patch file

#include "blk_utils.h"
#include "blk_imageutils.h"

#include "blk_patchfileutils.h"

// buffer of offsets in a container
offset_t offset_block[NUM_PATCHES_PER_CONTAINER];
// buffer of patch blocks
char patchblock[BLOCKSIZE * NUM_PATCHES_PER_CONTAINER];

FILE *patchfile;

void open_patchfile_write( char *patch_file_name ) 
{
 // open patchfile for output and set the variable patchfile
 // if patch_file_name is NULL, it wont be opened and patchfile will be set
 // to NULL
 // if patch_file_name is "-", it sets patchfile=stdout
 if (patch_file_name) {
   if (strcmp(patch_file_name,"-")==0) {
      patchfile=stdout;
   } else {
      patchfile=fopen(patch_file_name,"w");
      if (!patchfile) {
        die("Cant write patchfile");
      }
   }
 } else {
   patchfile=NULL;
 }
}

void close_patchfile_write( void )
{
   if (patchfile == NULL) { return; }
   if (patchfile == stdout) { return; } /* dont close stdout */
   fclose(patchfile);
}

void add_patchblock( offset_t offset, char *datablock )
{
  /* add a patch block to the output */
  /* offset = offset in image for the patch */
  /* datablock points to the replacement 4K block of data */
  /* if datablock=NULL then this call is used to end the patchfile */
  static int patches_in_container=0;
  int i, j;
  if (!patchfile) { 
     /* not writing patchfile, so no need to do anything */
     return;
  }
  if (datablock!=NULL) {
    /* add this patch to the container */
    offset_block[patches_in_container]=offset;
    memcpy(&patchblock[BLOCKSIZE * patches_in_container],datablock,BLOCKSIZE);
    patches_in_container++;
  }
  if (datablock==NULL && patches_in_container!=0) {
    /* end the patchfile */
    /* zero pad the offset block if necessary */
    for(i=patches_in_container; i< NUM_PATCHES_PER_CONTAINER; i++) {
         offset_block[i]=0;
    }
  }
  if (patches_in_container==NUM_PATCHES_PER_CONTAINER ||
      (datablock==NULL && patches_in_container!=0)) {
      /* write out the current container */
      if (NUM_PATCHES_PER_CONTAINER != fwrite(offset_block, sizeof(offset_t), NUM_PATCHES_PER_CONTAINER, patchfile)) {
         die("error writing patchfile");
      }
      if (patches_in_container != fwrite(patchblock,BLOCKSIZE,patches_in_container, patchfile)) {
         die("error writing patchfile");
      }
      patches_in_container=0;
   }
   if (datablock==NULL) { close_patchfile_write(); }
}

void open_patchfile_read( char *patch_file_name ) 
{
 // open patchfile for reading and set the variable patchfile
 // if patch_file_name is NULL, it wont be opened and patchfile will be set
 // to NULL
 // if patch_file_name is "-", it sets patchfile=stdin
 //
 if (patch_file_name) {
   if (strcmp(patch_file_name,"-")==0) {
      patchfile=stdin;
   } else {
      patchfile=fopen(patch_file_name,"r");
      if (!patchfile) {
        die("Cant read patchfile");
      }
   }
 } else {
   patchfile=NULL;
 }
}

void close_patchfile_read( void )
{
   if (patchfile == NULL) { return; }
   if (patchfile == stdin) { return; } /* dont close stdin */
   fclose(patchfile);
}

int read_patchfile_container( offset_t **offset_block_p, char **patchblocks_p)
{
   // read the next container into the buffer
   // returns pointer to the offset block in offset_block_p
   // returns pointer to the patch blocks in patchblocks_p
   // returns the number of patchblocks in container
   //  returns 0 if there are no more patchblocks (empty container)
   //  this also marks the end of the file
   
   size_t num_offsets,num_patches;
   // attempt to read a full container
   //  = 1 4K offset block
   //  + 512 patch blocks
   num_offsets=fread(offset_block, sizeof(offset_t), NUM_PATCHES_PER_CONTAINER, patchfile);
   if (num_offsets == 0 ) {
      // end of file, no more containers
      return(num_offsets);
   }
   if (num_offsets != NUM_PATCHES_PER_CONTAINER) {
      // error - short read, this is a corrupt patchfile.
      // since we cant read any patchblocks we will ignore the short
      // read, but flag an error
      fprintf(stderr,"Short read of patchfile, corrupt patchfile?\n");
      return 0;
   }
   // now read as many patchblocks as are available in the container
   // max NUM_PATCHES_PER_CONTAINER
   num_patches=fread(patchblock,BLOCKSIZE,NUM_PATCHES_PER_CONTAINER,patchfile);
   if (num_patches==0) {
      fprintf(stderr,"Short read of patchfile, corrupt patchfile?\n");
      return 0;
   }
   *offset_block_p = &(offset_block[0]);
   *patchblocks_p = &(patchblock[0]);
   return(num_patches);
}
