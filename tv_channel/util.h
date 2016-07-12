/**
* Mihail Dunaev, 331CC
* Computer Systems Architecture
* Assignment 3 - TV Channel Preview Rendering
* 30 April 2013
*
* Useful header. Used in both spu.c and ppu.c
*/

#ifndef UTIL_H_
#define UTIL_H_

#include "image.h"
#include "libmisc.h"

#define PADDING_SIZE_STRUCT 116
#define MAX_NUM_FILES 100

typedef struct {
  int n;                                /* size of imgs array */
  image* imgs;                          /* array of images (all 16) */
  image* res_img;                       /* resulting image */
  char padding[PADDING_SIZE_STRUCT];    /* to 128 bytes - peak performance */
} my_struct;

#endif /* UTIL_H_ */
