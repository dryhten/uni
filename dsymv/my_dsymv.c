/**
* Mihail Dunaev, 331CC
* Computer Systems Architecture
* Assignment 2 - BLAS dsymv
* 07 April 2013
*
*	My C implementation of BLAS dsymv
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define DEFAULT_SIZE 1000
#define MAX_PATHNAME_LENGTH 100

/* --- Arguments --- */

int n;				/* order of matrix */
double alpha, beta;		/* scalars */
double *a, *x, *y;		/* double precision matrix and vectors to multiply/add */


/* --- Routines --- */

/* cblas_dsymv equivalent function; matrix a is full (duplicated elements) */
void compute_result(int n, double alpha, double *a, double *x, double beta, double *y);

/* input reading function - actual data is stored on heap */
void input_data(char* size);

/* prints row-major (C) array y of size n */
void print_result(int n, double *y);

/* frees heap memory */
void free_mem(double *a, double *x, double *y);


/* --- Main Program --- */

int main (int argc, char* argv[]){

	struct timeval start, end;
	long int delta;
	char pathname[MAX_PATHNAME_LENGTH];

	/* get input data */
	if (argc >= 2)
		input_data(argv[1]);
	else
		input_data(NULL);

	/* set path name - useful when printing time */
	memset(pathname, 0, 100);
	int offset = 0;

	if (argc >= 3){
		offset = strlen(argv[2]);
		memcpy(pathname, argv[2], strlen(argv[2]));
		sprintf(pathname + offset, "/my_dsymv.time");
	}
	else
		sprintf(pathname, "my_dsymv.time");

	/* compute result */
	gettimeofday(&start, NULL);
	compute_result(n, alpha, a, x, beta, y);
	gettimeofday(&end, NULL);

	/* total time */
	delta = (end.tv_sec * 1000) + (end.tv_usec / 1000);
	delta = delta - (start.tv_sec * 1000) - (start.tv_usec / 1000);

	/* print it to file */
	FILE* output = fopen(pathname, "w+");
	fprintf(output, "N = %d time = %ld ms\n", n, delta);
	fclose(output);

	/* print result to stdout */
	print_result(n, y);

	/* free memory */
	free_mem(a, x, y);

	return 0;
}

void compute_result(int n, double alpha, double *a, double *x, double beta, double *y){

	/* sanity checks */
	if (a == NULL || x == NULL || y == NULL)
		return;

	/* quick return if possible - y won't change value */
	if ((n == 0) || ((alpha == 0) && (beta == 1)))
		return;

	int i, j;

	/* (beta * y) */
	if (beta == 0)
		for (i = 0; i < n; i++)
			y[i] = 0;
	else if (beta != 1)
		for (i = 0; i < n; i++)
			y[i] = beta * y[i];

	/* (alpha * A * x) */
	if (alpha == 0)
		return;
	else{
		double aux;
		double *a_adr = a;
		for (i = 0; i < n; i++){
			aux = 0;
			for (j = 0; j < n; j++){
				aux = aux + ((*a_adr) * x[j]);
				a_adr++;
			}
			y[i] = y[i] + (alpha * aux);
		}
	}
}

void input_data(char* size){

	int i,j;

	if (size == NULL)
		n = DEFAULT_SIZE;
	else
		n = atoi(size);

	alpha = 1;
	beta = 1;

	a = malloc(sizeof(double)*n*n);
	x = malloc(sizeof(double)*n);
	y = malloc(sizeof(double)*n);

	/* input a */
	double *a_adr = a;
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++){
			*(a_adr) = i+j;
			a_adr++;
		}

	/* input x */
	for (i = 0; i < n; i++)
		x[i] = i;

	/* input y */
	for (i = 0; i < n; i++)
		y[i] = i;
}

void print_result(int n, double *y){
	int i;
	for (i = 0; i < n; i++)
		printf("%f\n", y[i]);
}

void free_mem(double *a, double *x, double *y){
	if (a != NULL)
		free(a);
	if (x != NULL)
		free(x);
	if (y != NULL)
		free(y);
}
