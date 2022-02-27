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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <dlfcn.h>
#include "mc.h"

// FIXME:  pthread-wrappers.h was for pthread-wrappers.c
//         Can we split the parts that belong here?
#include <pthread.h>
#include <semaphore.h>
#include "pthread-wrappers.h"

// You can change this initial trace to a particular trace for debugging.
// Make sure to include '-1' to mark end of trace.
#define DEFAULT_INITIAL_TRACE {0, -1}
char *INITIAL_TRACE_STR = NULL;

// This is added after the final '-1' of a trace to mark when to backtrack.
#define DO_BACKTRACK (-99)
#define DONT_BACKTRACK 0

// Successively fork child for each trace.
// Each element in the trace chooses a thread to schedule,
//   and the exit status of the child represents information
//   about the next critical event, but does not actually enter it.
//   The event is reported by get_exit_status().

int MAX_TRACE_LEN = 30;
#define TRACE_SIZE (MAX_TRACE_LEN*sizeof(int))
int MAX_FRONTIER_LEN = 5000000;
int VERBOSE = 0;
int PARALLEL = 0;

// int mc_trace_frontier[MAX_FRONTIER_LEN][MAX_TRACE_LEN];
int **mc_trace_frontier;

int mc_cur_trace_frontier_idx = 0;
int mc_last_trace_frontier_idx = 1;

int parse_trace(char *trace, int thread_idx_int[]) {
  char *thread_idx_str[MAX_TRACE_LEN];
  int i = 0;
  thread_idx_str[i++] = strtok(trace, ", {}");
  while (thread_idx_str[i-1] != NULL) {
    thread_idx_str[i++] = strtok(NULL, ", {}:");
  }
  thread_idx_str[i-1] = "-1";
  i = 0; // Convert thread_idx_str[] to thread_idx_int[]
  while (1) {
#if 0
    errno = 0;    /* To distinguish success/failure after call */
    thread_idx_int[i] = strtol(thread_idx_str[i], NULL, 10);
#else
    char * choice_str = strstr(thread_idx_str[i], "choose(");
    errno = 0;    /* To distinguish success/failure after call */
    if (choice_str == NULL) {
      thread_idx_int[i] = strtol(thread_idx_str[i], NULL, 10);
    } else { // Add choice from mc_choose()
      int choice = strtol(choice_str + strlen("choose("), NULL, 10);
      thread_idx_int[i] = (choice | CHOOSE_MASK);
    }
#endif
    if (thread_idx_int[i++] == -1) {
      break;
    }
    if (errno != 0) {
      fprintf(stderr, "bad input");
      exit(1);
    }
  }
}

// FIXME:  Should we move the definition of this function earlier, up here?
int last_trace_index(int trace_frontier_idx);

