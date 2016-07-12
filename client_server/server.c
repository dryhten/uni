/**
* A3 PC - Chat and file sharing system
* Mihail Dunaev
* 12 May 2013
*
* server.c - code for server
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "general.h"

/* handles server command */
void handle_command(void);

/* handles client message */
void handle_message(int fd);

/* handles a new connection from a client */
void handle_new_connection(void);

/* save ip & port of client (see get_info) */
int get_info_cnt(char* req_name, char* buffer, int max);

/* saves info on client with req_name in buffer */
int get_info(char* req_name, char* buffer, int max);

/* saves list of clients into buff (at most max) */
int get_clients(char* buffer, int max);

/* prints a list of connected clients */
void print_clients(void);

/* init stuff */
void init(char* argv[]);

/* gets index of client with name */
int get_index_from_name(char* name);

/* gets index of client with fd */
int get_index_from_fd(int fd);

/* returns index of an empty spot in clients */
int get_free_index(void);

/* checks if name already exists in clients */
boolean name_exists(char* name);

/* reads from a socket num_bytes */
int read_from_socket(int sd, char* buffer, int num_bytes);

/* data */
struct sockaddr_in server_info; /* server specific data */
client_s clients[MAX_CLIENTS];   /* list of connected clients */
int listend, fdmax;              /* socket descriptor info */
int num_clients = 0;             /* connected clients */
fd_set fds;                      /* read fds considered in select */
boolean stop = false;            /* if true, server stops */
  
