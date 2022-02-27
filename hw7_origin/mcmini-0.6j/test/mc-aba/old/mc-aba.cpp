// Taken from:  https://en.wikipedia.org/wiki/ABA_problem

#include <atomic>
#include <string>
#include <iostream>
#include <stdio.h>

class Obj
{
public:
  std::string value;
  Obj* next;
  Obj(std::string value1) {
    value = value1;
    next = nullptr;
  }
};

/* Naive lock-free stack which suffers from ABA problem.*/
class Stack {
  std::atomic<Obj*> top_ptr;

public:
  //
  // Pops the top object and returns a pointer to it.
  //
  Obj* Pop() {
    while (1) {
      Obj* ret_ptr = top_ptr;
      if (!ret_ptr) return nullptr;
      // For simplicity, suppose that we can ensure that this dereference is safe
      // (i.e., that no other thread has popped the stack in the meantime).
      Obj* next_ptr = ret_ptr->next;
      // If the top node is still ret, then assume no one has changed the stack.
      // (That statement is not always true because of the ABA problem)
      // Atomically replace top with next.
      if (top_ptr.compare_exchange_weak(ret_ptr, next_ptr)) {
        return ret_ptr;
      }
      // The stack has changed, start over.
    }
  }
  //
  // Pushes the object specified by obj_ptr to stack.
  //
  void Push(Obj* obj_ptr) {
    while (1) {
      Obj* next_ptr = top_ptr;
      obj_ptr->next = next_ptr;
      // If the top node is still next, then assume no one has changed the stack.
      // (That statement is not always true because of the ABA problem)
      // Atomically replace top with obj.
      if (top_ptr.compare_exchange_weak(next_ptr, obj_ptr)) {
        return;
      }
      // The stack has changed, start over.
    }
  }

  void Print() {
    Obj* obj_ptr = top_ptr;
    while (1) {
      if (obj_ptr == nullptr) {
        break;
      }
      std::cout << obj_ptr->value << " ";
      obj_ptr = obj_ptr->next;
    }
    std::cout << std::endl;
  }

  Stack() {
    top_ptr = nullptr;
  }
};

int main() {
  Stack stack;
  Obj node_a = Obj("A");
  Obj node_b = Obj("B");
  Obj node_c = Obj("C");
  Obj node_d = Obj("D");
  stack.Push(&node_d);
  stack.Push(&node_c);
  stack.Push(&node_b);
  stack.Push(&node_a);
  stack.Print();

  stack.Pop();
  stack.Print();
  return 0;
}
