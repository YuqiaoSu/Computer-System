#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

static bool _initialized = false;

#include <sys/mman.h>
#include <stdlib.h>

inline void* _alloc_raw(size_t n)
{
  void* p = mmap(NULL, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if(p==MAP_FAILED) { perror("_alloc_raw: "); }
  return p;
}

inline void _dealloc_raw(void* ptr, size_t n)
{
  if(ptr==0 || n==0) return;
  int rv = munmap(ptr, n);
  if(rv!=0)
    perror("_dealloc_raw: ");
}

template <size_t _N>
class JFixedAllocStack {
public:
  enum { N=_N };
  JFixedAllocStack() {
    if (_blockSize == 0) {
      _blockSize = 10*1024;
      _root = NULL;
    }
  }

  void initialize(int blockSize) {
    _blockSize = blockSize;
  }

  size_t chunkSize() { return N; }

  //allocate a chunk of size N
  void* allocate() {
    FreeItem* item = NULL;
    do {
      if (_root == NULL) {
        expand();
      }

      // NOTE: _root could still be NULL (if other threads consumed all
      // blocks that were made available from expand().  In such case, we
      // loop once again.

      /* Atomically does the following operation:
       *   item = _root;
       *   _root = item->next;
       */
      item = _root;
    } while (!_root || !__sync_bool_compare_and_swap(&_root, item, item->next));

    item->next = NULL;
    return item;
  }

  //deallocate a chunk of size N
  void deallocate(void* ptr) {
    if (ptr == NULL) return;
    FreeItem* item = static_cast<FreeItem*>(ptr);
    do {
      /* Atomically does the following operation:
       *   item->next = _root;
       *   _root = item;
       */
      item->next = _root;
    } while (!__sync_bool_compare_and_swap(&_root, item->next, item));
  }

protected:
  //allocate more raw memory when stack is empty
  void expand() {
    FreeItem* bufs = static_cast<FreeItem*>(_alloc_raw(_blockSize));
    int count= _blockSize / sizeof(FreeItem);
    for(int i=0; i<count-1; ++i){
      bufs[i].next=bufs+i+1;
    }

    do {
      /* Atomically does the following operation:
       *   bufs[count-1].next = _root;
       *   _root = bufs;
       */
      bufs[count-1].next = _root;
    } while (!__sync_bool_compare_and_swap(&_root, bufs[count-1].next, bufs));
  }

protected:
  struct FreeItem {
    union {
      FreeItem* next;
      char buf[N];
    };
  };
private:
  FreeItem* volatile _root;
  size_t _blockSize;
  char padding[128];
};

// The original code had 4 levels of blocks of different sizes.
JFixedAllocStack<64>   lvl1;

void initialize(void)
{
  lvl1.initialize(1024*16);
  _initialized = true;
}
void* allocate(size_t n)
{
  if (!_initialized) {
    initialize();
  }
  void *retVal;
  if(n <= lvl1.chunkSize()) retVal = lvl1.allocate(); else
  retVal = _alloc_raw(n);
  return retVal;
}
void deallocate(void* ptr, size_t n)
{
  if (!_initialized) {
    char msg[] = "***DMTCP INTERNAL ERROR: Free called before init\n";
    abort();
  }
  if(n <= lvl1.N) lvl1.deallocate(ptr); else
  _dealloc_raw(ptr, n);
}

// This function added for testing
void *allocate_tester(void *dummy) {
  // Each thread has a private item array
  void *item[20];
  int idx = 0;
  for (int i = 0; i < 10; i++) {
     if ((random() & 1) == 1) {
       item[idx++] = lvl1.allocate();
     } else if (idx > 0) {
       lvl1.deallocate(item[--idx]);
     } // Else do nothing
   }
  return NULL;
}

int main() {
  // Initialize 1024 blocks of 16 bytes
  initialize();  
  // Create second thread, with competing calls by allocate_tester()
  srandom(42);
  pthread_t thread2;
  pthread_create(&thread2, NULL, allocate_tester, NULL);
  allocate_tester(NULL);
  return 0;
}
