/* Copyright (c) 2020 David Lai davidlai@webstandard.com see LICENSE */

#include "blk_utils.h"

void die(char *message)
{
  fprintf(stderr,"Error: %s\n",message);
  exit(2);
}