void init_trace_frontier() {
  mc_trace_frontier = (int**)malloc(MAX_FRONTIER_LEN*sizeof(int*));
  int i;
  for (i=0; i < MAX_FRONTIER_LEN; i++) {
    mc_trace_frontier[i] = (int *)malloc(TRACE_SIZE);
  }
  // Initialize first trace: (initialization fails If MAX_TRACE_LEN is var)
  mc_assert(100 >= MAX_TRACE_LEN);
  int mc_trace_tmp[100] = DEFAULT_INITIAL_TRACE;  // By default, this is {0, -1}
  if (INITIAL_TRACE_STR) {
    parse_trace(INITIAL_TRACE_STR, mc_trace_tmp); // convert string to int's
  }
  memcpy(mc_trace_frontier[0], mc_trace_tmp, TRACE_SIZE);
  mc_trace_frontier[0][last_trace_index(0) + 1] = DO_BACKTRACK;
}
int* get_cur_frontier_trace() {
  if (mc_cur_trace_frontier_idx < mc_last_trace_frontier_idx) {
    return mc_trace_frontier[mc_cur_trace_frontier_idx];
  } else {
    return NULL;  // We're at the end of the frontier.
  }
}
int get_cur_frontier_index() {
  return mc_cur_trace_frontier_idx;
}
int* get_next_frontier_trace() {
  if (mc_cur_trace_frontier_idx+1 < mc_last_trace_frontier_idx) {
    return mc_trace_frontier[mc_cur_trace_frontier_idx+1];
  } else {
    return NULL;  // We're at the end of the frontier.
  }
}
int* get_last_frontier_trace() {
  return mc_trace_frontier[mc_last_trace_frontier_idx-1];
}
int last_trace_index(int trace_frontier_idx) {
  int i;
  for (i = 0; i < MAX_TRACE_LEN; i++) {
    if (mc_trace_frontier[trace_frontier_idx][i] == -1) {
      return i;
    }
  }
  fprintf(stderr, "mcmini: internal error: trace_frontier_idx: %d\n"
                  "        Does that trace end in '-1' (end of trace)?\n",
          trace_frontier_idx);
  mc_assert(0);
}
void mark_backtrack(int index) {
  int *trace = mc_trace_frontier[index];
  trace[last_trace_index(index) + 1] = DO_BACKTRACK;
}
void mark_not_backtrack(int index) {
  int *trace = mc_trace_frontier[index];
  trace[last_trace_index(index) + 1] = DONT_BACKTRACK;
}
int trace_idx_is_backtrack(int index) {
  int *trace = mc_trace_frontier[index];
  return trace[last_trace_index(index) + 1] == DO_BACKTRACK;
}
void copy_trace_and_extend(int dest, int src, int thread_idx) {
  if ( dest >= MAX_FRONTIER_LEN ) {
    fprintf(stderr, "mcmini: maximum frontier length reached: %d\n",
            MAX_FRONTIER_LEN);
  }
  mc_assert(dest < MAX_FRONTIER_LEN);
  memcpy(mc_trace_frontier[dest], mc_trace_frontier[src], TRACE_SIZE);
  int i = last_trace_index(dest);
  mc_trace_frontier[dest][i] = thread_idx;
  if ( i+1 >= MAX_TRACE_LEN) {
    fprintf(stderr, "mcmini: maximum trace length reached: %d\n",
            MAX_TRACE_LEN);
  }
  mc_assert(i+1 < MAX_TRACE_LEN);
  mc_trace_frontier[dest][i+1] = -1;
  // mc_trace_frontier[dest][i+2] = DONT_BACKTRACK;
  mark_not_backtrack(dest);
}
int exit_status_is_choose(int status) {
  return status >= CHOOSE_MASK;
}

/* did_expand_trace:  Did the current trace cause extensions of this trace
 *                    to be added to the frontier?  If so, no deadlock.
 *                    (Presumably, the current trace returned EXIT_live.)
 * is_backtrack:  Is the final thread scheduled by the current trace also
 *                the last of the running threads?  Is so, we have finished
 *                scheduling each possible thread at the current level.
 * If the current trace satisfies is_backtrack, and if all of the
 * statuses for traces at this level resulted in EXIT_blocked_at_mutex or
 * EXIT_blocked_at_semaphore, then we have deadlock.  Note that if any
 * traces at this level returned EXIT_ignore or EXIT_no_such_thread
 * then we ignore those traces in determining if deadlock occurred.
 * In fact, the code presumably ignores all other statuses except EXIT_live,
 * or if exit_status_is_choose() is true.  These two cases force
 * 'did_expand_trace' to be true.
 * FIXME:  Is the last sentence true?  Right now, we set 'did_expand_trace'
 *         as the return value of expand_trace_frontier().  Can we simplify
 *         the code to directly set did_expand_trace first, and use that in
 *         expand_trace_frontier().
 */
