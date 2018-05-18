
#include "socketcom.h"

/**
 * Send 1 byte ack.
 */
void preesm_send_ack(int socket) {
  char ack = 1;
  send(socket, &ack, sizeof(char), 0);
}

/**
 * Receive 1 byte ack and check its value.
 */
void preesm_receive_ack(int socket) {
  char ack = 0;
  recv(socket, &ack, sizeof(char), 0);
  if (!ack) {
    printf("error ack ( = %d)\n",ack);
    exit(_PREESM_ERROR_ACK);
  }
}

/**
 * Send a packet.
 */
void preesm_send(int targetId, int * socketRegistry, char* buffer, int size) {
  int socket = socketRegistry[targetId];
  send(socket, buffer, size, 0);
}

/**
 * Receive a packet.
 */
void preesm_receive(int sourceId, int * socketRegistry, char* buffer, int size) {
  int socket = socketRegistry[sourceId];
  recv(socket, buffer, size, 0);
}

/**
 * Open client connection.
 */
int preesm_connect(ProcessingElement * to) {
  char* host = to->host;
  int port = to->port;
  char portString[6];
  sprintf(portString,"%d",port);
  int sockfd;
  struct addrinfo hints, *servinfo;
  int rv;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(host, portString, &hints, &servinfo)) != 0) {
    printf("[PREESM] - Could not resolve host [%m]\n");
    exit(_PREESM_ERROR_RESOLVE_HOST);
  }
  if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0) {
    printf("[PREESM] - Could not create socket [%m]\n");
    exit(_PREESM_ERROR_CREATE_SOCKET);
  }
  while (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
    // wait for the server to start
    usleep(_PREESM_WAIT_SERVER_START_US);
  }

  preesm_receive_ack(sockfd);
  preesm_send_ack(sockfd);

  freeaddrinfo(servinfo);
  return sockfd;
}

/**
 * Open connection from listening socket.
 */
int preesm_accept(int listeningSocket) {
  struct sockaddr_in clientAddress;
  socklen_t clientAddressLength = sizeof(clientAddress);
  int connfd = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
  preesm_send_ack(connfd);
  preesm_receive_ack(connfd);
  return connfd;
}

/**
 * Open listening socket.
 */
int preesm_listen(ProcessingElement * listener, int numberOfProcessingElements) {
  int listeningProcessingElementID = listener->id;
  int port = listener->port;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
  if(sockfd < 0) {
    printf("[PREESM] - Error : Could not create listen socket [%m]\n");
    exit(_PREESM_ERROR_CREATE_SOCKET);
  }
  listener->listeningIPPort.sin_family = AF_INET;
  listener->listeningIPPort.sin_addr.s_addr = INADDR_ANY;
  listener->listeningIPPort.sin_port = htons(port);
  if (bind(sockfd, (struct sockaddr*)&(listener->listeningIPPort), sizeof(struct sockaddr_in)) < 0) {
    printf("[PREESM] - Error : Could not binding port %d [%m]\n", port);
    exit(_PREESM_ERROR_BINDING);
  }
  //listen to all higher IDs
  listen(sockfd, numberOfProcessingElements - listeningProcessingElementID - 1); 
  return sockfd;
}

/**
 * Open connection to all other processing elements.
 */
void preesm_open(int* socketFileDescriptors, int processingElementID, int numberOfProcessingElements, ProcessingElement registry[numberOfProcessingElements]) {
  // 1- create listen socket, put it in local array of socket at processingElementID index
  socketFileDescriptors[processingElementID] = preesm_listen(&registry[processingElementID], numberOfProcessingElements);
  if (socketFileDescriptors[processingElementID] == -1) {
    exit(-1);
  }
  // 2- connect to all other who have lower ID
  for (int i = processingElementID-1; i >= 0 ; i--) {
    socketFileDescriptors[i] = preesm_connect(&registry[i]);
  }
  // 3- accept connection from all other who have higher ID
  for (int i = processingElementID+1; i < numberOfProcessingElements; i++) {
    int socket = preesm_accept(socketFileDescriptors[processingElementID]);
    socketFileDescriptors[i] = socket;
  }
}

/**
 * Close connection to all other processing elements.
 */
void preesm_close(int * socketRegistry, int processingElementID, int numberOfProcessingElements) {
  // 1- close "client" connection
  for (int i = processingElementID-1; i >= 0 ; i--) {
    preesm_send_ack(socketRegistry[i]);
    close(socketRegistry[i]);
  }
  // 2- close accepted ("server") connection
  for (int i = processingElementID+1; i < numberOfProcessingElements; i++) {
    preesm_receive_ack(socketRegistry[i]);
    close(socketRegistry[i]);
  }
  // 3- close listening socket
  close(socketRegistry[processingElementID]);
}


