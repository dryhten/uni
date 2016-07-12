/**
* A3 PC - Chat and file sharing system
* Mihail Dunaev
* 12 May 2013
*
* general.h - general defines & adts
*/

/**
* compile with LOG for more info
*/

#ifdef LOG
#define lprintf(msg,...)  printf(msg, ##__VA_ARGS__)
#else
#define lprintf(msg,...)               /* do nothing */
#endif

/**
* the popular DIE macro
*/

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)

/**
* polymorphic min, max
*/

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

/**
* server respones (on connect, ...)
*/

#define SERV_SUCCESS 0    /* success on connecting */
#define SERV_FULL 1       /* server is full */
#define SERV_NAME 2       /* name already exists */
#define SERV_FAIL 3       /* any other type of failure */
#define SERV_KICK 4       /* kicked by admin */
#define SERV_SAY 5        /* admin chat */

#define CLI_LIST 5
#define CLI_INFO 6
#define CLI_CNT 7
#define CLI_BCAST 8

#define SERV_CNT_OK 9
#define SERV_CNT_EXIST 10  /* name doesn't exist */
#define SERV_CNT_FAIL 11   /* generic fail */

#define CHAT_MSG 12     /* client-client communication */
#define FILE_REQ 13     /* request to send a file */
#define FILE_ACCEPT 14  /* accept req */
#define FILE_REJECT 15  /* ... */
#define FILE_BUSY 16    /* busy with other transfer */

#define BUFLEN 256
#define MAX_CMD_LEN 100
#define GENERIC_FAIL_CODE -1
#define MAX_NAME_LEN 100        /* 1 byte reserved for '\0' */
#define MAX_CLIENTS 100
#define MAX_DIGITS_PORT 6
#define MAX_FILENAME_SIZE 100   /* should be less than BUFLEN */

#define CHUNK 1024
#define RECV_FILE_ENDNAME "_primit"
#define HISTORY_BUFFLEN 300

typedef enum{
true,
false
} boolean;

typedef struct{
  int fd;
  struct sockaddr_in info;
  socklen_t len;
  char name[MAX_NAME_LEN];
  short int port;
  time_t time;
} client_s;