int is_deadlock(int did_expand_trace, int is_backtrack, exit_status_t status) {
  static int is_maybe_deadlock = 0;
  static int cannot_be_deadlock = 0;
  if (did_expand_trace) {
    // If we expanded a trace, this was EXIT_live or a mc_choose().
    // So, there is no deadlock.
    cannot_be_deadlock = 1;
  }
  if (status == EXIT_blocked_at_mutex ||
      status == EXIT_blocked_at_semaphore) {
    is_maybe_deadlock = 1;
  }

  if (is_backtrack) { // We've tried each thread at this level now.
    if (is_maybe_deadlock && ! cannot_be_deadlock) {
      return 1; // This is definitely deadlock.
    } else {
      is_maybe_deadlock = 0;  // Initialize for trying threads on next schedule
      cannot_be_deadlock = 0; // Initialize for trying threads on next schedule
    }
  }
  return 0; // No deadlock so far.
}

int expand_trace_frontier(int trace_status) {
  if (WIFSIGNALED(trace_status)) { return 0; }

  if (WIFEXITED(trace_status) && WEXITSTATUS(trace_status) == EXIT_ignore &&
      get_cur_frontier_index() == 0 && get_cur_frontier_trace()[0] == 0) {
    // If this is EXIT_ignore and it's not DEFAULT_INITIAL_TRACE, then stop.
    if (last_trace_index(*mc_trace_frontier[0]) > 1) { return 0; }
    // Initial trace, {0, -1}, called pthread_join(), which returned
    // EXIT_ignore.  But we don't want to stop at the {0, -1} trace.
    int next_trace[2] = {1, -1}; // Try again with {1, -1}
    memcpy(mc_trace_frontier[mc_last_trace_frontier_idx++],
           &next_trace, sizeof(next_trace));
    return 1; // We succeeded in expanding trace frontier by one trace.
  }

  int exit_status = WEXITSTATUS(trace_status);
  if (exit_status == EXIT_mcmini_internal_error) {
    fprintf(stderr, "mcmini: internal error: Error in McMini model-checker\n");
    exit(1);
  } else if (exit_status == EXIT_invalid_trace) {
    fprintf(stderr, "mcmini: internal error: Encountered invalid trace\n");
    exit(1);
  } else if (exit_status == EXIT_process_exited) {
    return 0;
  } else if (exit_status == EXIT_ignore) {
    return 0;
  } else if (exit_status == EXIT_no_such_thread) {
    return 0;
  } else if (exit_status == EXIT_blocked_at_mutex) {
    return 0;
  } else if (exit_status == EXIT_blocked_at_semaphore) {
    return 0;
  } else if (exit_status_is_choose(exit_status)) {
    int num_choices = exit_status ^ CHOOSE_MASK;
    int i;
    for (i = 0; i < num_choices; i++) {
      // Set mc_choose value
      copy_trace_and_extend(mc_last_trace_frontier_idx++,
                            mc_cur_trace_frontier_idx, i | CHOOSE_MASK);
    }
    mark_backtrack(mc_last_trace_frontier_idx-1);
    return 1; // We expanded this trace on the frontier.
  } else {
    // FIXME:  This always expands with exactly 4 threads
    //         See pthread_simulator.c for how to use more threads dynamically.
    copy_trace_and_extend(mc_last_trace_frontier_idx++,
                          mc_cur_trace_frontier_idx, 0); // thread_idx 0
    copy_trace_and_extend(mc_last_trace_frontier_idx++,
                          mc_cur_trace_frontier_idx, 1); // thread_idx 1
    copy_trace_and_extend(mc_last_trace_frontier_idx++,
                          mc_cur_trace_frontier_idx, 2); // thread_idx 2
    copy_trace_and_extend(mc_last_trace_frontier_idx++,
                          mc_cur_trace_frontier_idx, 3); // thread_idx 3
    mark_backtrack(mc_last_trace_frontier_idx-1);
    return 1; // We expanded this trace on the frontier.
  }
}

