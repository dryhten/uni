/**
* Mihail Dunaev, 331CC
* Computer Systems Architecture
* Assignment 3 - TV Channel Preview Rendering
* 30 April 2013
*
* Source file for spu program - using vectors
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spu_mfcio.h>
#include <spu_intrinsics.h>

#include "util.h"
#include "image.h"

#define BUFFER_SIZE 164736    /* just another magic number */
#define KB16 (1<<14)          /* DMA - 16384 bytes */

/**
* reads into local store 88x624 pixels (164736 bytes)
* multiple DMA transfers (11) of 16K will take place
* last transfer = 896 bytes = 16 * 56 bytes
*/
void read_image (unsigned char* local_addr, unsigned char* remote_addr,
                 unsigned long int* global_offset, int tag);

/**
* scales image & saves it into same buffer
* new image will be 22x156 pixels (10296 bytes)
*/
void scale_image (image* img);

/**
* sends back the scaled image, at the right address
* multiple DMA transfers due to alignment
*/
void send_image (unsigned char* local_addr, unsigned char** remote_addr,
                image* res_image, int tag, int col);


int main (unsigned long long speid, unsigned long long argp,
          unsigned long long envp){

  /* local data aligned(128) */
  unsigned int spe_id;                          /* my id (better than param) */
  unsigned int aux;                             /* helper */
  unsigned long int offset;                     /* offset in image */
  unsigned char* res_addr;                      /* res img addr */
  unsigned int line, col;                       /* index, index ... */
  my_struct ms __attribute__((aligned(128)));   /* global info */
  image my_image __attribute__((aligned(128))); /* my image to scale */
  image res_image __attribute__((aligned(128)));/* result image */
  int tag = 1, tag_mask = 1 << tag;             /* used in dma transf */

  /* get id */
  spe_id = spu_read_in_mbox();

  /* get image address */
  mfc_get(&ms, (unsigned int)argp, envp, tag, 0, 0);
  mfc_write_tag_mask(tag_mask);
  mfc_read_tag_status_all();

  /* get destination address */
  mfc_get(&res_image, (unsigned long int)ms.res_img, 128, tag, 0, 0);
  mfc_read_tag_status_all();
  res_addr = res_image.data;
  line = spe_id / 4;
  col = spe_id % 4;

  /* main loop */
  do{

    /* local data */
    unsigned char* remote_data_address;
    unsigned long int global_offset = 0;
    unsigned int image_size;
    unsigned char* start_res_addr = res_addr;

    /* wait for PPU to read file */
    aux = spu_read_in_mbox();

    /* get data address */
    offset = spe_id * sizeof(image);
    mfc_get(&my_image, (unsigned long int)ms.imgs + offset, 128, tag, 0, 0);
    mfc_read_tag_status_all();
    remote_data_address = my_image.data;

    /* check stop condition */
    if (remote_data_address == NULL)
      break;

    /* allocate new mem */
    my_image.data =(unsigned char*) malloc_align(BUFFER_SIZE, 7);
    image_size = my_image.width * my_image.height * NUM_CHANNELS;


   	if (!my_image.data){
		  perror("malloc failed");
		  printf("data = %p\n", my_image.data);
		}

    /* compute resulting image start address */
    start_res_addr += (line * res_image.width * NUM_CHANNELS *
                       res_image.height) / SCALE_FACTOR;
    start_res_addr += (col * res_image.width * NUM_CHANNELS) / SCALE_FACTOR;

    /* small loop */
    do{

      /* read data */
      read_image(my_image.data, remote_data_address, &global_offset, tag);

      /* scale it */
      scale_image(&my_image);

      /* send it */
      send_image(my_image.data, &start_res_addr, &res_image, tag, col);

      /* are we done yet ? */
      if (global_offset >= image_size)
        break;

    } while (1);

    /* notify */
    spu_write_out_intr_mbox(aux);

    /* free data */
    free_align(my_image.data);

  } while (1);


  return 0;
}

