#include <stdio.h>
#include <pthread.h>

struct node {
  char *value;
  struct node *next;
};
typedef struct node node_t;

node_t top_of_stack = {"TOP_OF_STACK", NULL};
node_t *STACK = &top_of_stack;

node_t make_node(char *value) {
  node_t result = {value, NULL};
  return result;
}

void push(node_t *stack, node_t *node) {
  node_t *first = stack->next;
  node->next = first;
  stack->next = node;
}

// This returns NULL when stack is empty.
node_t *pop(node_t *stack) {
  node_t *first = stack->next;
  if (first == NULL) {
    return NULL;
  }
  node_t *second = first->next;
  stack->next = second;
  return first;
}

void print(node_t *stack) {
  for (; stack != NULL; stack = stack->next) {
    printf("%s->", stack->value);
  }
  printf("NULL\n");
}

void *thread_A_start(void *dummy) {
  pop(STACK);
  return NULL;
}

void *thread_B_start(void *dummy) {
  node_t *x = pop(STACK);
  pop(STACK);
  push(STACK, x);
  return NULL;
}

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
  print(STACK);

  return 0;
}
