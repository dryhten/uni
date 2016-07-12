/**
* Mihail Dunaev, 331CC
* Computer Systems Architecture
* Assignment 3 - TV Channel Preview Rendering
* 30 April 2013
*
* Source file for ppu program
*/

#include <stdio.h>
#include <stdlib.h>
#include <libspe2.h>
#include <pthread.h>
#include <string.h>

#include "util.h"
#include "image.h"

#define MAX_THREADS 16

/* thread function */
void* thread_func (void* arg);

/* name of the elf file */
extern spe_program_handle_t spu;

/* context_run data */
typedef struct {
  spe_context_ptr_t ctx;  /* spe ctx */
  unsigned int entry;     /* will be modified for each */
  void* argp;             /* pointer to prog data */
  void* envp;             /* pointer to env data */
} data_t;

int main (int argc, char* argv[]){

  /* data */
  int ret;                                        /* return values */
  unsigned int i, j;                              /* iterators */
  unsigned int aux;                               /* mailbox use */
  spe_event_unit_t pevents[MAX_THREADS];          /* event array */
  spe_event_unit_t events_received[MAX_THREADS];  /* ... */
  spe_event_handler_ptr_t event_handler;          /* handler for events */
  char filename[MAX_PATH_LEN];                    /* filenames */
  data_t ds[MAX_THREADS];                         /* data passed to each th */
  pthread_t th[MAX_THREADS];                      /* thread ids */
  int num_files;                                  /* how many images */
  double scale_time = 0, total_time = 0;          /* time vars */
	struct timeval t1, t2, t3, t4;

  /* array of images (16) */
  image input[NUM_STREAMS] __attribute__((aligned(128)));
  /* resulting image */
  image res_image __attribute__((aligned(128)));
  /* struct with more info (first DMA transfer) */
  my_struct ms __attribute__((aligned(128)));

  /* quick return */
  if (argc != 4){
		printf("Usage: ./ppu input_path output_path num_frames\n");
		exit(1);
	}

  /* start time */
  gettimeofday(&t3, NULL);
  /* create event handler */
  event_handler = spe_event_handler_create();
  /* compute num_files */
  num_files = atoi(argv[3]);

  /**
  * to set-up the result image (widht, height, type etc)
  * we just read a random (first) image
  */

  snprintf(filename, MAX_PATH_LEN, "%s/stream01/image1.pnm", argv[1]);
  read_pnm(filename, &res_image);

  /**
  * set-up info structure :
  * - how mant images
  * - address of images array
  * - address of resulting image
  */

  ms.n = NUM_STREAMS;
  ms.imgs = input;
  ms.res_img = &res_image;

  /**
  * init procedure :
  * - create context
  * - init event vars
  * - load the elf address into ctx
  * - add entry, argp & envp
  * - start threads
  */

  for (i = 0; i < NUM_STREAMS; i++){
    ds[i].ctx = spe_context_create(SPE_EVENTS_ENABLE, NULL);
    if (ds[i].ctx == NULL){
      perror("spe_context_create");
      exit(-1);
    }
    pevents[i].events = SPE_EVENT_OUT_INTR_MBOX;
    pevents[i].spe = ds[i].ctx;
    pevents[i].data.u32 = i;
    spe_event_handler_register(event_handler, &pevents[i]);

    ret = spe_program_load(ds[i].ctx, &spu);
    if (ret == -1){
      perror("spe_program_load");
      exit(-1);
    }
    ds[i].entry = SPE_DEFAULT_ENTRY;
    ds[i].argp = (void*)(&ms);
    ds[i].envp = (void*)128;

    pthread_create(&th[i], NULL, &thread_func, &ds[i]);
  }

  /* send ids to SPUs using mailboxes */
  for (i = 0; i < NUM_STREAMS; i++){
    ret = spe_in_mbox_write(ds[i].ctx, &i, 1, SPE_MBOX_ANY_NONBLOCKING);
    if (ret == 0)
      printf("Message could not be written to %d\n", i);
  }

  /**
  * main loop :
  * - read images from files
  * - notify SPUs they can start scaling
  * - wait for them to finish
  * - write result image to file
  */

  for (j = 0; j < num_files; j++){

    /**
    * reading part :
    * notify SPU using a simple mailbox write
    * SPUs will use the MFC to get image data from
    * the image address
    */

    printf("Processing Frame %d\n", j + 1);

    for (i = 0; i < NUM_STREAMS; i++){
      snprintf(filename, MAX_PATH_LEN,
                "%s/stream%02d/image%d.pnm", argv[1], i + 1, j + 1);
      read_pnm(filename, &input[i]);
      ret = spe_in_mbox_write(ds[i].ctx, &i, 1, SPE_MBOX_ANY_NONBLOCKING);
    }

    /**
    * waiting part :
    * the scaling time is greater so it's better to use events
    * after 16 received events, we may continue
    * SPUs will be blocked waiting for image data so
    * synchronization is assured
    */

    gettimeofday(&t1, NULL);
    for (i = 0; i < NUM_STREAMS; i++){
      spe_event_wait(event_handler, events_received, NUM_STREAMS, -1);
      spe_out_intr_mbox_read(events_received[0].spe, &aux, 1,
                              SPE_MBOX_ANY_BLOCKING);
    }
		gettimeofday(&t2, NULL);
		scale_time += GET_TIME_DELTA(t1, t2);

    /**
    * saving part :
    * just print result img
    * and free images - every read uses malloc
    */

    snprintf(filename, MAX_PATH_LEN, "%s/result%d.pnm", argv[2], j + 1);
    write_pnm(filename, &res_image);

    for (i = 0; i < NUM_STREAMS; i++)
      free_image(&input[i]);
  }

  /**
  * one last iteration with data = NULL
  * SPUs will die softly
  */

  for (i = 0; i < NUM_STREAMS; i++){
    input[i].data = NULL;
    ret = spe_in_mbox_write(ds[i].ctx, &i, 1, SPE_MBOX_ANY_NONBLOCKING);
  }


  /**
  * wait for threads to finish & destroy them
  */

  for (i = 0; i < NUM_STREAMS; i++){

    pthread_join(th[i], NULL);
    spe_context_destroy(ds[i].ctx);
  }

  /**
  * free memory : pointless but whatever ...
  */

  free_image(&res_image);

  /**
  * print time info
  */

  gettimeofday(&t4, NULL);
	total_time += GET_TIME_DELTA(t3, t4);
	printf("Scale time: %lf\n", scale_time);
	printf("Total time: %lf\n", total_time);

  return 0;
}

