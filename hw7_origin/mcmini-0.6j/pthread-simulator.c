/*************************************************************************
 *  Copyright 2020-2021 Gene Cooperman                                        *
 *                                                                       *
 *  This file is part of McMini.                                         *
 *                                                                       *
 *  McMini is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by *
 *  the Free Software Foundation, either version 3 of the License, or    *
 *  (at your option) any later version.                                  *
 *                                                                       *
 *  McMini is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *  GNU General Public License for more details.                         *
 *                                                                       *
 *  You should have received a copy of the GNU General Public License    *
 *  along with McMini.  If not, see <https://www.gnu.org/licenses/>.     *
 *************************************************************************/

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <dlfcn.h>
#include "mc.h"
// This defines __real_XXX()
#include "pthread-wrappers.h"

static volatile int mc_trace_index = 0;
static volatile int mc_thread_index_last = 0;
static volatile int mc_cur_thread_index = 0;
static int *mc_trace;
static int num_threads = 1; // primary thread at beginning

#define MAX_THREAD_INDEX 100
struct mc_thread {
  pthread_t thread_id;
  volatile int is_alive;
  sem_t sem;
  sem_t join_sem; // The parent of this thread will wait on this to join.
  int wake_parent;
  // These next two fields are used by mc_thread_start_routine();
  void *(*start_routine) (void *);
  void *arg;
};
typedef struct mc_thread mc_thread_t;
mc_thread_t mc_thread[MAX_THREAD_INDEX];
mc_thread_t *new_thread_blocked(pthread_t thread_id);

void initialize_pthread_simulator(int mc_trace_from_mc[]) {
  // FIXME:  This is defined in mc.h; now run inside mc.c
  //         We can delete this, when the code is robust.
  //initialize_pthread_wrappers();

  mc_trace = mc_trace_from_mc;
  mc_trace_index = 0;
  mc_thread_index_last = 0;
  mc_cur_thread_index = 0;
  new_thread_blocked(pthread_self()); //initialize primary thread
}

// Use with: (gdb)  dprintf mc_yeidl_to_thread, "%s", gdb_print()
char * mc_debug() {
  int i;
  for (i = 0; i == 0 || i>0 && mc_trace[i-1] != -1; i++) {
    if (i == mc_trace_index) {
      fprintf(stderr, "__%d__, ", mc_trace[i]);
    } else {
      fprintf(stderr, "%d%s", mc_trace[i],
              (mc_trace[i] == -1 ? "\n" : ", "));
    }
  }
  return "";
}

mc_thread_t *new_thread_blocked(pthread_t thread_id) {
  mc_thread[mc_thread_index_last].thread_id = thread_id;
  mc_thread[mc_thread_index_last].is_alive = 1;
  __real_sem_init(&mc_thread[mc_thread_index_last].sem, 0, 0);
  __real_sem_init(&mc_thread[mc_thread_index_last].join_sem, 0, 0);
  mc_thread[mc_thread_index_last].wake_parent = -1;
  mc_thread_t *rc = &mc_thread[mc_thread_index_last];
// FIXME: Testing
mc_assert(mc_thread[mc_thread_index_last].thread_id == thread_id);
  mc_thread_index_last++;
  return rc;
}
int get_thread_index(pthread_t thread_id) {
  // NOTE:  An alternative could be based on returning mc_trace[mc_trace_index].
  int idx;
  for (idx = 0; idx < mc_thread_index_last; idx++) {
    if (pthread_equal(thread_id, mc_thread[idx].thread_id)) {
      return idx;
    }
  }
  // FIXME:  check this.  Should we change new_thread_blocked to return idx?
  // Else it's a new mc_thread.
  new_thread_blocked(thread_id);
  return mc_thread_index_last;
}

