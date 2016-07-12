/**
* A3 PC - Chat and file sharing system
* Mihail Dunaev
* 12 May 2013
*
* client.c - code for client
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "general.h"

/* does a select on global fds until timeout (inf if NULL) */
void mini_select(struct timeval* timeout);

/* handles user command */
void handle_command(void);

/* handles new messages from connected clients */
void handle_message(int fd);

/* handles response from server */
void handle_response(void);

/* handles new connections to local port */
void handle_new_connection(void);

/* receives file_size bytes from client at index and writes them to fd */
int receive_file(int index, int fd, int file_size);

/* sends file with fd to client at index */
int send_file(int index, int fd, int file_size);

/* sends message to client_name */
void send_message(char* client_name, int name_len, char* message, int m_len);

/* trie to connect to client */
int connect_to_client(char* client_name, int* aux_index, int len);

/* request client list from server and saves it in buffer */
int request_client_list(char* buffer, int max);

/* init stuff */
void init(char* argv[]);

/* gets index of client with name */
int get_index_from_name(char* name);

/* gets index of client with fd */
int get_index_from_fd(int fd);

/* returns index of an empty spot in clients */
int get_free_index(void);

/* reads num_bytes from a socket */
int read_from_socket(int sd, char* buffer, int num_bytes);

/* does garbage collecting - close sockets etc */
void free_everything(void);

/* data */
int fdmax;
fd_set fds;                       /* read fds considered in select */
int serverfd, localfd;            /* file descriptors */
int history_fd;                   /* history fd */
int num_clients = 0;              /* connected clients */
boolean stop = false;             /* if true, client stops */
boolean transf = false;           /* if there's a file transfer going on */
boolean show_transf_info = true;  /* verbose transfer */
char name[MAX_NAME_LEN];          /* own name */
int receiver_index;               /* receiver of a file */
char file_buffer[CHUNK];          /* big buffer for file transfer */
client_s clients[MAX_CLIENTS];    /* list of connected clients */
struct sockaddr_in server_info,   /* server info */
                    local_info;   /* connections to local machine */

