

#ifndef _SOCKETCOM_H_
#define _SOCKETCOM_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>


typedef struct processingElement_t {
  int id;
  char* host;
  int port;
  struct sockaddr_in listeningIPPort;
} ProcessingElement;


int preesm_listen(ProcessingElement * listener, int numberOfProcessingElements);
int preesm_connect(ProcessingElement * from, ProcessingElement * to);
int preesm_accept(int listeningSocket, ProcessingElement * listener);

void preesm_send(int socket, char* buffer, int size);
void preesm_receive(int socket, char* buffer, int size);

#endif
