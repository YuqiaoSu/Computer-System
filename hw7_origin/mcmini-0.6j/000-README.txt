McMini converts a multi-threaded executable written in C or C++
into an instrumented version of the same program (with multiple threads),
bu McMini adds extra locks to force the threads to obey the indicated
trace.  A trace is a sequence of thread numbers (thread 0, thread 1,
etc.).  When a thread is scheduled, it runs until it reaches a library
call where one might switch contexts (e.g., until pthread_mutex_lock,
or until pthread_mutex_unlock or mc_sched_yield).  If an assert fails,
McMini displays the schedule leading to a failing assert.  And so on ....

QUICK_START:
Following the usual convention, to install McMini, do:
  tar xvf mcmini-*.tgz
  cd mcmini-*
  make

Compiling the target executable:
      If you do not use is_mcmini(), mc_sched_yield(), mc_choose(), etc.,
  then you do not need to re-compile the target executable.
      If you do use any of these features, then you will need to add
  '#include "mcmini.h"' to your target code, and then re-compile with
  '-I MCMINI_INCLUDE_PATH', where MCMINI_INCLUDE_PATH is the path of
  the directory containing 'mcmini.h' in your installation.

Some useful things to try, once it is installed, are:
  ./mcmini -h
  make check-verbose
  ./mcmini --verbose <executable of some program using mutexes>
  ./mcmini --initial-trace '<thread_trace>' \
     --gdb <executable of some program using mutexes>
     [ where thread_trace is a list of threads to schedule
      within quotes:  e.g.: --initial-trace '0,0,1,2,0' ]
  ./mcmini --parallel <executable of some program using mutexes>
     [ Runs 10 trace schedules at once in parallel ]
  ./mcmini --verbose <executable of some program using mutexes> 1>/dev/null
     [ This will run faster, since we don't print stdout to the screen.
       But mcmini will continue to print results to the screen. ]
  ./mcmini --verbose <executable of some program using mutexes> \
        1>tmp.out 2>&1 &
     [ This will run even faster, and you can use 'tail -20 tmp.out' as needed.]


Conceptual overview of what McMini does:
    McMini runs the target executable under a specific schedule or trace,
such as '0, 1, 3, 1, 2' (Run threads 0, 1, 3, 1, and 2 in that order.)
McMini will generate all possible trace schedules up to a certain depth
(up to a certain length of the trace).  McMini stops early and reports
a bug in the target executable if it discovers a trace that leads to
deadlock or assertion failure.  Otherwise, it will continue to generate
and run more trace schedules forever.
    For a given trace schedule, McMini will run a given thread (initially
thread 0 in our example) until that thread reaches a thread
synchronization point in the code (typically pthread_mutex_lock/unlock,
sem_wait/post, mc_sched_yield, or mc_choose).  It will then
stop the current thread, and allow the next thread in the trace
schedule to begin running.  McMini does this "magic" by re-defining
pthread_mutex_lock/unlock, etc., so that the re-defined function will
first call sem_post() on a McMini-internal semaphore associated with
the next thread in the schedule; and it will then call sem_wait() on
the McMini-internal semaphore associated with the current thread in the
schedule.  At any given moment, all threads of the target executable are
blocked on sem_wait(), except for _one_ thread of the target executable
(the current thread that is being run by McMini).

====

TABLE OF CONTENTS FOR THIS FILE:
 * CURRENT LIMITATIONS
 * USING mc_sched_yield()
 * USING mc_choose()
 * USING McMini EFFICIENTLY
 * USING A McMini TRACE TO DEBUG THE TARGET CODE
 * STRUCTURE OF McMini IMPLEMENTATION
 * DETAILED COMMENTS ON THE IMPLEMENTATION

====

CURRENT LIMITATIONS:
 * It will model at most 4 threads.
   [ You can change this by changing the last lines in mc.c:expand_trace_frontier()
     to call copy_trace_and_extend() more than the current 4 invocations. ]
 * All calls to pthread_create() must occur before any call to pthread_join().
 * The retval argument of pthread_join() is not passed from child thread to
   parent thread.
 * If the target executable includes libraries that define constructor
   functions, then those actions are currently performed before McMini
   takes control.  This should not cause problems as long as those
   constructor functions do not create new threads to be modeled by McMini.

====

USING mc_sched_yield():
    This is useful primarily for target code that includes a non-local
variable that is shared among more than one thread.  (Non-local variables
of type pthread_mutex_t and sem_t are already accounted for, and don't
require 'mc_sched_yield'.)
    Normally, McMini will consider switching to a different thread (or
continuing with the same thread) only just before a call to
pthread_mutex_lock/unlock, sem_wait/post, pthread_create, etc.
By inserting mc_sched_yield into the target code, you can force McMini
to choose which thread (possibly the same one) to schedule next at this
point in the code..  This is useful for target executables that use
variables shared among more than one thread.  You should identify such
shared variable

