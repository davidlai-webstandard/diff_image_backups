/* Copyright (c) 2020 David Lai davidlai@webstandard.com see LICENSE */

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLOCKSIZE 4096
#define NUM_BLOCKS_64K	(65536/BLOCKSIZE)	/* number of blocks in buffer 64K buffer */

extern void die(char *message);


