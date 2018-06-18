
#include "socketcom.h"
#include <pthread.h>

#define PREESM_COM_PORT 25400

ProcessingElement registry[_PREESM_NBTHREADS_];

void *(*coreThreadComputations[_PREESM_NBTHREADS_])(void *) = {
	$threadComputationFunctionList
};


void actualThreadComputations(int processingElementID) {
  int socketFileDescriptors[_PREESM_NBTHREADS_];
  preesm_open(socketFileDescriptors, processingElementID, _PREESM_NBTHREADS_, registry);
  coreThreadComputations[processingElementID](NULL);
  preesm_close(socketFileDescriptors, processingElementID, _PREESM_NBTHREADS_);
}

void * threadComputations (void *arg) {
  int processingElementID = *((int*) arg);
  actualThreadComputations(processingElementID);
  return NULL;
}

int main(int argc, char** argv) {
  // TODO overide pes array with values from config file/arguments
  for (int i = 0; i < _PREESM_NBTHREADS_; i++) {
    registry[i].id = i;
    registry[i].host = "127.0.0.1";
    registry[i].port=(PREESM_COM_PORT+i);
  }
  if (argc == 2) {
    // read ID from arguments
    int peId = atoi(argv[1]);
    actualThreadComputations(peId);
  } else {
    // launch all IDs in separate threads
    pthread_t threads[_PREESM_NBTHREADS_];
    int threadArguments[_PREESM_NBTHREADS_];
    for (int i = 0; i < _PREESM_NBTHREADS_; i++) {
      threadArguments[i] = i;
      pthread_create(&threads[i], NULL, threadComputations, &(threadArguments[i]));
    }
    for (int i = 0; i < _PREESM_NBTHREADS_; i++) {
      pthread_join(threads[i], NULL);
    }
  }
  return 0;
}