void* thread_func (void* arg){

  data_t* data = (data_t*)arg;
  spe_context_run(data->ctx, &data->entry, 0, data->argp,
                   data->envp, NULL);
  pthread_exit(NULL);
}


/** --------------------------------------------------------------------------
* everything else here is taken from serial.c
* functions that work with pnm images
*/

void read_pnm (char* path, image* img){

	int fd, bytes_read, bytes_left;
	char image_type[IMAGE_TYPE_LEN];
	unsigned char *ptr;
	unsigned int max_color;

	fd = open(path, O_RDONLY);

	if (fd < 0){
		PRINT_ERR_MSG_AND_EXIT("Error opening %s\n", path);
		exit(1);
	}

	//read image type; should be P6
	bytes_read = read(fd, image_type, IMAGE_TYPE_LEN);
	if (bytes_read != IMAGE_TYPE_LEN){
		PRINT_ERR_MSG_AND_EXIT("Couldn't read image type for %s\n", path);
	}
	if (strncmp(image_type, "P6", IMAGE_TYPE_LEN)){
		PRINT_ERR_MSG_AND_EXIT("Expecting P6 image type for %s. Got %s\n",
			path, image_type);
	}

	//read \n
	read_char(fd, path);

	//read width, height and max color value
	img->width = read_until(fd, ' ', path);
	img->height = read_until(fd, '\n', path);
	max_color = read_until(fd, '\n', path);
	if (max_color != MAX_COLOR){
		PRINT_ERR_MSG_AND_EXIT("Unsupported max color value %d for %s\n",
			max_color, path);
	}

	//allocate image data
	alloc_image(img);

	//read the actual data
	bytes_left = img->width * img->height * NUM_CHANNELS;
	ptr = img->data;
	while (bytes_left > 0){
		bytes_read = read(fd, ptr, bytes_left);
		if (bytes_read <= 0){
			PRINT_ERR_MSG_AND_EXIT("Error reading from %s\n", path);
		}
		ptr += bytes_read;
		bytes_left -= bytes_read;
	}

	close(fd);
}