// sem_post to next thread in schedule, and then sem_wait on this thread.
#define THREAD_SELF -1
void mc_yield_to_thread(int thread_index) {
  if (thread_index == THREAD_SELF) {
    return; // This was called purely for debugging
  } else if (thread_index >= num_threads) {
    exit(EXIT_ignore); // This schedule listed more threads than exist.
  } else if ( ! mc_thread[thread_index].is_alive ) {
    exit(EXIT_no_such_thread); // This thread no longer exists.
  }
  // FIXME:  Add assert if same as: int my_thread_idx = mc_cur_thread_index;
  int my_thread_idx = get_thread_index(pthread_self());
  mc_trace_index++; // Set to index of next step in mc_trace[]
  // FIXME:  mc_cur_thread_index set but never used.
  mc_cur_thread_index = thread_index; // Set to next thread in sched
  __real_sem_post(&mc_thread[thread_index].sem); // Post to next thread in sched
  __real_sem_wait(&mc_thread[my_thread_idx].sem);
}
// Child thread stopped for first time; wakes up parent thread
void mc_wake_up_parent_thread(int parent_thread_index) {
  // FIXME:  Add assert if same as: int my_thread_idx = mc_cur_thread_index;
  int my_thread_idx = get_thread_index(pthread_self());
  __real_sem_post(&mc_thread[parent_thread_index].sem);
  __real_sem_wait(&mc_thread[my_thread_idx].sem);
}
void mc_sched() {
  if (mc_trace[mc_trace_index] == -1) {
    exit(EXIT_live);
  }
  if (mc_trace[mc_trace_index] > mc_thread_index_last) {
    exit(EXIT_no_such_thread);
  }
  int parent = mc_thread[get_thread_index(pthread_self())].wake_parent;
  if (parent == -1) {
     int trace_index = mc_trace[mc_trace_index];
     if (mc_thread[trace_index].is_alive) {
       mc_yield_to_thread(trace_index);
     } else {
       exit(EXIT_no_such_thread); // This thread is no longer alive.
     }
  } else {
    // We are a child created by pthread_create.  Wake our parent first.
    mc_thread[get_thread_index(pthread_self())].wake_parent = -1;
    mc_assert(parent != -1);
    mc_wake_up_parent_thread(parent);
  }
}


// ===============================================
// We insert our own start_routine into the call to pthread_create().
// This allows us to capture the point where the start_routine exits,
//   and then do our own processing.
typedef void *(*start_routine_ptr_t)(void *);
void *mc_thread_start_routine(void *parent_thread_ptr) {
  // The parent has called sem_wait().  So, we can still use the
  //   mc_thread_ptr of the parent.
  mc_thread_t *mc_thread_ptr = parent_thread_ptr;
  start_routine_ptr_t start_routine = mc_thread_ptr->start_routine;
  void *arg = mc_thread_ptr->arg;
  void *rc = (*start_routine)(arg);
  // FIXME:  The modified pthread_exit will go into an infinite loop.
  //         We should set our mc_thread[thread_id] to indicate we exited
  //         and then 'return rc', to meet up with pthread_join() of parent.
  //         And pthread_exit() should also set our mc_thread[thread_id].
  // FIXME:  Or maybe we should simply return rc, and let libpthread do rest..
#if 0
  pthread_exit(rc);
#else
  void mc_thread_start_routine_exit(void *retval);
  mc_thread_start_routine_exit(rc);
#endif
}
int pthread_create(pthread_t *thread_id, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg) {
  mc_yield_to_thread(THREAD_SELF); // For debugging
  if (attr) {
    fprintf(stderr,
        "mcmini: pthread_create: The 'attr' parameter must be NULL for now.\n");
    exit(EXIT_mcmini_internal_error);
  }
  mc_thread_t *mc_thread_ptr =
    new_thread_blocked(*thread_id); //This will have garbage thread_id copied in
  mc_thread_ptr->wake_parent = get_thread_index(pthread_self());
  mc_thread_ptr->start_routine = start_routine;
  mc_thread_ptr->arg = arg;
  pthread_t *thread_id_ptr = &(mc_thread_ptr->thread_id);
  num_threads++; // Add 1 for the new child thread.
  if (num_threads > 4) {
    fprintf(stderr, "More than the maximum of 4 threads created.\n"
                    "See " __FILE__ ":%d for how to extend the maximum.\n"
                    "But the combinatorial explosion is likely to consume "
                    "too much time.\n",
                    __LINE__);
    exit(EXIT_mcmini_internal_error);
  }
  int rc = __real_pthread_create(thread_id_ptr, attr,
                                 &mc_thread_start_routine, mc_thread_ptr);
  // Now parent waits; when child reaches mc_sched(), it posts to wake_parent";
  // FIXME:  What if child didn't execute mc_sched() and post before exiting?
  __real_sem_wait(&mc_thread[get_thread_index(pthread_self())].sem);
  *thread_id = *thread_id_ptr; // Was set by pthread_create
  return rc;
}

