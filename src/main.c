/**
 * @file main.c
 */

#include "socketcom.h"
#include <pthread.h>
#include <execinfo.h>
#include <signal.h>

#define NBLOOP 5
#define arraysize (200*1024)

ProcessingElement registry[_PREESM_NBTHREADS_];

void handler(int sig) {
  void *array[10];
  size_t size;
  size = backtrace(array, 10);
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

void* computationThread_Core0(void *arg) {
  int* socketRegistry = (int*)arg;
  int processingElementID = socketRegistry[_PREESM_NBTHREADS_];
  for (int timeLoop = 0; timeLoop < NBLOOP; timeLoop++) {
    preesm_barrier(socketRegistry, processingElementID, _PREESM_NBTHREADS_);
    char somedata[arraysize];
    int value = timeLoop * 1000000 + timeLoop;
    somedata[0] = value % 256;
    somedata[1] = (value >> 8) % 256;
    somedata[2] = (value >> 16) % 256;
    somedata[3] = (value >> 24) % 256;

    preesm_send_start(processingElementID,processingElementID+1,socketRegistry, (char*)&somedata, arraysize, "test");
    preesm_send_start(processingElementID,processingElementID+3,socketRegistry, (char*)&somedata, arraysize, "test");

    char data[4];
    preesm_receive_start(processingElementID+3,processingElementID,socketRegistry, (char*)&data, 4, "test");
    if (data[0] != somedata[0]) {
      exit(-1);
    }
  }
  preesm_barrier(socketRegistry, processingElementID, _PREESM_NBTHREADS_);
  printf("finished\n");
  return NULL;
}
void* computationThread_Core1(void *arg) {
  int* socketRegistry = (int*)arg;
  int processingElementID = socketRegistry[_PREESM_NBTHREADS_];
  for (int timeLoop = 0; timeLoop < NBLOOP; timeLoop++) {
    preesm_barrier(socketRegistry, processingElementID, _PREESM_NBTHREADS_);
    char somedata[arraysize];
    preesm_receive_start(processingElementID-1,processingElementID,socketRegistry, (char*)&somedata, arraysize, "test");
    preesm_send_start(processingElementID,processingElementID+1,socketRegistry, (char*)&somedata, arraysize, "test");
  }
  preesm_barrier(socketRegistry, processingElementID, _PREESM_NBTHREADS_);
  return NULL;
}
void* computationThread_Core2(void *arg) {
  int* socketRegistry = (int*)arg;
  int processingElementID = socketRegistry[_PREESM_NBTHREADS_];
  for (int timeLoop = 0; timeLoop < NBLOOP; timeLoop++) {
    preesm_barrier(socketRegistry, processingElementID, _PREESM_NBTHREADS_);
    char somedata[arraysize];

    preesm_receive_start(processingElementID-1,processingElementID,socketRegistry, (char*)&somedata, arraysize, "test");
    preesm_send_start(processingElementID,processingElementID+1,socketRegistry, (char*)&somedata, arraysize, "test");

  }
  preesm_barrier(socketRegistry, processingElementID, _PREESM_NBTHREADS_);
  return NULL;
}
void* computationThread_Core3(void *arg) {
  int* socketRegistry = (int*)arg;
  int processingElementID = socketRegistry[_PREESM_NBTHREADS_];
  for (int timeLoop = 0; timeLoop < NBLOOP; timeLoop++) {
    preesm_barrier(socketRegistry, processingElementID, _PREESM_NBTHREADS_);

    unsigned char data[4];

    int value = timeLoop * 1000000 + timeLoop;
    data[0] = value % 256;
    data[1] = (value >> 8) % 256;
    data[2] = (value >> 16) % 256;
    data[3] = (value >> 24) % 256;

    preesm_send_start(processingElementID,processingElementID-3,socketRegistry, (char*)&data, 4, "test");

    unsigned char somedata1[arraysize];
    unsigned char somedata2[arraysize];
    preesm_receive_start(processingElementID-3,processingElementID,socketRegistry, (char*)&somedata1, arraysize, "test");
    preesm_receive_start(processingElementID-1,processingElementID,socketRegistry, (char*)&somedata2, arraysize, "test");
    if (somedata2[0] != somedata1[0]) {
      exit(-1);
    }
    if (somedata2[0] != data[0]) {
      exit(-2);
    }
  }
  preesm_barrier(socketRegistry, processingElementID, _PREESM_NBTHREADS_);
  return NULL;
}

void *(*coreThreadComputations[_PREESM_NBTHREADS_])(void *);

void actualThreadComputations(int processingElementID) {
  registry[processingElementID].id = processingElementID;

  int socketFileDescriptors[_PREESM_NBTHREADS_ + 1];
  socketFileDescriptors[_PREESM_NBTHREADS_] = processingElementID;

  preesm_open(socketFileDescriptors, processingElementID, _PREESM_NBTHREADS_, registry);
#ifdef _PREESM_TCP_DEBUG_
  printf("[TCP-DEBUG] %d READY\n", processingElementID);
#endif
  coreThreadComputations[processingElementID](socketFileDescriptors);
#ifdef _PREESM_TCP_DEBUG_
  printf("[TCP-DEBUG] %d DONE\n", processingElementID);
#endif
  preesm_close(socketFileDescriptors, processingElementID, _PREESM_NBTHREADS_);
}

void * threadComputations (void *arg) {
  int processingElementID = *((int*) arg);
  actualThreadComputations(processingElementID);
  return NULL;
}

void init() {
  if (_PREESM_NBTHREADS_ % 4 != 0) {
    printf("Error: number of PE should be multiple of 4\n");
    exit(-1);
  }

  signal(SIGSEGV, handler);
  signal(SIGPIPE, handler);
  signal(SIGKILL, handler);
  signal(SIGINT, handler);

  for (int i = 0; i < _PREESM_NBTHREADS_; i+=4) {
    coreThreadComputations[i+0] = &computationThread_Core0;
    coreThreadComputations[i+1] = &computationThread_Core1;
    coreThreadComputations[i+2] = &computationThread_Core2;
    coreThreadComputations[i+3] = &computationThread_Core3;
  }
}

void initMainPEConfig(ProcessingElement registry[_PREESM_NBTHREADS_]) {
  char * buffer = 0;
  long length;
  FILE * f = fopen ("socketcom.conf", "r");
  if (f) {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = malloc (length);
    if (buffer) {
      fread (buffer, 1, length, f);
    }
    fclose (f);
    char *r = buffer;
    char *tok = r, *end = r;
    strsep(&end, ":");
    char* host = strdup(tok);
    tok = end;
    strsep(&end, ":");
    char* portStr  = tok;
    int port = atoi(portStr);
    free(r);
    strcpy(registry[0].host,host);
    registry[0].port = port;
    registry[0].id = 0;
  } else {
    registry[0].port = 25400;
    registry[0].id = 0;
    strcpy(registry[0].host,"127.0.0.1");
    printf("Warning: Could not locate file 'socketcom.conf'. Using '127.0.0.1:25400'.\n");
  }
}
int main(int argc, char** argv) {
  // 1- init
  init();
  
  // 2- read main Core IP and Port
  initMainPEConfig(registry);
  
  if (argc == 2) {
    // read ID from arguments
    int peId = atoi(argv[1]);
#ifdef _PREESM_TCP_DEBUG_
    printf("[TCP-DEBUG] Runnin %d only\n", peId);
#endif
    actualThreadComputations(peId);
  } else {
    // launch all IDs in separate threads
    pthread_t threads[_PREESM_NBTHREADS_];
    int threadArguments[_PREESM_NBTHREADS_];
    for (int i = 0; i < _PREESM_NBTHREADS_; i++) {
      threadArguments[i] = i;
#ifdef _PREESM_TCP_DEBUG_
      printf("[TCP-DEBUG] Launching %d \n", i);
#endif
      pthread_create(&threads[i], NULL, threadComputations, &(threadArguments[i]));
    }
    for (int i = 0; i < _PREESM_NBTHREADS_; i++) {
      pthread_join(threads[i], NULL);
    }
  }
  //*/
  return 0;
}



