
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "socketcom.h"

#define NB_PE 4

ProcessingElement registry[NB_PE];

void * threadComputations (void *arg) {
  
  int processingElementID = *((int*) arg);
  int socketFileDescriptors[NB_PE];

  // 1- create listen socket, put it in local array of socket at processingElementID index
  // TODO : fix listening max count accepts
  socketFileDescriptors[processingElementID] = preesm_listen(&registry[processingElementID], NB_PE);
  if (socketFileDescriptors[processingElementID] == -1) {
    exit(-1);
  }

  // 2- connect to all other who have lower ID
  // except for first processing element
  for (int i = 0; i < processingElementID; i++) {
    socketFileDescriptors[i] = preesm_connect(&registry[processingElementID], &registry[i]);
  }

  // 3- accept connection from all other who have higher ID
  // except for last processing element
  for (int i = processingElementID+1; i < NB_PE; i++) {
    socketFileDescriptors[i] = preesm_accept(socketFileDescriptors[processingElementID], &registry[processingElementID]);
  }

  // 4- TODO computation + test comm
  switch (processingElementID) {
    case 0:
      {
        char* data = "some message";
        preesm_send(socketFileDescriptors[1],data, 12);
        break;
      }
    case 1:
      {
        char buffer[13];
        preesm_receive(socketFileDescriptors[0],buffer, 12);
        printf("[%s]\n",buffer);
        break;
      }
    case 2:
      {
        printf("I'm 2\n");
        break;
      }
    case 3:
      {
        printf("I'm 3\n");
        break;
      }
  }
  
  sleep(1);
  
  printf("%d closing\n", processingElementID);
  for (int i = 0; i < NB_PE; i++) { 
    close(socketFileDescriptors[i]);
  }
  return NULL;
}

int main(int argc, char** argv) {
  
  // TODO overide pes array with values from config file/arguments  
  pthread_t threads[NB_PE];
  for (int i = 0; i < NB_PE; i++) {
    registry[i].id = i;
    registry[i].host = "127.0.0.1";
    registry[i].port=(PREESM_COM_PORT+i);
  }

  int threadArguments[NB_PE];
  for (int i = 0; i < NB_PE; i++) {
    threadArguments[i] = i;
    pthread_create(&threads[i], NULL, threadComputations, &(threadArguments[i]));
  }

  for (int i = 0; i < NB_PE; i++) {
    pthread_join(threads[i], NULL);
  }
  return 0;
}