void read_image (unsigned char* local_addr, unsigned char* remote_addr,
                 unsigned long int* global_offset, int tag){

  unsigned int delta = KB16;
  unsigned int total_read = 0;
  unsigned int local_offset = 0;

  do{

    total_read = total_read + delta; /* we should read 16K */

    if (total_read > BUFFER_SIZE){
      total_read = total_read - delta;
      delta = BUFFER_SIZE - total_read;
      total_read = BUFFER_SIZE;
    }

    mfc_get(local_addr + local_offset,
            (unsigned long int)remote_addr + (*global_offset),
            delta, tag, 0, 0);
    mfc_read_tag_status_all();

    (*global_offset) = (*global_offset) + delta;

    if (total_read >= BUFFER_SIZE)
      break;

    local_offset = local_offset + delta;
  } while (1);
}

void scale_image (image* img){

  unsigned int b_rows = img->height / SCALE_FACTOR;   /*  88 */
  unsigned int b_cols = img->width;                   /* 624 */
  unsigned int s_rows = b_rows / SCALE_FACTOR;        /*  22 */
  unsigned int s_cols = b_cols / SCALE_FACTOR;        /* 156 */
  unsigned int di,dj,si,sj;
  unsigned char aux;

  /* 16 values or red, green, blue at once */
  unsigned char rvalues[16], *raddr;
  unsigned char gvalues[16], *gaddr;
  unsigned char bvalues[16], *baddr;

  for (di = 0; di < s_rows; di++)
    for (dj = 0; dj < s_cols; dj++){

      /* first - only get values, no summing */
      raddr = rvalues;
      gaddr = gvalues;
      baddr = bvalues;

			for (si = di * SCALE_FACTOR; si < (di + 1) * SCALE_FACTOR; si++)
				for (sj = dj * SCALE_FACTOR; sj < (dj + 1) * SCALE_FACTOR; sj++){
					(*raddr) = RED(img, si, sj);
					(*gaddr) = GREEN(img, si, sj);
					(*baddr) = BLUE(img, si, sj);
					raddr++; gaddr++; baddr++;
				}

      /* sum using specific intrinsics */
      vector unsigned char* rvec;
      vector unsigned char* gvec;
      vector unsigned char* bvec;

      rvec = (vector unsigned char*) rvalues;
      gvec = (vector unsigned char*) gvalues;
      bvec = (vector unsigned char*) bvalues;

      /* aux vectors */
      vector unsigned char a,b,d,pattern;
      vector unsigned char zero = (vector unsigned char) {0};

      /* 8 red, 8 green */
      pattern = (vector unsigned char)
                {0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23};
      a = spu_shuffle((*rvec), (*gvec), pattern);
      pattern = (vector unsigned char)
                {8,9,10,11,12,13,14,15,24,25,26,27,28,29,30,31};
      b = spu_shuffle((*rvec), (*gvec), pattern);
      d = (vector unsigned char)spu_avg (a,b);

      /* 4 red, 4 green, 8 blue */
      pattern = (vector unsigned char)
                {0,1,2,3,8,9,10,11,16,17,18,19,20,21,22,23};
      a = spu_shuffle(d, (*bvec), pattern);
      pattern = (vector unsigned char)
                {4,5,6,7,12,13,14,15,24,25,26,27,28,29,30,31};
      b = spu_shuffle(d, (*bvec), pattern);
      d = (vector unsigned char)spu_avg(a,b);

      /* 2 red, 2 green, 4 blue */
      pattern = (vector unsigned char)
                {0,1,4,5,8,9,10,11,16,17,18,19,20,21,22,23};
      a = spu_shuffle(d, zero, pattern);
      pattern = (vector unsigned char)
                {2,3,6,7,12,13,14,15,24,25,26,27,28,29,30,31};
      b = spu_shuffle(d, zero, pattern);
      d = (vector unsigned char)spu_avg (a,b);

      /* red, green, 2 blue */
      pattern = (vector unsigned char)
                {0,2,4,5,8,9,10,11,16,17,18,19,20,21,22,23};
      a = spu_shuffle(d, zero, pattern);
      pattern = (vector unsigned char)
                {1,3,6,7,12,13,14,15,24,25,26,27,28,29,30,31};
      b = spu_shuffle(d, zero, pattern);
      d = (vector unsigned char)spu_avg(a,b);

      /* save red, green, sum(blue)/2 */
      aux = spu_extract(d, 2);
      aux = aux + ((spu_extract(d,3) - aux)/2);
      RED(img, di, dj) = spu_extract(d,0);
      GREEN(img, di, dj) = spu_extract(d,1);
      BLUE(img, di, dj) = aux;
    }
}

