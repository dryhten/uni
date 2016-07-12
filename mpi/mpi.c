/**
*	A2 SO - Minimal MPI
*	Dunaev Mihail, 331CC
*	10 Apr 2013
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <mqueue.h>
#include <semaphore.h>

#include "mpi.h"
#include "protocols.h"

typedef enum{
true,
false
} bool;

struct mpi_comm{
/* NULL */
};

MPI_Comm mpi_comm_world = NULL;

char* addr;                             /* address of shared memory */
bool init = false,                      /* set to true after Init() was called */
     fin = false;                       /* set to true after Finalize() was called */
int m_rank;                             /* rank of process in MPI_COMM_WORLD */
int m_size;                             /* size of MPI_COMM_WORLD */
const char* shm_name = "/mpi";          /* filename on disk */
const char* sem_name = "/sem_rank";     /* it's like a protocol, mpirun & mpilib come from same dev */
mqd_t* queues;                          /* all queues */
sem_t** sems;                           /* all semaphores */
char m_buffer[MAX_MSG_SIZE];            /* used in MPI_Recv() */

int DECLSPEC MPI_Init (int *argc, char ***argv){

	/* already called */
	if (init == true)
		return MPI_ERR_OTHER;

	/* ignore argc, argv */

	/* get a file descriptor to mpi shared memory */
	int md = shm_open(shm_name, O_RDWR, 0644);

	if (md == -1)
		return MPI_ERR_IO;

	/* map it into the virtual memory */
	addr = (char*) mmap(NULL, SIZE, PROT_WRITE, MAP_SHARED, md, 0); /* OFFSET_START */

	if (addr == MAP_FAILED)
		return MPI_ERR_IO;
	
	/* get and save size */
	int* ptr_int = (int*)(addr + OFFSET_SIZE);
	m_size = *(ptr_int);

	if (m_size < 1)
		return MPI_ERR_IO;

	/* open semaphore to get rank */
	sem_t* sem = sem_open(sem_name, 0);

	if (sem == SEM_FAILED)
		return MPI_ERR_IO;

	/* get & increment rank */
	ptr_int = (int*)(addr + OFFSET_RANK);
	sem_wait(sem);
	m_rank = *(ptr_int);
	(*ptr_int)++;
	sem_post(sem);

	if ((m_rank < 0) || (m_rank >= m_size))
		return MPI_ERR_IO;

	if (sem_close(sem) == -1)
		return MPI_ERR_IO;

	/* get queues info */
	char queue_name[MAX_QUEUE_NAME];
	queues = malloc(m_size * sizeof(mqd_t));

	if (queues == NULL)
		return MPI_ERR_IO;

	/* open your queue for reading - can't send yourself a msg */
	snprintf(queue_name, MAX_QUEUE_NAME, "%s%d", BASE_QUEUE_NAME, m_rank);
	queues[m_rank] = mq_open(queue_name, O_RDONLY);

	if (queues[m_rank] == (mqd_t)-1)
		return MPI_ERR_IO;

	/* open other queues for writing */
	int i;

	for (i = 0; i < m_rank; i++){

		snprintf(queue_name, MAX_QUEUE_NAME, "%s%d", BASE_QUEUE_NAME, i);
		queues[i] = mq_open(queue_name, O_WRONLY);

		if (queues[i] == (mqd_t)-1)
			return MPI_ERR_IO;
	}

	for (i = (m_rank+1); i < m_size; i++){

		snprintf(queue_name, MAX_QUEUE_NAME, "%s%d", BASE_QUEUE_NAME, i);
		queues[i] = mq_open(queue_name, O_WRONLY);

		if (queues[i] == (mqd_t)-1)
			return MPI_ERR_IO;
	}

	/* open queue semaphores */
	char s_name[MAX_SEM_NAME];
	sems = malloc(m_size * sizeof(sem_t*));

	if (sems == NULL)
		return MPI_ERR_IO;

	for (i = 0; i < m_size; i++){
		
		snprintf(s_name, MAX_SEM_NAME, "%s%d", BASE_SEM_NAME, i);
		sems[i] = sem_open(s_name, 0);

		if (sems[i] == SEM_FAILED)
			return MPI_ERR_IO;
	}

	init = true;
	return MPI_SUCCESS;
}

int DECLSPEC MPI_Initialized (int *flag){

	if (flag == NULL)
		return MPI_ERR_OTHER;

	if (init == true)
		(*flag) = 1;
	else
		(*flag) = 0;

	return MPI_SUCCESS;
}

int DECLSPEC MPI_Finalize (void){

	int ret;

	if (init == false || fin == true)
		return MPI_ERR_OTHER;

	/* unmap */
	ret = munmap(addr, SIZE);

	if (ret == -1)
		return MPI_ERR_IO;

	/* close queues */
	int i;

	for (i = 0; i < m_size; i++)
		if (mq_close(queues[i]) == -1)
			return MPI_ERR_IO;

	free(queues);

	/* close sems */
	for (i = 0; i < m_size; i++)
		if (sem_close(sems[i]) == -1)
			return MPI_ERR_IO;

	free(sems);

	fin = true;
	return MPI_SUCCESS;
}

