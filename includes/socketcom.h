

#ifndef _SOCKETCOM_H_
#define _SOCKETCOM_H_

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define _PREESM_WAIT_SERVER_START_US 50
#define _PREESM_TCP_TIMEOUT_MS 10000

#define _PREESM_ERROR_ACK 1
#define _PREESM_ERROR_RESOLVE_HOST 2
#define _PREESM_ERROR_CREATE_SOCKET 3
#define _PREESM_ERROR_BINDING 4

#define PREESM_COM_PORT 25400

typedef struct processingElement_t {
  int id;
  char* host;
  int port;
  struct sockaddr_in listeningIPPort;
} ProcessingElement;

void preesm_send(int targetId, int * socketRegistry, char* buffer, int size);
void preesm_receive(int sourceId, int * socketRegistry, char* buffer, int size);

void preesm_open(int* socketFileDescriptors, int processingElementID, int numberOfProcessingElements, ProcessingElement registry[numberOfProcessingElements]);
void preesm_close(int * socketRegistry, int processingElementID, int numberOfProcessingElements);

#endif
