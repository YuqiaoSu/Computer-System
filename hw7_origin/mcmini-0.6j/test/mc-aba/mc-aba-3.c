// To compile, gcc -I DIR_OF_mcmini THIS_FILE

#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include "mcmini.h"

struct node {
  char *value;
  struct node *next;
  int is_on_stack; // Used for debugging and verification.
};
typedef struct node node_t;

node_t top_of_stack = {"TOP_OF_STACK", NULL, 1};
node_t *STACK = &top_of_stack;

int stack_len(node_t *stack) {
  int len = 0;
  for (; stack != NULL && len < 100; stack = stack->next, len++) {
  }
  return len - 1; // Don't count last node, since it points to NULL.
}

void print(node_t *stack) {
  for (; stack != NULL; stack = stack->next) {
    printf("%s->", stack->value);
  }
  printf("NULL\n");
}

node_t make_node(char *value) {
  node_t result = {value, NULL, 0};
  return result;
}

// FROM: https://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Atomic-Builtins.html
// bool __sync_bool_compare_and_swap (type *ptr, type oldval, type newval)
//   These builtins perform an atomic compare and swap. That is, if the
//   current value of *ptr is oldval, then write newval into *ptr.

void push(node_t *stack, node_t *node) {
  while (1) {
    assert( ! node->is_on_stack );
    mc_sched_yield(); // stack->next is a shared variable
    node_t *first = stack->next;
    node->next = first;
    node->is_on_stack = 1;
    mc_sched_yield(); // stack->next is a shared variable
    assert(stack_len(STACK) < 100);
    printf("  push(%s)[TRYING]: ", node->value);
    print(STACK);
    // ATOMIC: if (stack->next == first) { stack->next = node; }
    if ( __sync_bool_compare_and_swap (&(stack->next), first, node) ) {
      break;
    } else {
      node->is_on_stack = 0; // else try again in while loop
    }
  }
  printf("  push(%s)[DONE]: ", node->value); print(stack);
}

// This returns NULL when stack is empty.
node_t *pop(node_t *stack) {
  while (1) {
    mc_sched_yield(); // stack->next is a shared variable
    node_t *first = stack->next;
    if (first == NULL) {
      return NULL;
    }
    assert( first->is_on_stack );
    node_t *second = first->next;
    mc_sched_yield(); // stack->next is a shared variable
    // ATOMIC: if (stack->next == first) { stack->next = second; }
    if ( __sync_bool_compare_and_swap (&(stack->next), first, second) ) {
      first->is_on_stack = 0;
      assert(stack_len(STACK) < 100);
      printf("  pop()=>%s: ", first->value);
      print(STACK);
      return first;
    } // else try again in while loop
  }
}

#if 0
void *thread_A_start(void *name) {
  printf("  %s: ", (char *)name);
  pop(STACK);
}

void *thread_B_start(void *name) {
  printf("  %s: ", (char *)name);
  node_t *x = pop(STACK);
  printf("  %s: ", (char *)name);
  pop(STACK);
  printf("  %s: ", (char *)name);
  push(STACK, x);
}
#else
void *thread_A_start(void *name) {
  printf("  %s: ", (char *)name);
  node_t *x = pop(STACK);
  printf("  %s: ", (char *)name);
  push(STACK, x);
  return NULL;
}

void *thread_B_start(void *name) {
  printf("  %s: ", (char *)name);
  node_t *x = pop(STACK);
  printf("  %s: ", (char *)name);
  push(STACK, x);
  return NULL;
}
#endif

int main() {
  printf("*** STARTING NEW JOB ***\n");
  // Create STACK: X->Y->Z->NULL
  node_t x = make_node("X"), y = make_node("Y"), z = make_node("Z");
  push(STACK, &z);
  push(STACK, &y);
  push(STACK, &x);
  print(STACK); fflush(stdout);

  pthread_t thr_A, thr_B;
  pthread_create(&thr_A, NULL, &thread_A_start, (void *)"A");
  pthread_create(&thr_B, NULL, &thread_B_start, (void *)"B");
  // The threads execute, potentially triggering the ABA bug.
  pthread_join(thr_A, NULL);
  pthread_join(thr_B, NULL);
  assert(stack_len(STACK) < 100);
  fflush(stdout); print(STACK);

  return 0;
}