int main (int argc, char* argv[]){

  /* ./client [name] [local port] [ip serv] [port serv] */
  if (argc < 5){
    printf("Usage : %s [name] [local port] [ip serv] [port serv]\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  
  /* init */
  init (argv);
  lprintf("\n");
     
  /* main loop */
  mini_select(NULL);
  
  /* close everything */
  free_everything();

  return 0;
}

void mini_select (struct timeval* timeout){

  /* data */
  int ret, i;
  fd_set aux_fds;

  while (stop == false){
    
    /* save fd set */
    aux_fds = fds;
    
    /* wait until a new message */
    ret = select(fdmax + 1, &aux_fds, NULL, NULL, timeout);
    if (ret < 0){
      perror("Error in select");
      return;
    }
    
    if (ret == 0)  /* timeout expired */
      return;

    /* check which descriptor */
    for (i = 0; i <= fdmax; i++)
      if (FD_ISSET(i, &aux_fds)){
        if (i == serverfd){  /* from server */
          handle_response();
          lprintf("\n");
        }
        else if (i == STDIN_FILENO){  /* from user */
          handle_command();
          lprintf("\n");
        }
        else if (i == localfd){  /* on local port */
          handle_new_connection();
          lprintf("\n");
        }
        else{  /* from connected clients */
          handle_message(i);
        }
      }
  }
  
  /**
  * if you get kicked while transfering a file
  * just keep doing it; who cares about server anyway?
  * you'll stop in the main loop
  */
}

void handle_command (void){

  /* get command from stdin */
  int ret, bytes_to_read;
  char buffer[MAX_CMD_LEN];
  char aux_buffer[MAX_CMD_LEN];
  fgets(buffer, MAX_CMD_LEN, stdin);
  
  /* save it in aux_buffer (strtok ruins everything) */
  strncpy(aux_buffer, buffer, strlen(buffer) + 1);
  char* token = strtok(buffer, " \n");
  
  if (token == NULL)  /* just empty spaces */
    return;

  /* parse it */
  if ((strcmp(token, "quit") == 0) ||
      (strcmp(token, "q") == 0)){
    
    if (transf == true){
      printf("There is a transfer going on;");
      printf("Are you sure you want to quit?\n\n");
      /* TODO */
      return;
    }
    
    stop = true;
  }
  else if ((strcmp(token, "listclients") == 0) ||
            (strcmp(token, "list") == 0) ||
            (strcmp(token, "l") == 0)){

    /* request it */
    ret = request_client_list(buffer, BUFLEN);
    if (ret == GENERIC_FAIL_CODE)
      return;
    
    /* print it */
    printf("\n");
    printf("Connected clients :\n%s", buffer);
  }
  else if ((strcmp(token, "infoclient") == 0) ||
            (strcmp(token, "info") == 0) ||
            (strcmp(token, "i") == 0)){
  
    /* get client name */
    token = strtok(NULL, " \n");
    
    if (token == NULL){
      printf("\n");
      printf("Usage : infoclient [client name]\n");
      return;
    }
    
    int ns = strlen(token);
    
    if (ns >= MAX_NAME_LEN){
      fprintf(stderr, "\n");
      fprintf(stderr, "Error : name too long\n");
      return;
    }
    
    /* apparently you can't send static vars */
    char client_name[MAX_NAME_LEN];
    strncpy(client_name, token, ns);
    client_name[ns] = '\0';
    
    /* send info request */
    *((int*)buffer) = CLI_INFO;
    *((int*)(buffer + sizeof(int))) = ns;
    send(serverfd, buffer, 2 * sizeof(int), 0);
    send(serverfd, client_name, ns, 0);
    
    /* wait for response */
    bytes_to_read = sizeof(int);
    ret = read_from_socket(serverfd, buffer, bytes_to_read);
    if (ret <= 0){
      perror("Error receiving info");
      return;
    }
    
    bytes_to_read = *((int*)buffer);
    if (bytes_to_read >= BUFLEN){
      fprintf(stderr, "Error : info string is too long (more than %d chars)\n",
               BUFLEN);
      return;
    }
    
    ret = read_from_socket(serverfd, buffer, bytes_to_read);
    if (ret <= 0){
      perror("Error receiving info");
      return;
    }
    
    /* print it */
    buffer[ret] = '\0';
    printf("\n");
    printf("%s", buffer);
  }
  else if ((strcmp(token, "message") == 0) ||
            (strcmp(token, "msg") == 0) ||
            (strcmp(token, "m") == 0)){
  
    /* get name & message */
    token = strtok(NULL, " \n");
    
    if (token == NULL){
      printf("\n");
      printf("Usage : message [client name] [message]\n");
      return;
    }
    
    int name_len = strlen(token);
    if (name_len >= MAX_NAME_LEN){
      fprintf(stderr, "\n");
      fprintf(stderr, "Error : name too long\n");
      return;
    }

    char client_name[MAX_NAME_LEN];
    strncpy(client_name, token, name_len);
    client_name[name_len] = '\0';
    
    token = strtok(NULL, " ");
    if (token == NULL){
      printf("\n");
      printf("Usage : message [client name] [message]\n");
      return;
    }

    char* message = strstr (aux_buffer, token);
    send_message(client_name, name_len, message, strlen(message));
  }
  else if ((strcmp(token, "broadcast") == 0) ||
            (strcmp(token, "bc") == 0) ||
            (strcmp(token, "b") == 0)){
  
    /* get message */
    token = strtok(NULL, " ");
    if (token == NULL){
      printf("\n");
      printf("Usage : broadcast [message]\n");
      return;
    }

    char* message = strstr(aux_buffer, token);
  
    /* get list of connected clients */
    ret = request_client_list(buffer, BUFLEN);
    if (ret == GENERIC_FAIL_CODE)
      return;

    /* parse it & send message to each */
    char* save_ptr;
    token = strtok_r(buffer, "\n", &save_ptr);
    while (token != NULL){
      if (strcmp(token, name) != 0)  /* don't send yourself a message */
        send_message(token, strlen(token), message, strlen(message));
      token = strtok_r(NULL, "\n", &save_ptr);
    }
  }
  else if ((strcmp(token, "sendfile") == 0) ||
            (strcmp(token, "send") == 0) ||
            (strcmp(token, "s") == 0)){
  
    /* quick return */
    if (transf == true){
      printf("\n");
      printf("You are already transfering a file. ");
      printf("Wait for the transfer to finsih\n");
      return;
    }
  
    char filename[MAX_FILENAME_SIZE];
    char client_name[MAX_NAME_LEN];
    int fd, index;
    
    /* save name */
    token = strtok(NULL, " ");
    if (token == NULL){
      printf("\n");
      printf("Usage : sendfile [client name] [filename]\n");
      return;
    }
    
    ret = strlen(token);
    strncpy(client_name, token, ret);
    client_name[ret] = '\0';  
    
    /* open file - faster return */
    token = strtok(NULL, " \n");
    if (token == NULL){
      printf("\n");
      printf("Usage : sendfile [client name] [filename]\n");
      return;
    }
    
    ret = strlen(token);
    strncpy(filename, token, ret);
    filename[ret] = '\0';
    
    fd = open(filename, O_RDONLY);  /* don't creat */
    if (fd < 0){
      perror("\nError : could not open file");
      return;
    }

    /* get index to client */
    index = get_index_from_name(client_name);
    if (index == -1){  /* new client - try to connect */
      ret = connect_to_client(client_name, &index, strlen(client_name));
      if (ret == GENERIC_FAIL_CODE){
        close(fd);
        return;
      }
    }

    /* send file request - TODO one send */
    int req = FILE_REQ;
    ret = send(clients[index].fd, &req, sizeof(int), 0);
    if (ret < 0){
      perror("\nError : could not send file request");
      close(fd);
      return;
    }
    
    /* send file info & sendfile flag */
    struct stat file_stat;
    ret = fstat(fd, &file_stat);
    
    if (ret < 0){
      perror("\nError : could not read file stat");
      close(fd);
      return;
    }
    
    /* send filename & file size */
    int offset = 0;
    *((int*)buffer) = file_stat.st_size;
    offset += sizeof(int);
    *((int*)(buffer + offset)) = (int) strlen (filename);
    offset += sizeof(int);
    strncpy(buffer + offset, filename, strlen(filename));
    ret = send(clients[index].fd, buffer,
               (2 * sizeof(int)) + strlen(filename), 0);

    if (ret < 0){
      perror("\nError : could not send file request");
      close(fd);
      return;
    }
    
    /* wait for user confirmation */
    printf("\nWaiting for confirmation ... ");
    fflush(stdout);
    bytes_to_read = sizeof(int);
    ret = read_from_socket(clients[index].fd, (char*) &req, bytes_to_read);
    if (ret <= 0){
      perror("\nError receiving request response");
      close(fd);
      return;
    }

    if (req == FILE_REJECT){
      printf("%s rejected file transfer\n", clients[index].name);
      close(fd);
      return;
    }
    
    if (req == FILE_BUSY){
      printf("%s is busy with another file transfer", clients[index].name);
      close(fd);
      return;
    }
    
    if (req != FILE_ACCEPT){
      printf("\nError receiving request response");
      close(fd);
      return;
    }

    printf("%s accepted your transfer\n", clients[index].name);
    send_file(index, fd, (int)file_stat.st_size);
    close(fd);
  }
  else if ((strcmp(token, "history") == 0) ||
            (strcmp(token, "hist") == 0) ||
            (strcmp(token, "h") == 0)){
  
    /* go to beginning  */
    lseek(history_fd, 0, SEEK_SET);
    
    /* read everything and print */
    while (1){  
      ret = read(history_fd, buffer, BUFLEN-1);
      if (ret == 0)
        break;
      buffer[ret] = '\0';
      printf("%s", buffer);
    }
  }
}

void handle_message (int fd){

  int ret, bytes_to_read, index;
  char buffer[BUFLEN];
  time_t time_now;
  struct tm* time_now_s;
  char time_now_string[BUFLEN];
  
  index = get_index_from_fd(fd);
  if (index < 0){
    fprintf(stderr, "Error : unknown sender\n\n");
    return;
  }
  
  bytes_to_read = sizeof(int);
  ret = read_from_socket(fd, buffer, bytes_to_read);
  if (ret < 0){
    perror("Error receiving message type");
    fprintf(stderr, "\n");
    return;
  }
  
  if (ret == 0){  /* client disconnected */
    printf("Client %s disconnected\n\n", clients[index].name);
    close(fd);
    FD_CLR(fd, &fds);
    num_clients--;
    clients[index].fd = 0;
    return;
  }
  
  ret = *((int*)buffer);
  
  if (ret == CHAT_MSG){
  
    /* read message len */
    bytes_to_read = sizeof(int);
    ret = read_from_socket(fd, buffer, bytes_to_read);  
    if (ret < 0){
      perror("Error receiving message len");
      fprintf(stderr, "\n");
      return;
    }
    
    /* check it */
    bytes_to_read = *((int*)buffer);
    if (bytes_to_read >= BUFLEN){
      fprintf(stderr, "Error : message is too long (more than %d chars)\n\n",
               BUFLEN);
      return;
    }

    /* read message */
    ret = read_from_socket(fd, buffer, bytes_to_read);
    if (ret <= 0){
      perror("Error receiving message");
      printf("\n");
      return;
    }
    
    /* format time */
    time_now = time(NULL);
    time_now_s = localtime(&time_now);
    strftime(time_now_string, BUFLEN, "%T", time_now_s);
    
    /* print it */
    buffer[ret] = '\0';
    printf("[%s][%s] %s\n", time_now_string, clients[index].name, buffer);
    
    /* save it in history */
    char history_buff[HISTORY_BUFFLEN];
    snprintf(history_buff, BUFLEN, "[%s][%s] %s", time_now_string,
                    clients[index].name, buffer);
    write(history_fd, history_buff, strlen(history_buff));
    /* check return values ... maybe some other time */
  }
  else if (ret == FILE_REQ){
  
    /* read file size, strlen (filename) & filename  */
    int file_size, res, name_size;
    char filename[MAX_FILENAME_SIZE];
  
    if (transf == true){  /* busy ... */
      res = FILE_BUSY;
      send(clients[index].fd, &res, sizeof(int), 0);
      return;
    }
  
    bytes_to_read = sizeof(int);
    ret = read_from_socket(fd, (char*) &file_size, bytes_to_read);
    if (ret < 0){
      perror("Error receiving file size");
      fprintf(stderr, "\n");
      return;
    }
    
    ret = read_from_socket(fd, (char*) &name_size, bytes_to_read);
    if (ret < 0){
      perror("Error receiving filename size");
      fprintf(stderr, "\n");
      return;
    }
    
    //bytes_to_read = *((int*)buffer);
    if (name_size > MAX_FILENAME_SIZE){
      fprintf(stderr, "Error : filename too big");
      return;
    }
    
    ret = read_from_socket(fd, filename, name_size);
    if (ret < 0){
      perror("Error receiving filename");
      fprintf(stderr, "\n");
      return;
    }
    
    filename[ret] = '\0';
    
    /* ask user */
    printf("%s is trying to send you a file : %s (%d bytes)\n",
            clients[index].name, filename, file_size);
    printf("accept ? [y/n] ");
    fflush(stdout);
    fgets(buffer, BUFLEN, stdin);  /* size must be > 3 */

    /* send accept / reject */
    if ((buffer[0] != 'y') && (buffer[0] != 'Y')) /* TODO loop */
      buffer[0] = 'n';
    
    if ((buffer[0] == 'n') || (buffer[0] == 'N')){
      res = FILE_REJECT;
      printf ("Rejecting ...\n\n");
      send(clients[index].fd, &res, sizeof(int), 0);
      return;
    }

    res = FILE_ACCEPT;
    printf("Accepting ...\n\n");
    ret = send(clients[index].fd, &res, sizeof(int), 0);
    if (ret < 0){
      printf("Error notifying %s\n", clients[index].name);
      return;
    }

    /* open file to write to */
    strncat(filename, RECV_FILE_ENDNAME, MAX_FILENAME_SIZE - name_size - 1);
    int wd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (wd < 0){
      perror("Error opening file");
      fprintf(stderr, "\n\n");
    }
    
    ret = receive_file(index, wd, file_size);
    if (ret == EXIT_SUCCESS){
      snprintf(buffer, BUFLEN, "Received file %s (%d bytes) from %s\n",
                filename, file_size, clients[index].name);
      write(history_fd, buffer, strlen(buffer));
    }
    
    close(wd);
  }
}

void handle_response (void){

  int ret, bytes_to_read, code;
  char buffer[BUFLEN];
  
  bytes_to_read = sizeof(int);
  ret = read_from_socket(serverfd, (char*) &code, bytes_to_read);
  
  if (ret < 0){  /* ignore */
    perror("Error receiving message from server");
  }
  else if (ret == 0){  /* server closed the connection */    
    printf("Server closed the connection\n");
    FD_CLR(serverfd, &fds); /* will close it later */
    stop = true;
  }
  else{  /* parse message */
  
    if (code == SERV_KICK){
      printf("You got kicked from the server. Reason : ");
      ret = read_from_socket(serverfd, (char*) &code, bytes_to_read);
      if (ret < 0){
        perror("Error receiving reason len");
        return;
      }
      
      bytes_to_read = code;
      if (bytes_to_read == 0){
        printf("No Reason\n");
        return;
      }
      
      if (bytes_to_read >= BUFLEN){
        fprintf(stderr, "Error : reason is too long\n");
        return;
      }
      
      ret = read_from_socket(serverfd, buffer, bytes_to_read);
      if (ret < 0){
        perror("Error receiving reason");
        return;
      }
      
      buffer[ret] = '\0';
      printf("%s", buffer);
    }
    else if (code == SERV_SAY){
      ret = read_from_socket(serverfd, (char*) &code, bytes_to_read);
      if (ret < 0){
        perror("Error receiving message len");
        return;
      }
      
      bytes_to_read = code;
      if (bytes_to_read >= BUFLEN){
        fprintf(stderr, "Error : message is too long\n");
        return;
      }
      
      ret = read_from_socket(serverfd, buffer, bytes_to_read);
      if (ret < 0){
        perror("Error receiving message");
        return;
      }
      
      buffer[ret] = '\0';
      printf("[Admin] %s", buffer);
    }
  }
}

void handle_new_connection (void){

  int ret, bytes_to_read, index;
  client_s new_client;                  /* buffer for new connections */
  char buffer[BUFLEN];
  
  lprintf("New connection ");
  
  /* accept it first */
  new_client.len = (socklen_t)sizeof(new_client.info);
  new_client.fd = accept(localfd, (struct sockaddr*) &new_client.info,
                          &new_client.len);
  lprintf("from ip %s ... ", inet_ntoa(new_client.info.sin_addr));

  /* just one client, ignore */
  if (new_client.fd < 0){
    perror("Error in accept\n");
    return;
  }

  /* no need to check load - already done in server */
  
  /* get name len */
  bytes_to_read = sizeof(int);
  ret = read_from_socket(new_client.fd, buffer, bytes_to_read);
  if (ret <= 0){
    perror("Error receiving name len");
    close(new_client.fd);
    return;
  }
  
  /* check name len */
  bytes_to_read = *((int*)buffer);
  if (bytes_to_read >= MAX_NAME_LEN){
    lprintf("Name too long. Rejecting\n");
    close(new_client.fd);
    return;
  }
  
  /* name */
  ret = read_from_socket(new_client.fd, new_client.name, bytes_to_read);
  if (ret <= 0){
    perror("Error receiving name");
    close(new_client.fd);
    return;
  }
  
  /* no need to check name */
  new_client.name[bytes_to_read] = '\0';
  lprintf("%s\n", new_client.name);

  /* save client info */
  index = get_free_index();
  if (index < 0){ /* should not happen */
    lprintf("Error obtaining index. Rejecting\n");
    close(new_client.fd);
    return;
  }

  clients[index] = new_client;
  clients[index].time = time (NULL);
  FD_SET(clients[index].fd, &fds);
  num_clients++;
  if (clients[index].fd > fdmax)
    fdmax = clients[index].fd;
}

int receive_file (int index, int fd, int file_size){

  /* sanity checks */
  if ((index < 0) || (fd < 0) || (file_size < 0)){
    fprintf(stderr, "Bad parameters for \"send_file\" call\n");
    fprintf(stderr, "index = %d fd = %d file_size = %d\n",
             index, fd, file_size);
    return GENERIC_FAIL_CODE;
  }

  /* quick return */
  if (file_size == 0)
    return EXIT_SUCCESS;
  
  int ret, bytes_to_read = CHUNK, written_bytes;
  transf = true;
  struct timeval timeout;
  
  char clear_line[40];            /* should define this ... */
  memset(clear_line, ' ', 40);
  int it = 0;
  
  /* remove client fd from fds - select would mess this up */
  FD_CLR(clients[index].fd, &fds);
  
  /* read file size bytes and save them */
  while (1){

    /* show info on transfer each 200 KB */
    if ((show_transf_info == true) && ((it % 200) == 0)){
      printf("\r");
      printf("%s", clear_line);
      printf("\r");
      printf("Remaining file size ... %d KB ", file_size/1024);
      fflush(stdout);
    }
    
    if (file_size < CHUNK)
      bytes_to_read = file_size;

    ret = read_from_socket(clients[index].fd, file_buffer, bytes_to_read);
    if (ret <= 0){  /* should take care of the case when connection is lost */
      perror("Error receiving file CHUNK");
      break;
    }

    written_bytes = 0;
    
    do{
      ret = write(fd, file_buffer, bytes_to_read);
      if (ret < 0){
        perror("Error writing to file");
        break;
      }
      written_bytes = written_bytes + ret;
    } while (written_bytes < bytes_to_read);

    if (written_bytes < bytes_to_read)
      break;

    file_size = file_size - bytes_to_read;
    
    if (file_size == 0)  /* done */
      break;
    
    /* set timeout in select */
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; /* 10 ms */
    
    /* do a mini select of timeout */
    mini_select(&timeout);
    it++;
  }

  printf("\r");
  printf("%s", clear_line);
  printf("\r");  
  printf("Transfer completed\n\n");
  
  FD_SET(clients[index].fd, &fds);  /* add descriptor back */
  transf = false;
  
  if (file_size == 0)
    return EXIT_SUCCESS;

  return GENERIC_FAIL_CODE;
}

int send_file (int index, int fd, int file_size){

  /* sanity checks */
  if ((index < 0) || (fd < 0)){
    fprintf(stderr, "Bad parameters for \"send_file\" call\n");
    return GENERIC_FAIL_CODE;
  }

  int ret;
  transf = true;
  receiver_index = index;
  struct timeval timeout;
  
  char clear_line[40];
  memset(clear_line, ' ', 40);
  int it = 0;
  
  /* in a loop : read CHUNK, write, select with timeout */
  while (1){

    /* show info on transfer each 200 KB */
    if ((show_transf_info == true) && ((it % 200) == 0)){
      printf("\r");
      printf("%s", clear_line);
      printf("\r");
      printf("Remaining file size ... %d KB ", file_size/1024);
      fflush(stdout);
    }
  
    ret = read(fd, file_buffer, CHUNK);
    if (ret < 0){
      perror("Error reading from file");
      break;
    }
    
    if (ret == 0)  /* done */
      break;

    file_size = file_size - ret;
    ret = send(clients[index].fd, file_buffer, ret, 0);
    if (ret < 0){
      perror("Could not send file chunk");
      printf("\n");
      break;
    }
    
    /* set timeout in select */
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; /* 10 ms */
    
    /* do a mini select */
    mini_select(&timeout);
    it++;
  }

  printf("\r");
  printf("%s", clear_line);
  printf("\r");  
  printf("\nTransfer completed\n");
  transf = false;
  receiver_index = -1;
  
  if (ret == 0)
    return EXIT_SUCCESS;
  
  return GENERIC_FAIL_CODE;
}

void send_message (char* client_name, int name_len, char* message, int m_len){

  /* quick return */
  if (m_len > (BUFLEN - (2 * sizeof(int)))){
    fprintf(stderr, "Error : message too long\n\n");
    return;
  }

  int index, ret;
  char buffer[BUFLEN];  /* TODO malloc it */

  /* iterate through clients */
  index = get_index_from_name(client_name);
  
  if (index == -1){  /* new client - try to connect */
    ret = connect_to_client(client_name, &index, name_len);
    if (ret == GENERIC_FAIL_CODE)
      return;
  }

  if ((transf == true) && (receiver_index == index)){
    printf("You are sending a file to %s\n", client_name);
    printf("Wait for transfer to finish!\n");
    return;
  }

  /* send message */
  *((int*)buffer) = CHAT_MSG;
  *((int*)(buffer + sizeof(int))) = m_len;
  memcpy(buffer + (2 * sizeof(int)), message, m_len);

  ret = send(clients[index].fd, buffer, m_len + (2 * sizeof(int)), 0);
  if (ret < 0){
    perror("Could not send message");
    printf("\n");
    return;
  }
}

int connect_to_client (char* client_name, int* aux_index, int len){

  /* data */
  int bytes_to_read, ret, index;
  char buffer[BUFLEN];
  char* token;

  /* ask server for port & ip */
  *((int*)buffer) = CLI_CNT;
  *((int*)(buffer + sizeof(int))) = len;
  send(serverfd, buffer, 2 * sizeof(int), 0);
  send(serverfd, client_name, len, 0);

  /* wait for response */
  bytes_to_read = sizeof(int);
  ret = read_from_socket(serverfd, buffer, bytes_to_read);
  if (ret <= 0){
    perror("Error receiving info");
    return GENERIC_FAIL_CODE;
  }

  /* check response - client exists etc */
  ret = *((int*)buffer);
  if (ret == SERV_CNT_EXIST){
    printf("Error : client \"%s\" doesn't exist\n", client_name);
    return GENERIC_FAIL_CODE;
  }
  
  if (ret != SERV_CNT_OK){
    printf("Error : bad response from server\n");
    return GENERIC_FAIL_CODE;
  }

  /* get (port & ip) buffer size */
  bytes_to_read = sizeof(int);
  ret = read_from_socket(serverfd, buffer, bytes_to_read);
  if (ret <= 0){
    perror("Error receiving info");
    return GENERIC_FAIL_CODE;
  }
    
  bytes_to_read = *((int*)buffer);
  if (bytes_to_read >= BUFLEN){
    printf("Error : info string is too long (more than %d chars)\n",
             BUFLEN);
    return GENERIC_FAIL_CODE;
  }

  /* get actual needed data */
  ret = read_from_socket(serverfd, buffer, bytes_to_read);
  if (ret <= 0){
    perror("Error receiving info");
    return GENERIC_FAIL_CODE;
  }
  
  /* parse it */
  buffer[ret] = '\0';
  token = strtok(buffer, " ");
  if (token == NULL){
    printf("Received info is corrupted\n");
    return GENERIC_FAIL_CODE;
  }

  /* connect to him */
  index = get_free_index();
  clients[index].info.sin_family = AF_INET;
  clients[index].port = atoi(token);
  clients[index].info.sin_port = htons(clients[index].port);

  token = strtok(NULL, " \n");
  if (token == NULL){
    printf("Received info is corrupted\n");
    return GENERIC_FAIL_CODE;
  }

  ret = inet_aton(token, &clients[index].info.sin_addr);
  if (ret == 0) {
    perror("Error in inte_aton");
    return GENERIC_FAIL_CODE;
  }

  clients[index].fd = socket(AF_INET, SOCK_STREAM, 0);
  if (clients[index].fd < 0) {
    perror("Error in socket");
    return GENERIC_FAIL_CODE;
  }

  /* connect */
  ret = connect(clients[index].fd, (struct sockaddr*)&clients[index].info,
                 sizeof(clients[index].info));
  if (ret < 0) {
    perror("Could not connect to client");
    return GENERIC_FAIL_CODE;
  }

  strncpy(clients[index].name, client_name, strlen(client_name) + 1);
  clients[index].time = time(NULL);
  num_clients++;

  /* send name */
  *((int*)buffer) = strlen(name);
  memcpy(buffer + sizeof(int), name, strlen(name));

  ret = send(clients[index].fd, buffer, strlen(name) + sizeof(int), 0);
  if (ret < 0){
    perror("Could not send name");  /* communication is compromised */
    close(clients[index].fd);
    clients[index].fd = 0;
    num_clients--;
    return GENERIC_FAIL_CODE;
  }

  /* add fd to set */
  FD_SET(clients[index].fd, &fds);
  if (clients[index].fd > fdmax)
    fdmax = clients[index].fd;
    
  (*aux_index) = index;
  return EXIT_SUCCESS;
}

int request_client_list (char* buffer, int max){

  int bytes_to_read, ret;
  int req = CLI_LIST;
    
  /* send list request */
  send(serverfd, &req, sizeof(int), 0);
  
  /* wait for response */
  bytes_to_read = sizeof(int);
  ret = read_from_socket(serverfd, (char*) &req, bytes_to_read);
  if (ret <= 0){
    perror("Error receiving list");
    return GENERIC_FAIL_CODE;
  }
  
  /* list len */
  bytes_to_read = req;
  if (bytes_to_read >= max){
    fprintf(stderr, "Error : list is too long (more than %d chars)\n", max);
    return GENERIC_FAIL_CODE;
  }
  
  /* list */
  ret = read_from_socket(serverfd, buffer, bytes_to_read);
  if (ret <= 0){
    perror("Error receiving list");
    return GENERIC_FAIL_CODE;
  }
  
  buffer[ret] = '\0';
  return EXIT_SUCCESS;
}

void init (char* argv[]){

  /* local data */
  int ret;
  char buffer[BUFLEN];

  /* first open listening socket (chat/files) */
  localfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(localfd < 0, "Error opening socket");
  
  /* fill info */
  local_info.sin_family = AF_INET;
  local_info.sin_port = htons(atoi(argv[2]));
  local_info.sin_addr.s_addr = INADDR_ANY;
  
  /* bind it */
  ret = bind(localfd, (struct sockaddr*) &local_info, sizeof(local_info));
  DIE(ret < 0, "Error in bind");
  
  /* mark it passive */
  ret = listen(localfd, MAX_CLIENTS);
  DIE(ret < 0, "Error in listen");

  /* open server socket */
  serverfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(serverfd < 0, "Error opening socket");
  
  /* server info */
  server_info.sin_family = AF_INET;
  server_info.sin_port = htons(atoi(argv[4]));
  ret = inet_aton(argv[3], &server_info.sin_addr);
  DIE(ret == 0, "Error in inet_aton");

  /* connect */
  lprintf("Connecting to server ... ");
  ret = connect(serverfd,(struct sockaddr*)&server_info,sizeof(server_info));
  DIE(ret < 0, "Could not connect to server");
  lprintf("Done!\n");
  
  /* format buffer - [name_len][name][port_len][port] */
  lprintf("Sending login info ... ");
  int offset = 0;
  *((int*)(buffer + offset)) = strlen (argv[1]);
  offset = offset + sizeof(int);
  memcpy(buffer + offset, argv[1], strlen(argv[1]));
  offset = offset + strlen(argv[1]);
  *((int*)(buffer + offset)) = strlen(argv[2]);
  offset = offset + sizeof(int);
  memcpy(buffer + offset, argv[2], strlen(argv[2]));
  offset = offset + strlen (argv[2]);
  
  /* send basic info */  
  ret = send(serverfd, buffer, offset, 0);
  DIE(ret < 0, "Could not send login info");
  lprintf("Done!\n");

  /* wait for response */
  lprintf("Waiting for response ... ");
  offset = sizeof(int);
  read_from_socket(serverfd, buffer, offset);
  ret = *((int*)buffer);
  lprintf("Got response with code %d : ", ret);

  if (ret == SERV_FULL){
    printf("Server Full!\n");
    close(serverfd);
    exit(EXIT_FAILURE);
  }
  else if (ret == SERV_NAME){
    printf("Name already exists!\n");
    close(serverfd);
    exit(EXIT_FAILURE);
  }
  else if (ret == SERV_FAIL){
    printf("Server Failure!\n");
    close(serverfd);
    exit(EXIT_FAILURE);
  }
  else
    lprintf("Success!\n");
  
  /* prepare fd set */
  FD_ZERO(&fds);
  FD_SET(serverfd, &fds);
  FD_SET(STDIN_FILENO, &fds);
  FD_SET(localfd, &fds);
  fdmax = MAX(serverfd, localfd);
  DIE(fdmax < serverfd, "bad max");
  DIE(fdmax < localfd, "bad max");
  
  /* save name */
  strncpy(name, argv[1], strlen(argv[1]));
  
  /* open history file */
  snprintf(buffer, BUFLEN, "client_%s.log", name);
  history_fd = open(buffer, O_RDWR | O_CREAT | O_TRUNC, 0644);
  DIE(history_fd < 0, "Cannot open history file");
}

int get_index_from_name (char* name){

  int i, j = num_clients;
  for (i = 0; (i < MAX_CLIENTS) && (j > 0); i++)
    if (clients[i].fd > 0){
      if (strcmp(clients[i].name, name) == 0)
        return i;
      j--;
    }

  return -1;
}

int get_index_from_fd (int fd){

  int i, j = num_clients;
  for (i = 0; (i < MAX_CLIENTS) && (j > 0); i++)
    if (clients[i].fd > 0){
      if (clients[i].fd == fd)
        return i;
      j--;
    }

  return -1;
}

int get_free_index (void){

  int i;
  for (i = 0; i < MAX_CLIENTS; i++)
    if (clients[i].fd <= 0) /* should change condition */
      return i;

  return -1;
}

int read_from_socket (int sd, char* buffer, int num_bytes){

  int ret, offset = 0;

  while (num_bytes > 0){
    ret = recv(sd, buffer, num_bytes, 0);
    if (ret <= 0)
      return ret;
    num_bytes = num_bytes - ret;
    offset = offset + ret;
  }
  
  return offset;
}

void free_everything (void){

  /* check with fcntl */

  /* close server socket */
  close(serverfd);
  
  /* close own socket */
  close(localfd);
  
  /* close history file */
  close(history_fd);
  
  /* close clients sockets */
  int i;
  for (i = 0; i < MAX_CLIENTS; i++)
    if (clients[i].fd > 0){
      close(clients[i].fd);
      clients[i].fd = 0;
    }
}