void print_trace (const char *prompt, int frontier_idx) {
  int trace[MAX_TRACE_LEN];
  int i = 0;

  // Destroying frontier data, but used only if terminating in deadlock.
  if (frontier_idx == -1) {
    frontier_idx = get_cur_frontier_index();
    int last_idx = last_trace_index(frontier_idx);
    mc_trace_frontier[get_cur_frontier_index()][last_idx-1] = -1;
  }
  memcpy(trace, mc_trace_frontier[frontier_idx], TRACE_SIZE);
  int last_i = last_trace_index(get_cur_frontier_index());
  fprintf(stderr, "%s", prompt);
  for (i = 0; i < last_i && trace[i] != -1; i++) {
    if (trace[i+1] >= CHOOSE_MASK) {
      fprintf(stderr, "%d:choose(%d), ", trace[i], trace[i+1] ^ CHOOSE_MASK);
      i++;
    } else {
      fprintf(stderr, "%d, ", trace[i]);
    }
  }
  fprintf(stderr, "\n");
  fflush(stderr); // fflush required, due to fprints w/o newline
}

const char* get_exit_status(int status) {
  if (WIFSIGNALED(status)) {
    static char buf[100];
    snprintf(buf, sizeof buf, "SIGNAL(%d)", WTERMSIG(status));
    return buf;
  }
  if (! WIFEXITED(status)) {
    return "??";
  }
  switch (WEXITSTATUS(status)) {
    // FIXME: Add FOREACH macro in mc.h to do this.
    case EXIT_process_exited: return "EXIT_process_exited";
    case EXIT_live: return "EXIT_live";
    case EXIT_blocked_at_mutex: return "EXIT_blocked_at_mutex";
    case EXIT_mutex_unlock_error: return "EXIT_mutex_unlock_error";
    case EXIT_sem_use_before_init: return "EXIT_sem_use_before_init";
    case EXIT_blocked_at_semaphore: return "EXIT_blocked_at_sem";
    case EXIT_no_such_thread: return "EXIT_no_such_thread";
    case EXIT_ignore: return "EXIT_ignore";
    case EXIT_invalid_trace: return "EXIT_invalid_trace";
    case EXIT_mcmini_internal_error: return "EXIT_mcmini_internal_error";
  }
  if (exit_status_is_choose(WEXITSTATUS(status))) {
    static char buf[100];
    snprintf(buf, 100, "mc_choose(%d)", WEXITSTATUS(status) ^ CHOOSE_MASK);
    buf[99] = '\0';
    return buf;
  }
  fprintf(stderr, "mcmini: internal error:  unknown exit status\n");
  mc_assert(0);
}

void set_var_from_env_var(int *var, const char *env_var) {
  if (getenv(env_var)) {
    char *endptr;
    *var = strtol( getenv(env_var), &endptr, 10 );
    mc_assert(*endptr == '\0');
  }
}

#define FORK_TO_CHILD -1
typedef struct fork_wait {
  int is_child;
  int status;
  int childpid;
  int pending_idx_forked; // for debugging
} fork_wait_t;

fork_wait_t fork_and_wait() {
  pid_t childpid = fork();
  fork_wait_t rc;
  rc.childpid = childpid;
  rc.is_child = (childpid == 0);
  if (childpid == 0) { // if child
    initialize_pthread_simulator(get_cur_frontier_trace());
    // Will return from constructor that called this, and then execute main()
  } else { // else parent
    waitpid(childpid, &rc.status, 0);
  }
  return rc;
}

#define NUM_PARALLEL 10