int main (int argc, char* argv[]){

  /* ./server [port] */
  if (argc < 2){
    printf("Usage : %s [port]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* local data */
  int i, ret;
  fd_set aux_fds;

  /* init (sockets etc) */
  init(argv);
  lprintf("\n");

  /* main loop */
  while (stop == false){

    /* save fd set */
    aux_fds = fds;

    /* wait until a new message */
    ret = select(fdmax + 1, &aux_fds, NULL, NULL, NULL);
    DIE(ret == GENERIC_FAIL_CODE, "Error in select");

    /* check which descriptor */
    for (i = 0; i <= fdmax; i++)
      if (FD_ISSET(i, &aux_fds)){
        if (i == listend){  /* new connection */
        	handle_new_connection();
        	lprintf("\n");
        }
        else if (i == STDIN_FILENO){  /* server command */
          handle_command();
          lprintf("\n");
        }
        else{  /* message from clients */
          handle_message(i);
          lprintf("\n");
        }
      }
  }

  /* close everything */
  lprintf("Terminating server ...\n");
  for (i = 0; (i < MAX_CLIENTS) && (num_clients > 0); i++)
    if (clients[i].fd > 0){
      close(clients[i].fd);
      num_clients--;
    }    
  close(listend);

  return 0;
}


void handle_command (void){

  /* get command from stdin */
  char buffer[MAX_CMD_LEN];
  char aux_buffer[MAX_CMD_LEN];
  fgets(buffer, MAX_CMD_LEN, stdin);
  
  /* save original cmd (strtok) */
  strncpy(aux_buffer, buffer, strlen(buffer) + 1);
  char* token = strtok(buffer, " \n");
  
  if (token == NULL)  /* just empty spaces */
    return;
  
  /* parse it */
  if ((strcmp(token, "quit") == 0) ||
      (strcmp(token, "q") == 0)){
    stop = true;
  }
  else if (strcmp(token, "status") == 0){
    print_clients();
  }
  else if (strcmp(token, "kick") == 0){
    lprintf("\n");
    token = strtok(NULL, " \n");
    int index = get_index_from_name(token);
    int code = SERV_KICK;
    
    /* check client */
    if (index < 0){
      lprintf("Error, client %s doesn't exists\n", token);
      return;
    }

    /* send message type */
    send(clients[index].fd, &code, sizeof(int), 0);

    /* reason */
    token = strtok(NULL, " \n");
    if (token != NULL){
      char* reason = strstr(aux_buffer, token);
      code = strlen(reason);
      send(clients[index].fd, &code, sizeof(int), 0);
      send(clients[index].fd, reason, strlen(reason), 0);
    }
    else{  /* no reason */
      code = 0;
      send(clients[index].fd, &code, sizeof(int), 0);
    }
     
    /* remove client */
    close(clients[index].fd);
    FD_CLR(clients[index].fd, &fds);
    num_clients--;
    clients[index].fd = 0;
    lprintf("Kicked %s (total = %d)\n", clients[index].name, num_clients);  
  }
  else if (strcmp(token, "say") == 0){
    int code = SERV_SAY;
    int m_len, i;
    
    /* get message */
    token = strtok(NULL, " \n");
    if (token == NULL){
      printf("\nUsage : say [message]\n");
      return;
    }
    
    char* message = strstr(aux_buffer, token);
    m_len = strlen(message);
    
    /* iterate through all clients */
    for (i = 0; i < MAX_CLIENTS; i++)
      if (clients[i].fd > 0){
        send(clients[i].fd, &code, sizeof(int), 0);
        send(clients[i].fd, &m_len, sizeof(int), 0);
        send(clients[i].fd, message, strlen(message), 0);
      }
  }
}

void handle_message (int fd){

  /* local data */
  int ret, index, bytes_to_read;
  char buffer[BUFLEN];
  
  /* first 4 bytes = type of message */
  bytes_to_read = sizeof(int);
  ret = read_from_socket(fd, buffer, bytes_to_read);
  index = get_index_from_fd(fd);
  
  if (index < 0){ /* should not happen */
    lprintf("Something is fishy about client with fd %d\n", fd);
    return;
  }
  
  if (ret == 0){  /* client closed connection, remove */      
    lprintf("Client %s ip %s port %d fd %d closed the connection ",
              clients[index].name, inet_ntoa(clients[index].info.sin_addr),
              clients[index].port, clients[index].fd);

    close(fd);
    FD_CLR(fd, &fds);
    num_clients--;
    clients[index].fd = 0;
    lprintf("(total = %d)\n", num_clients);
  }
  else if (ret < 0){  /* ignore */
    fprintf(stderr, "Error receiving message from client %s",
             clients[index].name);
    perror(" ");
  }
  else{  /* parse message */
    ret = *((int*)buffer);
    
    if (ret == CLI_LIST){  /* send list of clients */
      lprintf("%s requested the list of clients\n", clients[index].name);
      bytes_to_read = get_clients(buffer + sizeof(int), BUFLEN - sizeof(int));
      *((int*)buffer) = bytes_to_read;
      send(fd, buffer, sizeof(int) + bytes_to_read, 0);
    }
    else if (ret == CLI_INFO){

      lprintf("%s requested info ", clients[index].name);
      char req_name[MAX_NAME_LEN];
    
      /* get client name */
      bytes_to_read = sizeof(int);
      ret = read_from_socket(fd, buffer, bytes_to_read);
      if (ret <= 0){
        perror("Error receiving name");
        return;
      }

      bytes_to_read = *((int*)buffer);
      if (bytes_to_read >= MAX_NAME_LEN){ /* send error; fishy */
        ret = snprintf(buffer + sizeof(int), BUFLEN - sizeof(int),
                        "Name is too big, sure it doesn't exist\n");
        *((int*)buffer) = ret;
        send(fd, buffer, sizeof(int) + ret, 0);
        return;
      }
      
      ret = read_from_socket(fd, req_name, bytes_to_read);
      if (ret <= 0){
        perror("Error receiving name");
        return;
      }
      
      req_name[ret] = '\0';
      lprintf("on %s\n", req_name);
      bytes_to_read = get_info(req_name, buffer + sizeof(int),
                                BUFLEN - sizeof(int));
                                
      if (bytes_to_read == GENERIC_FAIL_CODE){
        ret = snprintf(buffer + sizeof(int), BUFLEN - sizeof(int),
                        "Could not find client name\n");
        *((int*)buffer) = ret;
        send(fd, buffer, sizeof(int) + ret, 0);
        return;
      }
      
      *((int*)buffer) = bytes_to_read;
      send(fd, buffer, sizeof(int) + bytes_to_read, 0);
    }
    else if (ret == CLI_CNT){
    
      lprintf("%s tries to connect to ", clients[index].name);
      char req_name[MAX_NAME_LEN];
    
      /* get client name */
      bytes_to_read = sizeof(int);
      ret = read_from_socket(fd, buffer, bytes_to_read);
      if (ret <= 0){
        perror("Error receiving name");
        return;
      }

      bytes_to_read = *((int*)buffer);
      if (bytes_to_read >= MAX_NAME_LEN){ /* send error; fishy */
        *((int*)buffer) = SERV_CNT_EXIST; /* no client with a name this big */
        send(fd, buffer, sizeof(int), 0);
        return;
      }
      
      ret = read_from_socket(fd, req_name, bytes_to_read);
      if (ret <= 0){
        perror("Error receiving name");
        return;
      }
      
      req_name[ret] = '\0';
      lprintf("%s\n", req_name);
      bytes_to_read = get_info_cnt(req_name, buffer + (2 * sizeof(int)),
                                BUFLEN - (2 * sizeof(int)));

      if (bytes_to_read == GENERIC_FAIL_CODE){
        *((int*)buffer) = SERV_CNT_EXIST;
        send(fd, buffer, sizeof(int), 0);
        return;
      }
      
      *((int*)buffer) = SERV_CNT_OK;
      *((int*)(buffer + sizeof(int))) = bytes_to_read;
      send(fd, buffer, (2 * sizeof(int)) + bytes_to_read, 0);
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
  new_client.fd = accept(listend, (struct sockaddr*) &new_client.info,
                          &new_client.len);
  lprintf("from ip %s ... ", inet_ntoa(new_client.info.sin_addr));

  /* just one client, ignore */
  if (new_client.fd < 0){
    perror("Error in accept\n");
    return;
  }

  /* check server load */
  if (num_clients >= MAX_CLIENTS){
    lprintf("Server is full. Rejecting\n");
    *((int*)buffer) = SERV_FULL;
    send(new_client.fd, buffer, sizeof(int), 0);
    close(new_client.fd);
    return;
  }
  
  /* get login info - name len - should have a timeout */
  bytes_to_read = sizeof(int);
  ret = read_from_socket(new_client.fd, buffer, bytes_to_read);
  if (ret <= 0){
    perror("Error reading from client");
    *((int*)buffer) = SERV_FAIL;
    send(new_client.fd, buffer, sizeof(int), 0);
    close(new_client.fd);
    return;
  }
  
  /* check name len */
  bytes_to_read = *((int*)buffer);
  if (bytes_to_read >= MAX_NAME_LEN){
    lprintf("Name too long. Rejecting\n");
    *((int*)buffer) = SERV_NAME;
    send(new_client.fd, buffer, sizeof(int), 0);
    close(new_client.fd);
    return;
  }
  
  /* name */
  ret = read_from_socket(new_client.fd, new_client.name, bytes_to_read);
  if (ret <= 0){
    perror("Error reading from client");
    *((int*)buffer) = SERV_FAIL;
    send(new_client.fd, buffer, sizeof(int), 0);
    close(new_client.fd);
    return;
  }
  
  /* check name */
  new_client.name[bytes_to_read] = '\0';
  if (name_exists(new_client.name) == true){
    lprintf("Name \"%s\" already exists. Rejecting\n", new_client.name);
    *((int*)buffer) = SERV_NAME;
    send(new_client.fd, buffer, sizeof(int), 0);
    close(new_client.fd);
    return;
  }
  
  /* port len */
  bytes_to_read = sizeof(int);
  read_from_socket(new_client.fd, buffer, bytes_to_read);
  bytes_to_read = *((int*)buffer);
  
  /* check port len */
  if (bytes_to_read >= MAX_DIGITS_PORT){
    lprintf("Port is too big (data corruption). Rejecting\n");
    *((int*)buffer) = SERV_FAIL;
    send(new_client.fd, buffer, sizeof(int), 0);
    close(new_client.fd);
    return;  
  }
  
  /* port */
  read_from_socket(new_client.fd, buffer, bytes_to_read);
  buffer[bytes_to_read] = '\0';
  new_client.port = atoi(buffer);
  lprintf("%s with local port %d\n", new_client.name, new_client.port);

  /* save client info */
  index = get_free_index();
  if (index < 0){ /* should not happen */
    lprintf("Error obtaining index. Rejecting\n");
    *((int*)buffer) = SERV_FAIL;
    send(new_client.fd, buffer, sizeof(int), 0);
    close(new_client.fd);
    return; 
  }

  clients[index] = new_client;
  clients[index].time = time(NULL);
  FD_SET(clients[index].fd, &fds);
  num_clients++;
  if (clients[index].fd > fdmax)
    fdmax = clients[index].fd;

  /* send success */
  *((int*)buffer) = SERV_SUCCESS;
  send(clients[index].fd, buffer, sizeof(int), 0); /* should work */
  lprintf("Successfully added with fd = %d (total = %d)\n", clients[index].fd,
            num_clients);
}

int get_info_cnt (char* req_name, char* buffer, int max){

  int i, j = num_clients;
  int offset = 0, ret;
  
  for (i = 0; (i < MAX_CLIENTS) && (j > 0); i++)
    if (clients[i].fd > 0)
      if (strcmp(clients[i].name, req_name) == 0){
      
        /* port + ip */
        ret = snprintf(buffer, max, "%d ", clients[i].port);
        max = max - ret;
        offset = offset + ret;
        if (max == 0)
          break;
        
        ret = snprintf(buffer + offset, max, "%s",
                        inet_ntoa(clients[i].info.sin_addr));
                        
        offset = offset + ret;
        return offset;
      }

  return GENERIC_FAIL_CODE;
}

int get_info (char* req_name, char* buffer, int max){

  int i, j = num_clients;
  int offset = 0, ret;
  
  for (i = 0; (i < MAX_CLIENTS) && (j > 0); i++)
    if (clients[i].fd > 0)
      if (strcmp(clients[i].name, req_name) == 0){
        ret = snprintf(buffer, max, "%s ", clients[i].name);
        max = max - ret;
        offset = offset + ret;
        if (max == 0)
          break;
        
        ret = snprintf(buffer + offset, max, "%d ", clients[i].port);
        max = max - ret;
        offset = offset + ret;
        if (max == 0)
          break;
          
        time_t total_time = time(NULL) - clients[i].time;
        time_t hours = total_time / 3600;
        time_t min = (total_time % 3600) / 60;
        time_t sec = (total_time % 3600) % 60;
        ret = snprintf(buffer + offset, max,
              "online for %d hours %d mins %d seconds\n",
              (int)hours, (int)min, (int)sec);
                           
        offset = offset + ret;
        return offset;
      }

  return GENERIC_FAIL_CODE;
}

int get_clients (char* buffer, int max){

  int i, j = num_clients;
  int offset = 0, ret;
  
  for (i = 0; (i < MAX_CLIENTS) && (j > 0); i++)
    if (clients[i].fd > 0){
      ret = snprintf(buffer + offset, max, "%s\n", clients[i].name);
      max = max - ret;
      offset = offset + ret;
      if (max == 0)
        break;  /* or error */
    }
  return offset;
}

void print_clients (void){

  int i, j = num_clients;
  
  printf ("\n");
  printf ("Connected clients (total = %d) :\n", num_clients);
  for (i = 0; (i < MAX_CLIENTS) && (j > 0); i++)
    if (clients[i].fd > 0)
      printf("%s %s port %d fd %d since %s", clients[i].name,
               inet_ntoa(clients[i].info.sin_addr), clients[i].port,
               clients[i].fd, ctime(&clients[i].time));
}

void init (char* argv[]){

  /* local data */
  int ret;
  
  /* open socket */
  listend = socket(AF_INET, SOCK_STREAM, 0);
  DIE(listend < 0, "Error opening socket");

  /* server info */
  server_info.sin_family = AF_INET;                /* IPv4 */
  server_info.sin_port = htons(atoi(argv[1]));     /* local port */
  server_info.sin_addr.s_addr = INADDR_ANY;        /* accept from everyone */

  /* bind info with socket */
  ret = bind(listend, (struct sockaddr*) &server_info, sizeof(server_info));
  DIE(ret < 0, "Error in bind");

  /* mark the socket as passive */
  ret = listen(listend, MAX_CLIENTS);
  DIE(ret < 0, "Error in listen");
  
  /* create file descriptor set */
  FD_ZERO(&fds);
  FD_SET(listend, &fds);
  FD_SET(STDIN_FILENO, &fds);
  fdmax = listend;
  lprintf("Everything is set; Listening for clients on port %s...\n", argv[1]);
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

boolean name_exists (char* name){

  int i, j = num_clients;
  for (i = 0; (i < MAX_CLIENTS) && (j > 0); i++)
    if (clients[i].fd > 0){
      if (strcmp(name, clients[i].name) == 0)
        return true;
      j--;
    }

  return false;
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
