#include <mutex>
#include <simgrid/modelchecker.h>
#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(mc_assert_example, "Logging channel used in this example");


#define main(...) \
main(__VA_ARGS__) { \
  void main_simgrid(__VA_ARGS__); \
  main_simgrid(argc, argv); \
} \
\
int primary_thread(__VA_ARGS__)

// primary_thread() is executed by main_simgrid()
int primary_thread(int argc, char* argv[]);

std::vector<simgrid::s4u::Host*> hosts;

inline void main_simgrid(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n", argv[0]);

  e.load_platform(argv[1]);
  hosts = e.get_all_hosts();
  // QUESTION:  Can we just use a single host for all actors?
  //            This would seem to be natural for simulating threads.
  //            A single host for all threads seems to work in this example.
  xbt_assert(hosts.size() >= 1, "This example requires at least 1 hosts");

  simgrid::s4u::Actor::create("primary thread", hosts[0], &primary_thread, argc, argv);

  e.run();
}


#undef assert
#define assert(...) MC_assert(__VA_ARGS__)


// ==============================================================
// This part would traditionally be declared in pthread.h

// This version of pthread_create assumes that arg will not be passed
// to the new thread.
// FIXME:  How to use Actor::create() while passing the single thread argument?
#define pthread_create(thread,attr,start_routine,arg) \
  simgrid::s4u::Actor::create("UNKNOWN", hosts[0], &start_routine);

// Ensure that: pthread_t mymutex = PTHREAD_MUTEX_INITIALIZER
//   translates correctly for SimGrid:
// FIXME: Note that an sthread mutex will be a pointer to allocated memory.
//        It will be re-allocated every time the declaration is initialized.
#undef pthread_mutex_t
#define pthread_mutex_t simgrid::s4u::MutexPtr
#undef PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_INITIALIZER simgrid::s4u::Mutex::create()

// Ensure that: pthread_mutex_lock(&mymutex) translates correctly for SimGrid:
#undef pthread_mutex_lock
#define pthread_mutex_lock(mymutex_ptr) (*mymutex_ptr)->lock()
#undef pthread_mutex_unlock
#define pthread_mutex_unlock(mymutex_ptr) (*mymutex_ptr)->unlock()

#undef sched_yield
#if 0
# define sched_yield() \
  simgrid::s4u::this_actor::yield();
#else
pthread_mutex_t yield_mutex = PTHREAD_MUTEX_INITIALIZER;
# define sched_yield() \
  pthread_mutex_lock(&yield_mutex); \
  pthread_mutex_unlock(&yield_mutex);
#endif