int DECLSPEC MPI_Finalized (int *flag){

	if (flag == NULL)
		return MPI_ERR_OTHER;

	if (fin == true)
		(*flag) = 1;
	else
		(*flag) = 0;

	return MPI_SUCCESS;
}

int DECLSPEC MPI_Comm_size (MPI_Comm comm, int *size){

	if (init == false || fin == true)
		return MPI_ERR_OTHER;

	if (comm != MPI_COMM_WORLD)
		return MPI_ERR_COMM;

	if (size == NULL)
		return MPI_ERR_OTHER;

	(*size) = m_size;
	return MPI_SUCCESS;
}

int DECLSPEC MPI_Comm_rank (MPI_Comm comm, int *rank){

	if (init == false || fin == true)
		return MPI_ERR_OTHER;

	if (comm != MPI_COMM_WORLD)
		return MPI_ERR_COMM;

	if (rank == NULL)
		return MPI_ERR_OTHER;

	(*rank) = m_rank;
	return MPI_SUCCESS;
}


int DECLSPEC MPI_Send (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm){

	if (init == false || fin == true)
		return MPI_ERR_OTHER;

	if (comm != MPI_COMM_WORLD)
		return MPI_ERR_COMM;
	
	if ((dest < 0) || (dest >= m_size) || (dest == m_rank))
		return MPI_ERR_RANK;

	if (buf == NULL)
		return MPI_ERR_IO;

	int size, ret;

	if (datatype == MPI_CHAR)
		size = sizeof (char);
	else if (datatype == MPI_INT)
		size = sizeof (int);
	else if (datatype == MPI_DOUBLE)
		size = sizeof (double);
	else
		return MPI_ERR_TYPE;

	/* get auxiliary buffer */
	char* s_buffer = malloc((count*size) + (2*sizeof(int)));

	/* add source & tag to it */
	memcpy(s_buffer, &m_rank, sizeof(int));
	/* priority is short int, so I prefer this method */
	memcpy(s_buffer + sizeof(int), &tag, sizeof(int));
	/* payload */
	memcpy(s_buffer + (2*sizeof(int)), buf, (count*size));

	/* wait for dest to make a Recv() */
	sem_wait(sems[dest]);
	ret = mq_send(queues[dest], s_buffer, (size*count) + (2*sizeof(int)), 0);
	free(s_buffer);
	
	if (ret == -1)
		return MPI_ERR_IO;

	return MPI_SUCCESS;
}

int DECLSPEC MPI_Recv (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status){

	if (init == false || fin == true)
		return MPI_ERR_OTHER;

	if (comm != MPI_COMM_WORLD)
		return MPI_ERR_COMM;
	
	if ((source != MPI_ANY_SOURCE) || (source == m_rank))
		return MPI_ERR_RANK;

	if (buf == NULL)
		return MPI_ERR_IO;

	if (tag != MPI_ANY_TAG)
		return MPI_ERR_TAG;

	if ((datatype != MPI_CHAR) && (datatype != MPI_INT) && (datatype != MPI_DOUBLE))
		return MPI_ERR_TYPE;

	int s_source, s_tag;
	ssize_t ret;

	/* let one sender send the message */
	sem_post(sems[m_rank]);
	ret = mq_receive(queues[m_rank], m_buffer, MAX_MSG_SIZE, NULL); /* (size*count) */
	
	if (ret == -1)
		return MPI_ERR_IO;

	/* get source */
	memcpy(&s_source, m_buffer, sizeof(int));
	/* tag */
	memcpy(&s_tag, m_buffer + sizeof(int), sizeof(int));
	/* payload */
	memcpy(buf, m_buffer + (2*sizeof(int)), ret - (2*sizeof(int)));

	if (status != MPI_STATUS_IGNORE){
		status->MPI_SOURCE = s_source;
		status->MPI_TAG = s_tag;
		status->_size = ret - (2*sizeof(int));
	}
	
	return MPI_SUCCESS;
}

int DECLSPEC MPI_Get_count (MPI_Status *status, MPI_Datatype datatype, int *count){

	if (init == false || fin == true)
		return MPI_ERR_OTHER;

	if ((status == NULL) || (status == MPI_STATUS_IGNORE))
		return MPI_ERR_OTHER;

	int size;

	if (datatype == MPI_CHAR)
		size = sizeof (char);
	else if (datatype == MPI_INT)
		size = sizeof (int);
	else if (datatype == MPI_DOUBLE)
		size = sizeof (double);
	else
		return MPI_ERR_TYPE;

	(*count) = (status->_size / size);
	return MPI_SUCCESS;
}
