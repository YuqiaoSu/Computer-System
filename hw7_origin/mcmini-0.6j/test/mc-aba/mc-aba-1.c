#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include "../../mcmini.h"

struct node {
  char *value;
  struct node *next;
  int is_on_stack; // Used for debugging and verification.
};
typedef struct node node_t;

node_t top_of_stack = {"TOP_OF_STACK", NULL, 1};
node_t *STACK = &top_of_stack;

node_t make_node(char *value) {
  node_t result = {value, NULL, 0};
  return result;
}

void push(node_t *stack, node_t *node) {
  assert( ! node->is_on_stack );
  mc_sched_yield();  // accessing stack, a shared variable
  node_t *first = stack->next;
  node->next = first;
  node->is_on_stack = 1;
  mc_sched_yield();  // accessing stack, a shared variable
  stack->next = node;
}

// This returns NULL when stack is empty.
node_t *pop(node_t *stack) {
  mc_sched_yield();  // accessing stack, a shared variable
  node_t *first = stack->next;
  if (first == NULL) {
    return NULL;
  }
  assert( first->is_on_stack );
  node_t *second = first->next;
  mc_sched_yield();  // accessing stack, a shared variable
  stack->next = second;
  first->is_on_stack = 0;
  return first;
}

void print(node_t *stack) {
  for (; stack != NULL; stack = stack->next) {
    printf("%s->", stack->value);
  }
  printf("NULL\n");
}

int stack_len(node_t *stack) {
  int len = 0;
  for (; stack != NULL && len < 100; stack = stack->next, len++) {
  }
  return len - 1; // Don't count last node, since it points to NULL.
}

#if 0
void *thread_A_start(void *dummy) {
  pop(STACK);
}

void *thread_B_start(void *dummy) {
  node_t *x = pop(STACK);
  pop(STACK);
  push(STACK, x);
}
#else
void *thread_A_start(void *dummy) {
  node_t *x = pop(STACK);
  push(STACK, x);
  return NULL;
}

void *thread_B_start(void *dummy) {
  node_t *x = pop(STACK);
  push(STACK, x);
  return NULL;
}
#endif

int main() {
  // Create STACK: X->Y->Z->NULL
  node_t x = make_node("X"), y = make_node("Y"), z = make_node("Z");
  push(STACK, &z);
  push(STACK, &y);
  push(STACK, &x);
  print(STACK);

  pthread_t thr_A, thr_B;
  pthread_create(&thr_A, NULL, &thread_A_start, NULL);
  pthread_create(&thr_B, NULL, &thread_B_start, NULL);
  // The threads execute, potentially triggering the ABA bug.
  pthread_join(thr_A, NULL);
  pthread_join(thr_B, NULL);
  assert(stack_len(STACK) < 100);
  print(STACK);

  return 0;
}