void send_image (unsigned char* local_addr, unsigned char** remote_addr,
                image* res_image, int tag, int col){

  unsigned int i;
  unsigned int num_lines = res_image->height / SCALE_FACTOR / SCALE_FACTOR;

  /**
  * depending on last column, last 4 bits will be :
  * 0000, 0100, 1000, 1100
  * we create the same last 4 bits by copying the scaled image
  * easiest method to write
  */

  if (col == 0){ /* no pre-transfer required; no copying */

    unsigned int total, dim_1, dim_2, dim_3, off = 0;
    total = (res_image->width * NUM_CHANNELS) / 4;              /* 468 */
    dim_1 = total - (total % 128);                              /* 384 */
    total = total - dim_1;                                      /*  84 */
    dim_2 = total - (total % 16);                               /*  80 */
    total = total - dim_2;                                      /*   4 */
    dim_3 = total - (total % 4);                                /*   4 */

    for (i = 0; i < num_lines; i++){

      /**
      * one line has 468 bytes
      * we first transfer 384, then 80 and 4
      */

      mfc_put(local_addr, (unsigned long int)(*remote_addr),
               dim_1, tag, 0, 0);
      mfc_read_tag_status_all();
      off = off + dim_1;

      mfc_put(local_addr + off, (unsigned long int)((*remote_addr) + off),
               dim_2, tag, 0, 0);
      mfc_read_tag_status_all();
      off = off + dim_2;

      mfc_put(local_addr + off, (unsigned long int)((*remote_addr) + off),
               dim_3, tag, 0, 0);
      mfc_read_tag_status_all();

      (*remote_addr) = (*remote_addr) + (res_image->width * NUM_CHANNELS);
      local_addr = local_addr + (res_image->width * NUM_CHANNELS);
      off = 0;
    }
  }
  else if (col == 1){ /* pre-transfers of 4 & 8 bytes */

    unsigned char* it_addr = local_addr;
    unsigned int offset = (res_image->width * NUM_CHANNELS) / SCALE_FACTOR;

    for (i = 0; i < num_lines; i++){
      memcpy(it_addr + offset, it_addr, offset);
      it_addr = it_addr + (res_image->width * NUM_CHANNELS);
    }

    local_addr = local_addr + offset;

    unsigned int total, dim_1, dim_2, dim_3, dim_4, off = 0;
    total = (res_image->width * NUM_CHANNELS) / 4;              /* 468 */
    dim_1 = 4;                                                  /*   4 */
    total = total - dim_1;                                      /* 464 */
    dim_2 = 8;                                                  /*   8 */
    total = total - dim_2;                                      /* 456 */
    dim_3 = total - (total % 16);                               /* 448 */
    dim_4 = total - dim_3;                                      /*   8 */

    for (i = 0; i < num_lines; i++){

      /**
      * one line has 468 bytes
      * we first transfer 4, then 8, 448 and 8
      */

      mfc_put(local_addr, (unsigned long int)(*remote_addr),
               dim_1, tag, 0, 0);
      mfc_read_tag_status_all();
      off = off + dim_1;

      mfc_put(local_addr + off, (unsigned long int)((*remote_addr) + off),
               dim_2, tag, 0, 0);
      mfc_read_tag_status_all();
      off = off + dim_2;

      mfc_put(local_addr + off, (unsigned long int)((*remote_addr) + off),
               dim_3, tag, 0, 0);
      mfc_read_tag_status_all();
      off = off + dim_3;

      mfc_put(local_addr + off, (unsigned long int)((*remote_addr) + off),
               dim_4, tag, 0, 0);
      mfc_read_tag_status_all();
      off = off + dim_4;

      (*remote_addr) = (*remote_addr) + (res_image->width * NUM_CHANNELS);
      local_addr = local_addr + (res_image->width * NUM_CHANNELS);
      off = 0;
    }
  }
  else if (col == 2){ /* just 8 */

    unsigned char* it_addr = local_addr;
    unsigned int offset = (2 * res_image->width * NUM_CHANNELS) / SCALE_FACTOR;

    for (i = 0; i < num_lines; i++){
      memcpy (it_addr + offset, it_addr, (offset/2));
      it_addr = it_addr + (res_image->width * NUM_CHANNELS);
    }

    local_addr = local_addr + offset;

    unsigned int total, dim_1, dim_2, dim_3, dim_4, off = 0;
    total = (res_image->width * NUM_CHANNELS) / 4;              /* 468 */
    dim_1 = 8;                                                  /*   8 */
    total = total - dim_1;                                      /* 460 */
    dim_2 = total - (total % 16);                               /* 448 */
    total = total - dim_2;                                      /*  12 */
    dim_3 = total - (total % 8);                                /*   8 */
    dim_4 = total - dim_3;                                      /*   4 */

    for (i = 0; i < num_lines; i++){

      /**
      * one line has 468 bytes
      * we first transfer 8, then 448, 8 and 4
      */

      mfc_put(local_addr, (unsigned long int)(*remote_addr),
               dim_1, tag, 0, 0);
      mfc_read_tag_status_all();
      off = off + dim_1;

      mfc_put(local_addr + off, (unsigned long int)((*remote_addr) + off),
               dim_2, tag, 0, 0);
      mfc_read_tag_status_all();
      off = off + dim_2;

      mfc_put(local_addr + off, (unsigned long int)((*remote_addr) + off),
               dim_3, tag, 0, 0);
      mfc_read_tag_status_all();
      off = off + dim_3;

      mfc_put(local_addr + off, (unsigned long int)((*remote_addr) + off),
               dim_4, tag, 0, 0);
      mfc_read_tag_status_all();

      (*remote_addr) = (*remote_addr) + (res_image->width * NUM_CHANNELS);
      local_addr = local_addr + (res_image->width * NUM_CHANNELS);
      off = 0;
    }
  }
  else if (col == 3){ /* just 4 */

    unsigned char* it_addr = local_addr;
    unsigned int offset = (3 * res_image->width * NUM_CHANNELS) / SCALE_FACTOR;

    for (i = 0; i < num_lines; i++){

      memcpy(it_addr + offset, it_addr, (offset/3));
      it_addr = it_addr + (res_image->width * NUM_CHANNELS);
    }

    local_addr = local_addr + offset;

    unsigned int total, dim_1, dim_2, off = 0;
    total = (res_image->width * NUM_CHANNELS) / 4;              /* 468 */
    dim_1 = 4;                                                  /*   4 */
    dim_2 = total - dim_1;                                      /* 464 */

    for (i = 0; i < num_lines; i++){

      /**
      * one line has 468 bytes
      * we first transfer 4 and then 464
      */

      mfc_put(local_addr, (unsigned long int)(*remote_addr),
               dim_1, tag, 0, 0);
      mfc_read_tag_status_all();
      off = off + dim_1;

      mfc_put(local_addr + off, (unsigned long int)((*remote_addr) + off),
               dim_2, tag, 0, 0);
      mfc_read_tag_status_all();

      (*remote_addr) = (*remote_addr) + (res_image->width * NUM_CHANNELS);
      local_addr = local_addr + (res_image->width * NUM_CHANNELS);
      off = 0;
    }
  }
}
