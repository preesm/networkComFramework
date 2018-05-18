

#include "socketcom.h"


void preesm_send(int socket, char* buffer, int size) {
  printf("sending %d bytes\n",size);
  int sentBytes = send(socket, buffer, size, MSG_CONFIRM);
  printf("sent %d bytes\n",sentBytes);
}
void preesm_receive(int socket, char* buffer, int size) {
  printf("receiving %d bytes\n",size);
  
  int count;
  ioctl(socket, FIONREAD, &count);
  while (count != size) {
    usleep(50);
  }
  //set of socket descriptors
  
  /*
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(socket, &readfds);
  
  int activity = select(socket + 1 , &readfds , NULL , NULL , NULL);
  printf("activity = %d\n",activity);
  * */
  int readBytes = recv(socket, buffer, size, MSG_WAITALL);
  printf("received %d bytes\n",readBytes);
}

int preesm_connect_v2(char* host, int port, int localPEID) {
  char portString[6];
  sprintf(portString,"%d",port);
  
  int sockfd;  
  struct addrinfo hints, *servinfo;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(host, portString, &hints, &servinfo)) != 0) {
    printf("[SEND %d] - Could not resolve host [%m]\n", localPEID); fflush(stdout);
    exit(1);
  }

  if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0) {
    printf("[SEND %d] - Could not create socket [%m]\n", localPEID); fflush(stdout);
    exit(1);
  }

  while (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
    // wait for the server to start
    usleep(50000);
  }
  freeaddrinfo(servinfo); // all done with this structure
  return sockfd;
}

/*
int preesm_connect_v1(char* host, int port, int localPEID) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct hostent *server = gethostbyname(host);
  if (server == NULL) {
    return -1;
  }
  
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  
  bzero((char *) &serv_addr, sizeof(serv_addr));
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  return connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
}
*/
int preesm_connect(ProcessingElement * from, ProcessingElement * to) {
  int localPEID = from->id;
  char* host = to->host;
  int port = to->port;
  
  return preesm_connect_v2(host, port, localPEID);
}

int preesm_accept(int listeningSocket, ProcessingElement * listener) {
  struct sockaddr_in clientAddress;
  socklen_t clientAddressLength = sizeof(clientAddress);
  int connfd = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
  return connfd;
}

int preesm_listen(ProcessingElement * listener, int numberOfProcessingElements) {
  int listeningProcessingElementID = listener->id;
  int port = listener->port;
  
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if(sockfd < 0) {
    printf("[RECEIVE %d] - Error : Could not create listen socket [%m]\n", listeningProcessingElementID); fflush(stdout);
    exit(1);
  }
  
  listener->listeningIPPort.sin_family = AF_INET;
  listener->listeningIPPort.sin_addr.s_addr = INADDR_ANY;
  listener->listeningIPPort.sin_port = htons(port);
  
  while (bind(sockfd, (struct sockaddr*)&(listener->listeningIPPort), sizeof(struct sockaddr_in)) < 0) {
//#ifdef PREESM_DBG
    printf("[RECEIVE %d] - Error binding port %d [%m] - retrying soon...\n", listeningProcessingElementID, port); fflush(stdout);
//#endif
    //return -1;
    //sleep(1);
    exit(1);
  }
  
  //listen to all except itself
  listen(sockfd, numberOfProcessingElements-1); 
  return sockfd;
}