// mc_cur_trace_frontier_idx < mc_last_trace_frontier_idx)
#define min(x,y) ((x)<(y) ? (x) : (y))
fork_wait_t fork_and_wait_par() {
  // trace_pending[] storage is mnior compared to mc_trace_frontier[][]
  static fork_wait_t trace_pending[10000000];
  assert(sizeof(trace_pending) / sizeof(fork_wait_t) >= MAX_FRONTIER_LEN);
  static int pending_next = 0;
  static int pending_latest_idx_forked = 0;
  int max_idx = min(pending_next + NUM_PARALLEL, mc_last_trace_frontier_idx);
  while (pending_latest_idx_forked < max_idx) {
    pid_t childpid = fork();
    fork_wait_t rc;
    rc.childpid = childpid;
    rc.is_child = (childpid == 0);
    rc.pending_idx_forked = pending_latest_idx_forked;
    if (childpid == 0) { // if child
     close(1); // close stdout
     int fd = open("/dev/null", O_WRONLY);
     mc_assert(fd == 1);
     close(2);
     dup(fd); // dup it to fd:2 (stderr)
     initialize_pthread_simulator(mc_trace_frontier[pending_latest_idx_forked]);
     // Will return from constructor that called this, and then execute main()
     return rc;
    } else { // else parent
      mc_assert(pending_latest_idx_forked == rc.pending_idx_forked);
      trace_pending[pending_latest_idx_forked++] = rc;
    }
  }
  // We are the parent, here.
  mc_assert(pending_next == trace_pending[pending_next].pending_idx_forked);
  fork_wait_t rc = trace_pending[pending_next++];
  waitpid(rc.childpid, &rc.status, 0);
  return rc;
}

