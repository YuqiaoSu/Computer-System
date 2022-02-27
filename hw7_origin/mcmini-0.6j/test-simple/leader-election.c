#include <stdio.h>
#include <assert.h>
#include <pthread.h>

int var_A = 0, var_B = 0;

void *thread_A_start(void *dummy) {
  sched_yield();
  if (var_B == 0) {
    sched_yield();
    var_A = 1;
  }
  return NULL;
}

void *thread_B_start(void *dummy) {
  sched_yield();
  if (var_A == 0) {
    sched_yield();
    var_B = 1;
  }
  return NULL;
}

int main() {
  pthread_t thr_A, thr_B;
  pthread_create(&thr_A, NULL, &thread_A_start, NULL);
  pthread_create(&thr_B, NULL, &thread_B_start, NULL);
  pthread_join(thr_A, NULL);
  pthread_join(thr_B, NULL);
  assert(! (var_A && var_B));

  return 0;
}
