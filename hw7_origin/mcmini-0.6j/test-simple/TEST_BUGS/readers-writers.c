// NOTE:  Due to subtleties of thread scheduling, this code doesn't
//        always work.  Try running it under GDB, to be sure:
//  gdb THIS_FILE
//  (gb) run

#include <stdio.h>
#include <pthread.h>

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int num_readers = 0;
int num_writers = 0;

void * reader(void *dummy) {
  int i;
  for (i = 0; i < 3; i++) {
    // Acquire reading resource
    pthread_mutex_lock(&mutex);
    while (num_writers > 0) {
      pthread_cond_wait(&cond, &mutex);
    }
    num_readers++;
    pthread_mutex_unlock(&mutex);

    printf("Reading for the %d-th time.\n", i+1);

    // Release reading resource
    pthread_mutex_lock(&mutex);
    // We use pthread_cond_broadcast() instead of pthread_cond_signal().
    // The problem with pthread_cond_signal() is that there is no way
    // to know which thread will be woken by pthread_cond_signal()
    // (nor even to know which thread would be woken by pthread_sem_post()).
    // In principle, we might repeatedly wake up another reader while
    // not waking the writer, if we use pthread_cond_signal().
    // NOTE:  If we wanted to insist on waking the oldest waiting thread first,
    //  the POSIX way is to use pthread_attr_setschedpolicy(), but that
    //  goes far beyond the scope of this exercise.  The current code
    //  has no guarantees of fairness.
    pthread_cond_broadcast(&cond);
    num_readers--;
    pthread_mutex_unlock(&mutex);
  }
}

void * writer(void *dummy) {
  int i;
  for (i = 0; i < 3; i++) {
    // Acquire writing resource
    pthread_mutex_lock(&mutex);
    num_writers++;
    while (num_readers > 0) {
      pthread_cond_wait(&cond, &mutex);
      // Any new readers will see num_writers > 0, & will call pthread_cond_wait
      // Even if they have woken up from a previous pthread_cond_wait,
      //  they will see 'num_writers > 0', and will again call pthread_cond_wait
    }
    pthread_mutex_unlock(&mutex);

    printf("Writing for the %d-th time.\n", i+1);

    // Release writing resource
    pthread_mutex_lock(&mutex);
    // The writer can safely use pthread_cond_signal() instead of
    // pthread_cond_wait() here because there cannot be a second writer
    // waiting.  So, everybody waiting is a reader.
    pthread_cond_signal(&cond);
    num_writers--;
    pthread_mutex_unlock(&mutex);
  }
}

#define NUM_READERS 3
int main() {
  pthread_t writer_thread;
  pthread_t reader_thread[NUM_READERS];
  int i;

  pthread_create(&writer_thread, NULL, &writer, NULL);
  for (i = 0; i < NUM_READERS; i++) {
    pthread_create(&reader_thread[i], NULL, &reader, NULL);
  }

  pthread_join(writer_thread, NULL);
  for (i = 0; i < NUM_READERS; i++) {
    pthread_join(reader_thread[i], NULL);
  }
  return 0;
}
