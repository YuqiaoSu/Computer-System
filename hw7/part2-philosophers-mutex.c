// Naive dining philosophers solution, which leads to deadlock.

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
// #include "mc-utils.h"

#define NUM_THREADS 3

struct forks {
  int philosopher;
  pthread_mutex_t *left_fork;
  pthread_mutex_t *right_fork;
} forks[NUM_THREADS];

pthread_mutex_t startEat;

void * philosopher_doit(void *forks_arg) {

  pthread_mutex_lock(&startEat); 
  struct forks *forks = forks_arg;
  pthread_mutex_lock(forks->left_fork);
  pthread_mutex_lock(forks->right_fork);
  sleep(1);
  printf("Philosopher %d just ate.\n", forks->philosopher);
  pthread_mutex_unlock(forks->left_fork);
  pthread_mutex_unlock(forks->right_fork);
  pthread_mutex_unlock(&startEat); 
  return NULL;
}

int main(int argc, char* argv[])
{
  pthread_t thread[NUM_THREADS];
  pthread_mutex_t mutex_resource[NUM_THREADS];
  pthread_mutex_init(&startEat,NULL);
  int i;
  for (i = 0; i < NUM_THREADS; i++) {
    // ANSI C/C++ require the cast to pthread_mutex_t, 'struct forks',
    //  respectively, because these are runtime statements, and not declarations
    mutex_resource[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    forks[i] = (struct forks){i,
                     &mutex_resource[i], &mutex_resource[(i+1) % NUM_THREADS]};
  }

  for (i = 0; i < NUM_THREADS; i++) {
    pthread_create(&thread[i], NULL, &philosopher_doit, &forks[i]);
  }

  for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(thread[i], NULL);
  }
  
  pthread_mutex_destroy(&startEat);
  return 0;
}