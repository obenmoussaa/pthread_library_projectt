#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "thread.h"
#include "queue.h"
#include <stdint.h>
#include <valgrind/valgrind.h>
#include <errno.h>
#include <ucontext.h>
#define STACK_SIZE (64 * 1024)

struct thread {
  thread_t id;
  int blocked;
  int finished;
  struct thread * waiting_threads;
  void *(*func)(void *);
  void * funcarg;
  ucontext_t uc;
  void * ret;
  int stack_id; 
  TAILQ_ENTRY(thread) queue_threads; 


};

struct thread *main_thread;
struct thread *current_thread = NULL;

TAILQ_HEAD(tailq, thread) run_queue = TAILQ_HEAD_INITIALIZER(run_queue);
TAILQ_HEAD(trash, thread) dead_queue = TAILQ_HEAD_INITIALIZER(dead_queue); // Pour la liberation des ressources

//   __attribute__((constructor)) is used to mark a function as a constructor,
//  which means it will be automatically called before the main() function when 
//  the program starts.
// ici pour initialiser main_thread
__attribute__((constructor)) void init_thread(void) {
    main_thread = malloc(sizeof(*main_thread));

    getcontext(&main_thread->uc); 
    
    current_thread = main_thread;
    main_thread->func = NULL;
    main_thread->blocked = 0;
    main_thread->funcarg = NULL;
    main_thread->waiting_threads = NULL;
    main_thread->ret = NULL;
    main_thread->finished = 0;
    main_thread->uc.uc_link = NULL;
    TAILQ_INSERT_HEAD(&run_queue, current_thread, queue_threads);
}

__attribute__((destructor)) void free_threads(void) {
    if(!main_thread->finished ){
      main_thread->finished = 1;
      TAILQ_INSERT_HEAD(&dead_queue, main_thread, queue_threads);
      // TAILQ_REMOVE(&run_queue, main_thread, queue_threads);
      // free(main_thread); 
    }
    while (!TAILQ_EMPTY(&dead_queue)) {
      struct thread *save_head = TAILQ_FIRST(&dead_queue);
      TAILQ_REMOVE(&dead_queue, save_head, queue_threads);
       if(save_head != main_thread){
           free(save_head->uc.uc_stack.ss_sp);
           VALGRIND_STACK_DEREGISTER(save_head->stack_id);
       }
      free(save_head);
    }
    
  }

int dead_lock(){
  if(TAILQ_EMPTY(&run_queue)){
    return 1;
  }
  return 0;
}


void wrap_func(struct thread *thread) {
  void *retval = thread->func(thread->funcarg);
  if (!thread->finished) {
    thread_exit(retval);
  }
}

int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg) {
    struct thread *thread = malloc(sizeof(*thread));
    thread->id = *newthread;
    thread->waiting_threads = NULL;
    thread->blocked = 0;
    thread->func = func;
    thread->funcarg = funcarg;
    thread->ret = NULL;
    *newthread = (thread_t) thread;
    thread->finished = 0;
    TAILQ_INSERT_TAIL(&run_queue, thread, queue_threads);
    getcontext(&thread->uc);
    thread->uc.uc_stack.ss_size = STACK_SIZE;
    thread->uc.uc_stack.ss_sp = malloc(thread->uc.uc_stack.ss_size);
    thread->stack_id = VALGRIND_STACK_REGISTER(thread->uc.uc_stack.ss_sp, // debut de la pile
                                               thread->uc.uc_stack.ss_sp + thread->uc.uc_stack.ss_size); // fin 
    thread->uc.uc_link = NULL;
    makecontext(&thread->uc, (void (*)(void))wrap_func, 1, (intptr_t) thread);
    
    return 0;
}

thread_t thread_self(void) {
    return (thread_t) current_thread;
}





// ################## la partie des signaux ##################