// FIXME:  Could place this inside mc_start_routine()
// void pthread_exit(void *retval) {
void mc_thread_start_routine_exit(void *retval) {
  // This thread has exited.  If we are scheduled to execute, just skip us,
  //   and continue with next thread in schedule.
  // FIXME:  In this variant, pthread_join won't receive retval from child.
  mc_thread[get_thread_index(pthread_self())].is_alive = 0;
  // Post to our join_sem that the parent thread is waiting on:
  __real_sem_post(&mc_thread[get_thread_index(pthread_self())].join_sem);
  mc_sched(); // We're the current thread.  Schedule next thread before exiting.
  // FIXME:  Consider:  __real_pthread_exit(retval);
  // FIXME:  Consider:  __real_pthread_kill(pthread_self(), SIGTERM);
  //         or pthread_detach() and then pthread_kill(pthread_self(), SIGKILL)
  // __builtin_unreachable(); // Satisfy gcc that the fnc does not return.
}

int pthread_join(pthread_t thread_id, void **retval) {
#if 0
  if ( ! mc_thread[get_thread_index(pthread_self())].is_alive ) {
    // This thread is exiting and libpthread.so:thr_start called pthread_join().
    return __real_pthread_join(thread_id, retval);
  }
#endif
  // FIXME:  pthread_exit() isn't really joining.  So, just skip us in sched.
  mc_sched();
  // Because the threads are scheduled one at a time, the mc_sched() above
  // caused this thread to sem_wait().  If this thread is scheduled again,
  // we can finish executing pthread_join().
  if (mc_thread[get_thread_index(thread_id)].is_alive) {
    // Our child thread is still alive.  But it has not yet exited to wake us.
    exit(EXIT_ignore); // Instead, wait for a schedule where child is not alive.
  } else {
    __real_sem_wait(&mc_thread[get_thread_index(thread_id)].join_sem);
    return 0;
    // FIXME:
    // This depends on __real_pthread_exit().  But that function will
    // call pthread_mutex_block, which lands in our own wrapper.
    // Can our wrapper pass through to libpthread.so when the
    // child thread is no longer alive?
    // return __real_pthread_join(thread_id, retval);
  }
}

// ===============================================
// sched_yield()
int sched_yield() {
  mc_sched();
  return 0; // POSIX: 0 means success
}

// ===============================================
// mc_choose(): for argument 3, it returns [0,1,2]
// If you need a larger range than 64, call mc_choose() a second time.
static int mc_choose(int num_choices) {
  mc_assert(num_choices < 64);
  // If here, trace chooses this thread_idx, and next trace sets the choice
  // NOTE:  We might have finished pthread_create; followed by mc_choose() on
  //   both branches.  We need this mc_sched() so that it's unambiguous
  //   which branch will get the next trace_idx for taking the choice.
  //   In principle, we don't need to schedule any thread, but keep code clean.
  mc_sched();
  if (mc_trace[mc_trace_index] == -1) { // first trace seeing this.
    // Model checker will see this special status and expand with choices above
    exit(CHOOSE_MASK | num_choices);
  } else if (mc_trace[mc_trace_index] >= CHOOSE_MASK) {
    // the trace has a scheduled choice; return it
    // Interpret trace value as a choose value (not thread_idx) in this case.
    return mc_trace[mc_trace_index++] ^ CHOOSE_MASK;  /* ^ is exclusive-or */
#if 0
  } else if (mc_trace[mc_trace_index] < num_choices) {
    // the trace has a scheduled choice; return it (CHOOSE_MASK not pass in)
    return mc_trace[mc_trace_index++];
#endif
  } else {
    exit(EXIT_invalid_trace);  // FIXME:  instead, maybe: mc_assert(0);
  }
}

