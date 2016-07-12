/**
* Mihail Dunaev, 331CC
* Computer Systems Architecture
* Assignment 3 - TV Channel Preview Rendering
* 30 April 2013
*
* Interface with image processing functions - serial.c
*/

#ifndef IMAGE_H_
#define IMAGE_H_

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#define PRINT_ERR_MSG_AND_EXIT(format, ...) \
	{ \
	fprintf(stderr, "%s:%d: " format, __func__, __LINE__, ##__VA_ARGS__); \
	fflush(stderr); \
	exit(1); \
	}

#define GET_COLOR_VALUE(img, i, j, k) \
	((img)->data[((i) * (img->width) + (j)) * NUM_CHANNELS + (k)])
#define RED(img, i, j)		GET_COLOR_VALUE(img, i, j, 0)
#define GREEN(img, i, j)	GET_COLOR_VALUE(img, i, j, 1)
#define BLUE(img, i, j)		GET_COLOR_VALUE(img, i, j, 2)

#define GET_TIME_DELTA(t1, t2) ((t2).tv_sec - (t1).tv_sec + \
				((t2).tv_usec - (t1).tv_usec) / 1000000.0)

#define NUM_STREAMS 		16
#define MAX_FRAMES			100
#define MAX_PATH_LEN		256
#define IMAGE_TYPE_LEN 		2
#define SMALL_BUF_SIZE 		16
#define SCALE_FACTOR		4
#define NUM_CHANNELS		3
#define MAX_COLOR			255
#define NUM_IMAGES_WIDTH	4
#define NUM_IMAGES_HEIGHT	4

#define IMAGE_HEIGHT 624      /* actually I don't use this *
                               * but it looks cool */
#define IMAGE_WIDTH 352       /* same */
#define IMAGE_SIZE 658944     /* same */


#define PADDING_SIZE_IMAGE 116

/* image abstraction */
typedef struct {
  unsigned int width, height;
  unsigned char* data;
  char padding[PADDING_SIZE_IMAGE];
} image;

/* reads image file from path, stores it in img */
void read_pnm (char* path, image* img);

/* save image to file */
void write_pnm (char* path, image* img);

/* scales the image by 1/SCALE_FACTOR */
void scale_area_avg (image* src, image* dest);

/* reads char from file */
char read_char (int fd, char* path);

/* allocates memory for image */
void alloc_image (image* img);

/* free image data */
void free_image (image* img);

/* read from fd until character c is found; returns atoi (str) read */
unsigned int read_until(int fd, char c, char* path);

/* create final result image from downscaled images - ...  */
void create_big_image(image* scaled, image* big_image);

#endif /* IMAGE_H_ */
