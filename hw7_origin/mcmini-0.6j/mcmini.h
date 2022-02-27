#include <stdlib.h>

#ifndef MCMINI_H
# define MCMINI_H

/* Alternative scheme for including C and C++:
 * #ifdef __cplusplus
 * # define EXTERNC extern "C"
 * #else // ifdef __cplusplus
 * # define EXTERNC
 * #endif // ifdef __cplusplus
 */

#undef EXTERNC

#ifdef __cplusplus
# define EXTERNC extern "C"
# define VOID
extern "C" {
#else
# define EXTERNC
# define VOID void
#endif // ifdef __cplusplu

// McMini will choose which thread (possibly the same one) to run next.
// Definition of mc_sched_yield().
#if 0
// FIXME:  pthread-simulator.c defines mc_sched().  Is mc_sched_yield() better?
// NOTE:
//   This version works _only_ if the target executable calling
//   mc_sched_yield() was compiled with -fPIC.  (Or the call to
//   mc_sched_yield() may be in a shared library, which of course uses -fPIC.)
EXTERNC int mc_sched(VOID) __attribute ((weak));
#define mc_sched_yield() (mc_sched ? mc_sched() : -1)
#else
typedef int (*mc_sched_yield_ptr_t)();
#define mc_sched_yield() \
  ( getenv("MC_SCHED") ? \
    (*(mc_sched_yield_ptr_t)strtol(getenv("MC_SCHED"), NULL, 16))() : \
     -1 \
  )
#endif

// If you run this without mcmini, any model-checking choices will
//   always choose path 0.  This is probably not what you want.
//   Use a conditional, 'if (is_mcmini()) {...}'  to avoid that.
// FIXME:  This can be made faster by using pattern of mc_sched_yield(), above.
typedef int (*mc_choose_ptr_t)(int);
static unsigned int seed = 0;
#define mc_choose(num_choices) \
   ( (getenv("MC_CHOOSE")) ? \
     (*(mc_choose_ptr_t)strtol(getenv("MC_CHOOSE"), NULL, 16))(num_choices) : \
     ( !seed ? seed = (unsigned int)time(NULL), rand_r(&seed) % num_choices : \
       rand_r(&seed) % num_choices ) \
   )

#define is_mcmini() (getenv("MCMINI") ? 1 : 0)

#ifdef __cplusplus
} // extern "C" {
#endif // ifdef __cplusplus

#undef EXTERNC
#undef VOID

#endif /* end: ifdef MCMIN_H */
