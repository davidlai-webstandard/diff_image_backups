/* Copyright (c) 2020 David Lai davidlai@webstandard.com see LICENSE */

typedef uint64_t offset_t;      /* an offset */

extern offset_t open_image( char *filename, int write_flag );
extern char *next_image_block(int *p_size);

extern int patch_image (
   offset_t start_offset,  // offset in image of start of patch
   char *start_patchblock_p,  // pointer to buffer of patch blocks
   int  number_blocks,  // number of 4K blocks
   offset_t image_size   // size of the image
  );

extern void close_image(void);
