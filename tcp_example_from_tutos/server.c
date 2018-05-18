/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[]) {
  // check args
  if (argc < 2) {
    fprintf(stderr,"ERROR, no port provided\n");
    exit(1);
  }
  
  // read port from argument
  int portno = atoi(argv[1]);

  // create socket
  int socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFileDescriptor < 0) {
    error("ERROR opening socket");
  }
  
  // init listening address
  struct sockaddr_in listeningIP;
  listeningIP.sin_family = AF_INET;
  listeningIP.sin_addr.s_addr = INADDR_ANY;
  listeningIP.sin_port = htons(portno);
  
  // bind and listen
  int res = bind(socketFileDescriptor, (struct sockaddr *) &listeningIP, sizeof(listeningIP));
  if (res < 0) {
   error("ERROR on binding");
  }
  listen(socketFileDescriptor,8);
  
  //while (1) {
    // init client structure and block on accept.
    struct sockaddr_in clientIP;
    socklen_t clilen = sizeof(clientIP);
    int connectionFileDescriptor = accept(socketFileDescriptor, (struct sockaddr *) &clientIP, &clilen);
    if (connectionFileDescriptor < 0) {
     error("ERROR on accept");
    }
    // connection with client is open. do things...
    char buffer[256];

    bzero(buffer,256);
    int n = read(connectionFileDescriptor,buffer,255);
    if (n < 0) error("ERROR reading from socket");
    printf("Here is the message: %s\n",buffer);
    n = write(connectionFileDescriptor,"I got your message",18);
    if (n < 0) error("ERROR writing to socket");
    
    // ... and finaly close connection
    close(connectionFileDescriptor);
  //}
  close(socketFileDescriptor);
  
  return 0; 
}