FIXME:  We will replace sched_yield with mc_sched_yield(), since the
        semantics of sched_yield() and pthread_yield() have the differing
        semantics that the current thread may not be scheduled until a
        different thread is scheduled.  But the McMini interpretation is
        that the current thread can be immediately re-scheduled.

====

USING mc_choose():
    In testing your target application, you may wish to test on several
inputs, or during the execution, you may wish to try either of several
branches.  (See mc_choos() in test/mc-aba/mc-aba-5.c for an example.)
You can call 'mc_choose(NUM)', and McMini will test each of NUM
possibility, where 'mc_choose(NUM)' returns rc, and 0 <= rc < NUM.
Normally, mc_choose(NUM) accepts a maximum value of 64 for NUM.  Call it
twice to get
 64*64 choices.
If the target is run without McMini, then when it reaches mc_choose(NUM),
it will return a random rc satisfying 0 <= rc < NUM.  If you prefer
something different when not in McMini, use 'is_mcmini()'.

====

USING McMini EFFICIENTLY:
    McMini already returns immediately from 'sleep(..)', instead of actually
sleeping, since this does not affect detection of a race condition or assertion.
In the same way, you can customize your target executable for better
performance with McMini.  If a section of the target code (like 'sleep()')
will not change the existence of a race condition or assertion failure,
then consider placing that section of a code in an if statement:
  if (!is_mcmini()) {
    ... /* code that need not execute within McMini */
  }

====

USING A McMini TRACE TO DEBUG THE TARGET CODE:
    If you are trying to understand a specific trace, then you can do:
  mcmini --gdb --initial-trace 'INITIAL_TRACE' TARGET_EXECUTABLE
  (gdb) run  # McMini already set a breakpoint at 'mc_yield_to_thread'
  (gdb) display mc_trace@9
  (gdb) display mc_trace_index
  (gdb) printf "%d -> %d\n", mc_trace[mc_trace_index], mc_trace[mc_trace_index+1]

====

STRUCTURE OF McMini IMPLEMENTATION:
    McMini is implemented primarily with three C files:
mcmini.c:  This is for parsing the command lines, setting some environment
           variables, and then exec'ing into the target executable.
           In fact, it execute:  LD_PRELOAD=libmc.so target_executable <args>
libmc.so:  This dynamic library is built from mc.c and pthread-simulator.c.
mc.c:  This file includes a constructor function, model_check(), which
       captures the thread of control before the user's main routine.
       The logic in this file does an iterative-deepening depth-first
       search over all possible schedules of the various threads.
       As befits iterative deepening, it explores all possible schedules
       of a given depth 'D', before exploring schedules of depth 'D+1'.
       If a schedule can be extended (no deadlock, the current thread
       is live), then it extends the schedule to 'N' schedules each
       of depth 'D+1', where 'N' is the number of threads that can be
       scheduled next.
           In mc.c, before a thread schedule can be extended, it must be
       decided whether the current schedule already leads to deadlock,
       assertion failure, etc.  It evaluates each such schedule
       by forking a child process.  In the child process,
       initialize_pthread_simulator() is then called to set the thread
       schedule to be performed by that child.   The thread of control in
       the child process causes the model_check() constructor function
       to return.  After that, the 'main()' routine of the target
       executable is called following the standard C/C++ semantics.
           If you are debugging McMini, try setting an initial breakpoint at
       model_check.  For insights into the code for testing if deadlock
       is found, see the comments before the function is_deadlock()
       in mc.c.  In summary, here are some useful GDB commands:
         gdb --args mcmini --verbose TARGET_EXECUTABLE
         (gdb) break model_check
         (gdb) break is_deadlock
         (gdb) print *mc_trace_frontier[4]@9
         (gdb) print *mc_trace_frontier[mc_cur_trace_frontier_idx]@9
pthread-simulator.c:
           The libmc.so library was preloaded before the target executable
       and it includes pthread-simulator.c.  The pthread-simulator.c file
       defines wrapper functions that interpose on the functions
       pthread_mutex_lock/unlock, sem_wait/post, pthread_create, etc.
       The wrapper functions use a McMini-defined semaphore to guarantee
       that all target threads except one are blocked.  Whether the
       current thread should be blocked or not is determined by
       the thread schedule that was passd in by initialize_pthread_simulator()
       from within mc.c.
           For debugging pthread-simulator.c, see the section:
       USING A McMini trace TO DEBUG THE TARGET CODE:

====

DETAILED COMMENTS ON THE IMPLEMENTATION:

mc.c:
    This is the parent process that will repeatedly run the target
application as a child process.  It passes in a 'frontier_trace'
(a schedule of which thread to execute at each interposition point,
where pthread-simulator.c defines the interposition points).
When the child process returns, the parent looks at the return code,
which indicates deadlock, success, or some other result from 'enum exit_status'
that is defined in mc.h.
    Note that the parent process already includes the target application.