void __attribute__((constructor))
initialize_env() {
  char buf[100] = {0};
  snprintf(buf, sizeof(buf), "%p", (void *)&mc_choose);
  setenv("MC_CHOOSE", buf, 1);
  snprintf(buf, sizeof(buf), "%p", (void *)&mc_sched);
  setenv("MC_SCHED", buf, 1);
  setenv("MCMINI", "EXISTS", 1);
}


// ===============================================
// PERFORMANCE FUNCTIONS:

// In model checking, there is no reason to sleep.
int sleep(unsigned int seconds) {
  return 0;
}


// ===============================================
// MUTEXES:  We don't do a sched_yield at pthread_mutex_init.
//           FIXME:  Even without doing this, we can still detect
//              and report EXIT_sem_use_before_init.  For details
//              see the corresponding discussion for semaphores.
//              (We haven't yet done this.)
//           On encountering an unknown mutex, we test if it is
//           set to PTHREAD_MUTEX_INITIALIZER, and otherwise assert
//           internal error.
//           FIXME:  We don't detect things like the wrong thread
//           doing the mutex_unlock.  This should be easy to add.

#define MAX_MUTEX_INDEX 100
enum mutex_state {MUTEX_LOCKED, MUTEX_UNLOCKED, MUTEX_UNKNOWN};
struct mc_mutex {
  pthread_mutex_t *mutex_addr;
  enum mutex_state state;
};
struct mc_mutex mutex_item[MAX_MUTEX_INDEX];
int mutex_index_last = 0;

// Register this mutex in our array.
// We do not call __pthread_mutex_init().  It may have already been init'ed.
int create_mutex(pthread_mutex_t *mutex) {
  if (mutex_index_last >= MAX_MUTEX_INDEX) { exit(1); }
  struct mc_mutex unlocked = {mutex, MUTEX_UNLOCKED};
  mutex_item[mutex_index_last] = unlocked;
  return mutex_index_last++;
}

#define UNKNOWN_MUTEX_INDEX (-1)
int get_mutex_index(pthread_mutex_t *mutex) {
  int idx;
  for (idx = 0; idx < mutex_index_last; idx++) {
    if (mutex == mutex_item[idx].mutex_addr)
      return idx;
  }
  pthread_mutex_t mutex_tmp = PTHREAD_MUTEX_INITIALIZER;
  if (memcmp(mutex, &mutex_tmp, sizeof *mutex) == 0) {
    // Assume this was declared by initialization of variable
    // We now need to register this mutex in our array.
    return create_mutex(mutex);
  } else {
    mc_assert(0); // internal error
  }
  return UNKNOWN_MUTEX_INDEX;
}

int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr) {
  if (attr) {
    fprintf(stderr,
        "mcmini: pthread_mutex_init:"
        " The 'attr' parameter must be NULL for now.\n");
    exit(EXIT_mcmini_internal_error);
  }
  create_mutex(mutex); // register the mutex; ignoring attr for now
  __real_pthread_mutex_init(mutex, attr); // init mutex using libpthread.so
  return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
  mc_sched();
  int idx = get_mutex_index(mutex);
  mc_assert(idx != UNKNOWN_MUTEX_INDEX);
  if (mutex_item[idx].state == MUTEX_LOCKED) {
    exit(EXIT_blocked_at_mutex); // TRY OTHER SCHED.
  }
  mutex_item[idx].state = MUTEX_LOCKED;
  __real_pthread_mutex_lock(mutex);
  return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
  mc_sched();
  int idx = get_mutex_index(mutex);
  mc_assert(idx != UNKNOWN_MUTEX_INDEX);
  if (mutex_item[idx].state == MUTEX_UNLOCKED) {
    printf("Trying to unlock a mutex that is already unlocked!\n");
    exit(EXIT_mutex_unlock_error);
  }
  mc_assert(mutex_item[idx].state == MUTEX_LOCKED);
  mutex_item[idx].state = MUTEX_UNLOCKED;
  __real_pthread_mutex_unlock(mutex);
  return 0;
}

