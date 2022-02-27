#include <assert.h>
#include <pthread.h>
#include "mcmini.h"

#define N 3  /* Number of elements in circular buffer */
#define UNUSED (-1)
#define DEBUG
#define ITERATIONS 3 /* Don't set this too large or mcmini can't do it */
#define NUM_THREADS 3 /* Don't set this too large or mcmini can't do it */

int buffer[N];
int next_idx = 0;
int last_idx = 0;
pthread_mutex_t buffer_mutex;

int put_in_buffer(int elt) {
  pthread_mutex_lock(&buffer_mutex);
  if(last_idx == next_idx) {
    pthread_mutex_unlock(&buffer_mutex);
    return -1; // We overwrote an elt.
  }
#ifdef DEBUG
  assert(buffer[last_idx] == UNUSED);
#endif
  buffer[last_idx] = elt;
  last_idx = (last_idx + 1) % N;
  pthread_mutex_unlock(&buffer_mutex);
}

int get_from_buffer() {
  pthread_mutex_lock(&buffer_mutex);
  if(last_idx == next_idx) {
    pthread_mutex_unlock(&buffer_mutex);
    return -1; // Buffer was empty.
  }
  int elt = buffer[next_idx]; // Copy this now, while we are protected
                              // by the guard mutex.
#ifdef DEBUG
  assert(buffer[next_idx] != UNUSED);
  buffer[next_idx] = UNUSED; // We've already copied out the elt.
#endif
  next_idx = (next_idx + 1) % N;
  pthread_mutex_unlock(&buffer_mutex);
  return elt; // Return the copy here; Never return while holding lock.
}

void * do_work(void *dummy) {
  int i;
  for (i = 0; i < ITERATIONS; i++) {
    int choose = mc_choose(2);
    if (choose == 0) {
      int rc = put_in_buffer(1);
      if (rc == -1) {
        i--;
      }
    } else { // else choose == 1;
      int rc = get_from_buffer();
      if (rc == -1) {
        i--;
      }
    }
  }
}

int main() {
  int i;
#ifdef DEBUG
  for (i = 0; i < N; i++) {
    buffer[i] = UNUSED;
  }
#endif

  pthread_t thread[NUM_THREADS];
  for (i = 0; i < NUM_THREADS; i++) {
    pthread_create(&thread[i], NULL, &do_work, NULL);
  }
  for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(thread[i], NULL);
  }
  return 0;
}
