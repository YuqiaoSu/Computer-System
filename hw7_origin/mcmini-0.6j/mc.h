void initialize_pthread_simulator(int mc_trace_from_mc[]);

#define CHOOSE_MASK 0b01000000

// If trying to lock a locked mutex, end it, and return status.
// If trying to unlock an unlocked mutex, end and return status.
// At most 16 values allowed (lowest 4 bits in exit status of process)
enum exit_status {
  EXIT_process_exited, // CATCH THIS IN _exit(); Must interpose on exit()
                       //   and maybe pass in the application's exit status.
  EXIT_live, // Process still running; it didn't exit yet.
  EXIT_blocked_at_mutex,
  EXIT_mutex_unlock_error,
  EXIT_sem_use_before_init,
  EXIT_blocked_at_semaphore,
  EXIT_no_such_thread,
  EXIT_ignore,  // E.g., no_such_thread, other traces w/ same prefix
                //   will determine if we have deadlock or not.
  EXIT_invalid_trace,
  EXIT_mcmini_internal_error
};
typedef enum exit_status exit_status_t;

// assert(EXIT_process_exited == 0);  (in _exit SIGSEGV handler)
// AND SAVE HIGHER BIT TO NOTE last thread.

#define mc_assert(CONDITION) \
  do { \
    if ( ! (CONDITION) ) { \
      printf("mcmini:" __FILE__ ":%d: %s: Assertion `" #CONDITION \
             "' failed.\n", \
             __LINE__, __func__); \
      exit(EXIT_mcmini_internal_error); \
    } \
  } while (0)