/*
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *restrict mutex,

int pthread_mutex_destroy(pthread_mutex_t *mutex);
const pthread_mutexattr_t *restrict attr);
*/


// ===============================================
// SEMAPHORES:  We don't do a sched_yield at sem_post, since this alone
//              cannot create deadlock.
//              We don't do a sched_yield at sem_init, but we do verify
//              that the step number at which sem_init was called was
//              strictly less than the step number of sem_post or sem_wait.
//              If this fails, we return an EXIT_sem_use_before_init.

#define MAX_SEMAPHORE_INDEX 100
enum semaphore_state {SEMAPHORE_LOCKED, SEMAPHORE_UNLOCKED, SEMAPHORE_UNKNOWN};
struct mc_semaphore {
  int init_time;
  sem_t *semaphore_addr;
  int count; // negative is # of blocked threads; positive is # of free passes
};
struct mc_semaphore semaphore_item[MAX_SEMAPHORE_INDEX];
int semaphore_index_last = 0;

// Register this semaphore in our array.
// We do not call __pthread_semaphore_init().  It may have already been init'ed.
int create_semaphore(int is_init, sem_t *sem, int value) {
  if (semaphore_index_last >= MAX_SEMAPHORE_INDEX) { exit(1); }
  struct mc_semaphore sem_start = {0, sem, value};
  if (is_init) {
    sem_start.init_time = mc_trace_index;
  }
  semaphore_item[semaphore_index_last] = sem_start;
  return semaphore_index_last++;
}

#define UNKNOWN_SEMAPHORE_INDEX (-1)
int get_semaphore_index(sem_t *sem) {
  int idx;
  for (idx = 0; idx < semaphore_index_last; idx++) {
    if (sem == semaphore_item[idx].semaphore_addr)
      return idx;
  }
  mc_assert(0); // internal error
  return UNKNOWN_SEMAPHORE_INDEX;
}

int sem_init(sem_t *sem, int pshared, unsigned int value) {
  mc_yield_to_thread(THREAD_SELF); // For debugging
  mc_assert(pshared == 0);  // This version doesn't handle process-shared sems
                         // But a variant of mcmini could support mult. proc's
  create_semaphore(1, sem, value); // register the semaphore
  __real_sem_init(sem, 0, value); // init mutex using libpthread.so
  return 0;
}

int sem_post(sem_t *sem) {
  // mc_sched(); /* Don't switch; sem_post can be before or after sem_wait */
  mc_yield_to_thread(THREAD_SELF); // For debugging
  int idx = get_semaphore_index(sem);
  mc_assert(idx != UNKNOWN_SEMAPHORE_INDEX);
  // ************ FIXME:  If post and init are same thread, just verify
  //                      that init_time exists;
  //                      If this is a different thread, our last time
  //                      executed must be greater than init_time.
  //                      where last time executed might be our pthread_create
  //                      time.
//FIXME:  Where is the definition of this function?
  if (last_time_this_thread_executed() > semaphore_item[idx].init_time) {
    exit(EXIT_sem_use_before_init);
  }
  semaphore_item[idx].count++;
  __real_sem_post(sem);
  return 0;
}

int sem_wait(sem_t *sem) {
  mc_sched();
  int idx = get_semaphore_index(sem);
  mc_assert(idx != UNKNOWN_SEMAPHORE_INDEX);
  if (semaphore_item[idx].count-- <= 0) {
    exit(EXIT_blocked_at_semaphore); // TRY OTHER SCHED.
  }
  mc_assert(semaphore_item[idx].count >= 0);
  __real_sem_wait(sem);
  return 0;
}
