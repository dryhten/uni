/**
*	A2 SO - Minimal MPI
*	Dunaev Mihail, 331CC
*	10 Apr 2013
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <mqueue.h>
#include <semaphore.h>

#include "protocols.h"

/* data */
int num_children,        /* nr of processes */
		md,              /* descriptor for shm */
		num_queues,      /* if no errors, num_queues = num_children */
		num_sems;        /* same, only for semaphores */

pid_t* children_pids = NULL;          /* extra safe ... */
const char* shm_name = "/mpi";        /* shared memory (size & ranks) */
const char* sem_name = "/sem_rank";   /* semaphore used when writing to shm */
char* shm_addr = NULL;                /* used to map the shared memory */

/* cleans up memory */
int free_mem (void);

int main (int argc, char* argv[]){

	int ret;

	if (argc < 4){
		printf("Error, usage : %s -np [n] [prog]\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (strcmp("-np",argv[1]) != 0){  /* nothing else to do - no other options */
		printf("Error, usage : %s -np [n] [prog]\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* parse data */
	num_children = atoi(argv[2]);

	if (num_children < 1){
		printf("Error, np < 1\n");
		return EXIT_FAILURE;
	}

	children_pids = malloc(num_children * sizeof(pid_t));

	if (children_pids == NULL){
		perror("Error in malloc () (NULL returned) ... exiting\n");
		return EXIT_FAILURE;
	}

	/* create shared memory */
	md = shm_open(shm_name, O_RDWR | O_CREAT, 0644);

	if (md == -1){
		perror("Error in shm_open () (-1 returned) ... exiting\n");
		free_mem();
		return EXIT_FAILURE;
	}

	/* truncate it to appropriate size */
	ret = ftruncate(md, SIZE);

	if (ret == -1){
		perror("Error in ftruncate () (-1 returned) ... exiting\n");
		free_mem();
		return EXIT_FAILURE;
	}

	/* map it */
	shm_addr = (char*)mmap(NULL, SIZE, PROT_WRITE, MAP_SHARED, md, 0);

	if (shm_addr == MAP_FAILED){
		perror("Error in mmap () (MAP_FAILED returned) ... exiting\n");
		free_mem();
		return EXIT_FAILURE;
	}
	
	/* write size & first rank there */
	int* int_ptr = (int*)(shm_addr + OFFSET_SIZE);
	(*int_ptr) = num_children;
	int_ptr = (int*)(shm_addr + OFFSET_RANK);
	(*int_ptr) = 0;  /* will be updates as children call MPI_Init() */

	/* create queues, one for each process */
	mqd_t queue;

	int i;
	char queue_name[MAX_QUEUE_NAME];
	num_queues = 0;

	for (i = 0; i < num_children; i++){

		snprintf(queue_name, MAX_QUEUE_NAME, "%s%d", BASE_QUEUE_NAME, i); /* '\0' is automatically added */
		queue = mq_open(queue_name, O_CREAT, 0644, NULL); /* O_EXCL */

		if (queue == (mqd_t)-1){  /* mpi is compromised */
			perror("Error in mq_open () (-1 returned) ... exiting\n");
			free_mem();
			return EXIT_FAILURE;
		}

		if (mq_close(queue) == -1){ /* queue won't be unlinked */
			perror("Error in mq_close () (-1 returned) ... exiting\n");
			free_mem();
			return EXIT_FAILURE;
		}

		num_queues++;  /* helpful when ulinking in case of error */
	}

	/* create semaphore for shm (rank) */
	sem_t* sem = sem_open(sem_name, O_CREAT, 0644, 1);

	if (sem == SEM_FAILED){
		perror("Error in sem_open () (SEM_FAILED returned) ... exiting\n");
		free_mem();
		return EXIT_FAILURE;
	}

	/* no need now, just unlink later */
	if (sem_close(sem) == -1){
		perror("Error in sem_close () (-1 returned) ... exiting\n");
		free_mem();
		return EXIT_FAILURE;
	}

	/* create semaphores for queues (blocking send & recv) */
	char s_name[MAX_SEM_NAME];
	num_sems = 0;

	for (i = 0; i < num_children; i++){

		snprintf(s_name, MAX_SEM_NAME, "%s%d", BASE_SEM_NAME, i);
		sem = sem_open(s_name, O_CREAT, 0644, 0); /* O_EXCL */

		if (sem == SEM_FAILED){  /* same queues */
			perror("Error in sem_open () (SEM_FAILED) ... exiting\n");
			free_mem();
			return EXIT_FAILURE;
		}

		if (sem_close(sem) == -1){ /* ... */
			perror("Error in sem_close () (-1 returned) ... exiting\n");
			free_mem();
			return EXIT_FAILURE;
		}

		num_sems++;
	}

	/* start children */
	char *const argvec[] = {argv[3], NULL}; /* no argv for program */
	char* path = malloc(strlen(argv[3]) + 3); /* '\0' at the end */

	if (path == NULL){
		perror("Error in malloc () (NULL returned) ... exiting\n");
		free_mem();
		return EXIT_FAILURE;
	}

	/* format string properly */
	sprintf(path, "./%s", argv[3]);

	for (i = 0; i < num_children; i++){

		ret = fork();

		if (ret == -1){  /* bad execution of mpi - clean & exit */
			perror("Error in fork () (-1 returned) ... exiting\n");
			free(path);
			free_mem();
			return EXIT_FAILURE;
		}
		else if (ret == 0){  /* children */

			ret = execvp(path, argvec);

			/* bad execution of mpi - clean & exit */
			printf("Error in execvp () for '%s' ... exiting\n", path);
			free(path);
			exit(ret);
		}

		/* parent */
		children_pids[i] = ret;
	}

	free(path);

	int status;
	ret = EXIT_SUCCESS;

	/* wait for children */
	for (i = 0; i < num_children; i++){

		waitpid(children_pids[i], &status, 0);

		if (WEXITSTATUS(status) != EXIT_SUCCESS)
			ret = WEXITSTATUS(status);
	}

	/* free mem */
	int aux_ret = free_mem();

	if (ret == EXIT_SUCCESS)
		ret = aux_ret;

	return ret;
}

int free_mem (void){

	int ret = EXIT_SUCCESS;

	/* free array of children pids */
	if (children_pids != NULL)
		free(children_pids);

	/* unmap shared memory */
	if (shm_addr != NULL)
		if (munmap(shm_addr, SIZE) == -1)
			ret = -1;

	/* unlink it */
	if (md > 0)
		if (shm_unlink(shm_name) == -1)
			ret = -1;

	/* unlink all queues */
	int i;
	char queue_name[MAX_QUEUE_NAME];

	for (i = 0; i < num_queues; i++){
		snprintf(queue_name, MAX_QUEUE_NAME, "%s%d", BASE_QUEUE_NAME, i);
		if (mq_unlink(queue_name) == -1)
			ret = -1;
	}

	/* unlink shm semaphore */
	if (sem_unlink(sem_name) == -1)
		ret = -1;

	/* unlink queue semaphores */
	char s_name[MAX_SEM_NAME];

	for (i = 0; i < num_sems; i++){
		snprintf(s_name, MAX_SEM_NAME, "%s%d", BASE_SEM_NAME, i);
		if (sem_unlink(s_name) == -1)
			ret = -1;
	}

	return ret;
}