But the parent runs the constructor and never enters main().  It repeatedly
forks and the child calls main().  (No execve is required, since the
target application code was also in the parent process.)  The child
then makes use of pthread-simulator.c from libmc.so, which does the
interpositions of the target application.

pthread-simulator.c is the core of the McMini simulator.
It interposes on calls to pthread_create, pthread_wait, pthread_post, etc.
Its job is to apply the schedule passed in as frontier_trace.
It then schedules whether to switch to a different thread at this
interposition point.

pthread-simulator.h (in libmc.so):
  Defines macros __real_pthread_create, __real_pthread_wait, __real_pthread_post, etc.
  Those macros go to the libpthread definitions directly.
  This is needed because pthread_create, pthread_wait, pthread_post, etc.,
    are interposed by definitions in libmc.so

pthread-simulator.c:mc_sched():
  Switches to next thread in schedule of mc_trace[mc_trace_index].
  This is the core of the algorithm.  Upon any interposition, mc_sched()
  decides what to do next.  It can call mc_yield_to_thread(parent) where
  parent is the parent thread of the current thread.
    If there is no parent thread, then mc_sched() calls
  mc_yield_to_thread(mc_trace[mc_trace_index++]) (i.e., the next thread
						  in the schedule).
    If there is a parent thread, then mc_sched() calls
  mc_yield_to_thread(parent), where
    parent = mc_thread[get_thread_index(pthread_self())].wake_parent
  So, we yield to the parent thread of this thread.

mc_thread_index:
  This is a global variable indicating the next thread to schedule
  in mc_trace[].  That is, mc_trace[mc_thread_index++] gets scheduled next.

pthread_create:
  When the target code calls this, we pass it an intermediate start
  routine, mc_thread_start_routine.  This calls the user start_routine.
  This allows us to capture when the user start_routine exits.
  Currently, when the user start_routine exits, it returns to
  mc_thread_start_routine and then calls pthread_exit.  Our pthread_exit
  then goes into an infinite loop calling 'mc_sched()'.  So, it never
  actually joins with the parent thread.
  FIXME:  We should add logic allowing it to join with the parent thread.

NOTE:
  When parent spawns a child thread, the parent waits until the child
  reaches mc_sched().  The child then posts to the parent.
  The other choice would be to have the parent continue to exit until
  it reaches mc_sched().  But then the child will not yet have initialized
  its data structures while the parent is running.

DATA STRUCTURES (in mc.c):
  // TO DEBUG mc.c IN GDB, DO:  (gdb) break model_check
  // int mc_trace_frontier[MAX_FRONTIER_LEN][MAX_TRACE_LEN];
  int **mc_trace_frontier;
  int mc_cur_trace_frontier_idx = 0;
  int mc_last_trace_frontier_idx = 1;
  // Each trace is an array indicating which thread to schedule next.
  //   The final value in the array is -1, indicating the end of the trace.
  // The mc_trace_frontier array is the array of traces on the frontier
  //   of a breadth-first search.  The mc_cur_trace_frontier_idx is
  //   the next trace for which mc.c will fork and call pthread-simulator.c.
  //   The mc_last_trace_frontier_idx is the index of the first trace
  //   that has not yet been set.
  // For mc_trace_frontier, we currently create it as an array of pointers
  //   to trace arrays.  Each individual trace array is malloc'ed.
  //   The alternative would be to set MAX_FRONTIER_LEN and MAX_TRACE_LEN
  //   to reasonable values in advance for a 2-dimensional array.
  // init_frontier_trace() initializes the mc_trace_frontier, and also
  //   the first trace (mc_trace_frontier[0]).  The first trace is
  //   {0, -1}, but this could be changed to any choice, if we wanted
  //   to begin the exploration at a specific trace (e.g., for debugging).
  // expand_trace_frontier(exit_status) looks at the exit_status of the
  //   most recent trace to complete in pthread-simulator.c.
  //   It then initializes additional traces on the frontier, whose
  //   trace length will be one larger than the current trace length.
  //   last_trace_index() is used to determine the current trace length.
  //   It also increments mc_cur_trace_frontier_idx to be ready to
  //   evaluate the next trace on the frontier.
  //   copy_trace_and_extend() is used to create each new trace onto
  //   the frontier.
  // is_mutex_deadlock() tests if we have reached deadlock.  Deadlock
  //   occurs when each trace returned EXIT_blocked_at_mutex (or something
  //   similar), and so there are no more traces on the mc_trace_frontier
  //   (i.e., mc_cur_trace_frontier_idx == mc_last_trace_frontier_idx).

ALGORITHM:
  At the first interposition point of a child thread, it will yield
  to the parent thread.

mc_choose():
  EXPLAIN
