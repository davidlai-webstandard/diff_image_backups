/* Copyright (c) 2020 David Lai davidlai@webstandard.com see LICENSE */

// number of patch blocks max in a container
#define NUM_PATCHES_PER_CONTAINER (BLOCKSIZE/sizeof(offset_t))

extern void open_patchfile_write( char *patch_file_name );
extern void close_patchfile_write( void );
extern void add_patchblock( offset_t offset, char *datablock );
extern void open_patchfile_read( char *patch_file_name );
extern void close_patchfile_read( void );
extern int read_patchfile_container( offset_t **offset_block_p, char **patchblocks_p);