void write_pnm (char* path, image* img){

	int fd, bytes_written, bytes_left;
	char buf[32];
	unsigned char* ptr;

	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0){
		PRINT_ERR_MSG_AND_EXIT("Error opening %s\n", path);
	}

	//write image type, image width, height and max color
	sprintf(buf, "P6\n%d %d\n%d\n", img->width, img->height, MAX_COLOR);
	ptr = (unsigned char*)buf;
	bytes_left = strlen(buf);
	while (bytes_left > 0){
		bytes_written = write(fd, ptr, bytes_left);
		if (bytes_written <= 0){
			PRINT_ERR_MSG_AND_EXIT("Error writing to %s\n", path);
		}
		bytes_left -= bytes_written;
		ptr += bytes_written;
	}

	//write the actual data
	ptr = img->data;
	bytes_left = img->width * img->height * NUM_CHANNELS;
	while (bytes_left > 0){
		bytes_written = write(fd, ptr, bytes_left);
		if (bytes_written <= 0){
			PRINT_ERR_MSG_AND_EXIT("Error writing to %s\n", path);
		}
		bytes_left -= bytes_written;
		ptr += bytes_written;
	}

	close(fd);
}

void scale_area_avg (image* src, image* dest){

	int m, n, i, j;

	for (i = 0; i < dest->height; i++){
		for (j = 0; j < dest->width; j++){
			int rval = 0, gval = 0, bval = 0;
			for (m = i * SCALE_FACTOR; m < (i + 1) * SCALE_FACTOR; m++){
				for (n = j * SCALE_FACTOR; n < (j + 1) * SCALE_FACTOR; n++){
					rval += RED(src, m, n);
					gval += GREEN(src, m, n);
					bval += BLUE(src, m, n);
				}
			}
			RED(dest, i, j) = rval / SCALE_FACTOR / SCALE_FACTOR;
			GREEN(dest, i, j) = gval / SCALE_FACTOR / SCALE_FACTOR;
			BLUE(dest, i, j) = bval / SCALE_FACTOR / SCALE_FACTOR;
		}
	}
}

char read_char (int fd, char* path){

	char c;
	int bytes_read;

	bytes_read = read(fd, &c, 1);
	if (bytes_read != 1){
		PRINT_ERR_MSG_AND_EXIT("Error reading from %s\n", path);
	}

	return c;
}

void alloc_image (image* img){

  /* allocate with align */
  img->data = (unsigned char*)malloc_align(NUM_CHANNELS * img->width *
               img->height * sizeof(char), 7); /* aligned 128 = 2^7 */

	if (!img->data){
		PRINT_ERR_MSG_AND_EXIT("Calloc failed\n");
	}
}

void free_image (image* img){
	free_align(img->data);
}

unsigned int read_until (int fd, char c, char* path){

	char buf[SMALL_BUF_SIZE];
	int i;
	unsigned int res;

	i = 0;
	memset(buf, 0, SMALL_BUF_SIZE);
	buf[i] = read_char(fd, path);
	while (buf[i] != c){
		i++;
		if (i >= SMALL_BUF_SIZE){
			PRINT_ERR_MSG_AND_EXIT("Unexpected file format for %s\n", path);
		}
		buf[i] = read_char(fd, path);
	}
	res = atoi(buf);
	if (res <= 0) {
		PRINT_ERR_MSG_AND_EXIT("Result is %d when reading from %s\n",
			res, path);
	}

	return res;
}

void create_big_image (image* scaled, image* big_image){

	int i, j, k;
	unsigned char* ptr = big_image->data;
	image* img_ptr;
	unsigned int height = scaled[0].height;
	unsigned int width = scaled[0].width;

	for (i = 0; i < NUM_IMAGES_HEIGHT; i++){
		for (k = 0; k < height; k++) {
			//line by line copy
			for (j = 0; j < NUM_IMAGES_WIDTH; j++){
				img_ptr = &scaled[i * NUM_IMAGES_WIDTH + j];
				memcpy(ptr, &img_ptr->data[k * width * NUM_CHANNELS], width *
				       NUM_CHANNELS);
				ptr += width * NUM_CHANNELS;
			}
		}
	}
}