void __attribute__((constructor))
model_check() {
  set_var_from_env_var(&VERBOSE, "MCMINI_VERBOSE");
  set_var_from_env_var(&PARALLEL, "MCMINI_PARALLEL");
  set_var_from_env_var(&MAX_TRACE_LEN, "MCMINI_MAX_TRACE_LENGTH");
  set_var_from_env_var(&MAX_FRONTIER_LEN, "MCMINI_MAX_FRONTIER_LENGTH");
  // Will be used by parse_trace(); Default NULL, unless: mcmini --intiial-trace
  INITIAL_TRACE_STR = getenv("MCMINI_INITIAL_TRACE");

  initialize_pthread_wrappers();
  init_trace_frontier();
  fprintf(stderr, "McMini: Model checker is starting.\n");
  time_t start_time = time(NULL);
  if (VERBOSE) {
    fprintf(stderr,
            "        (Asterisk at end of TRACE RESULT means to backtrack\n"
            "         from current trace level at the next iteration.)\n");
  }
  fflush(stdout); // Flush stdout now, or else each child will print it.
  int done = 0;
  int iter = 0;
  while ( !done ) {
    iter++;

    if (VERBOSE && !PARALLEL) { // Each interaction can print to stdout/stderr
      print_trace("\nNEW TRACE: ", get_cur_frontier_index());
    }
    // If running in parallel, we redirect stdout, stderr to /dev/null
    fork_wait_t rc = (PARALLEL ? fork_and_wait_par() : fork_and_wait());
    if (rc.is_child) {
      return; // Return from constructor, and then execute main() in child
    }
    int childpid = rc.childpid;
    int status = rc.status;

    if (getenv("MCMINI_GDB")) {
      VERBOSE = 1;
      done = 1; // If debugging child process, stop model_check
    }

    // If PARALLEL, we might have run this in parallel with other traces.
    // So stdout/stderr was redirected to /dev/null for clean output.
    if (VERBOSE && PARALLEL) {
      print_trace("NEW TRACE: ", get_cur_frontier_index());
    }
    int is_backtrack = trace_idx_is_backtrack(get_cur_frontier_index());
    if (VERBOSE) {
      fprintf(stderr,
           "  TRACE RESULT: iter: %d, childpid: %d, exit status: %s%s\n",
           iter, (int)childpid, get_exit_status(status), (is_backtrack?"*":""));
    }

    int did_expand_trace = expand_trace_frontier(status);
    if (is_deadlock(did_expand_trace, is_backtrack, WEXITSTATUS(status))) {
#if 0
      // FIXME: check this logic for the case when there are no mutexes.
      if (WEXITSTATUS(status) == EXIT_ignore) {
        // AND ALSO?  WEXITSTATUS(status) == EXIT_no_such_thread) {}
        fprintf(stderr, "\n%*s***************************\n", 20, "");
        fprintf(stderr, "%*s*** EXITED WITH NO BUG! ***\n", 20, "");
        fprintf(stderr, "%*s***************************\n", 20, "");
        exit(0);
      }
#endif
      // print trace only until deadlock is forced
      fprintf(stderr, "\n%*s*****************\n", 20, "");
      fprintf(stderr,   "%*s*** DEADLOCK! ***\n", 20, "");
      fprintf(stderr,   "%*s*****************\n", 20, "");
      print_trace("    *** TRACE THAT EXHIBITS DEADLOCK: ", -1);
      done = 1;
    } else if (WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT) {
      fprintf(stderr,
              "\n%*s*****************************************************\n"
                "%*s*** TERMINATED WITH SIGABRT! (assertion failure?) ***\n"
                "%*s*****************************************************\n",
              10, "", 10, "", 10, "");
      print_trace("    *** TRACE THAT EXHIBITS ASSERTION FAILURE: ",
                  get_cur_frontier_index());
      done = 1;
    } else if (WIFSIGNALED(status) && WTERMSIG(status) != SIGINT) {
      fprintf(stderr, "\n%*s************************************\n"
                        "%*s*** TERMINATED WITH SIGNAL %2d !! ***\n"
                        "%*s************************************\n",
                      20, "", 20, "", WTERMSIG(status), 20, "");
      print_trace("    *** TRACE THAT EXITS WITH SIGNAL: ",
                  get_cur_frontier_index());
      fprintf(stderr,
              "    *** Some common signals are:\n"
              "          %d(SIGILL), %d(SIGABRT), %d(SIGBUS), "
              "%d(SIGFPE), %d(SIGKILL),\n"
              "          %d(SIGSEGV), %d(SIGPIPE), %d(SIGALRM), %d(SIGTERM)\n",
              SIGILL, SIGABRT, SIGBUS,
              SIGFPE, SIGKILL, SIGSEGV,
              SIGPIPE, SIGALRM, SIGTERM);
      done = 1;
    } else if (mc_cur_trace_frontier_idx + 1 == mc_last_trace_frontier_idx) {
      mc_assert(mc_cur_trace_frontier_idx + 1 == mc_last_trace_frontier_idx);
      mc_assert(is_backtrack);
      fprintf(stderr, "%*s*****************************\n", 20, "");
      fprintf(stderr, "%*s*** SUCCESSFUL EXECUTION! ***\n", 20, "");
      fprintf(stderr, "%*s*****************************\n", 20, "");
      done = 1; // We exhausted the frontier.
    }

#if 0
// FIXME:  What is this condition intended to capture?
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_live &&
        WEXITSTATUS(status) != 0 &&
        !exit_status_is_choose(WEXITSTATUS(status))) {
      // FIXME:  The application could return its own non-zero status.
      //         We should interpose on return from main().
      fprintf(stderr, "\n%*s**************************************\n", 20, "");
      fprintf(stderr,   "%*s*** TERMINATED WITH EXIT STATUS %2d ***\n", 20, "",
               WEXITSTATUS(status));
      fprintf(stderr,  "%*s**************************************\n", 20, "");
      print_trace("    *** TRACE THAT EXHIBITS RACE CONDITION: ",
                  get_cur_frontier_index());
      done = 1;
#endif


    mc_cur_trace_frontier_idx++; // increment to next trace
  }
  fprintf(stderr, "McMini: Model checker is finished.\n");
  time_t total_time = time(NULL) - start_time;
  if (total_time > 10) {
    fprintf(stderr, "        (TIME: %d seconds; %d iterations;"
                    " %d iterations per second)\n",
            total_time, iter, iter/total_time);
  }
  exit(0);
}
