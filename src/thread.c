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
#define STACK_SIZE (10 * 4096)

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
  int is_signal_pending; 
  int signal_id;  
};

struct thread *main_thread;
struct thread *current_thread = NULL;

TAILQ_HEAD(tailq, thread) run_queue = TAILQ_HEAD_INITIALIZER(run_queue);
TAILQ_HEAD(trash, thread) trash_queue = TAILQ_HEAD_INITIALIZER(trash_queue);

/**
 * The __attribute__((constructor)) is used to mark a function as a constructor,
 * which means it will be automatically called before the main() function when 
 * the program starts.
 */
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
      TAILQ_INSERT_HEAD(&trash_queue, main_thread, queue_threads);
    }
    while (!TAILQ_EMPTY(&trash_queue)) {
      struct thread *save_head = TAILQ_FIRST(&trash_queue);
      TAILQ_REMOVE(&trash_queue, save_head, queue_threads);
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
    thread->waiting_threads = NULL;
    thread->funcarg = funcarg;
    thread->ret = NULL;
    *newthread = (thread_t) thread;
    thread->finished = 0;
    thread->is_signal_pending = 0;
    thread->signal_id = 0;

    TAILQ_INSERT_TAIL(&run_queue, thread, queue_threads);
    getcontext(&thread->uc);
    
    thread->uc.uc_stack.ss_size = 64 * 1024;
    thread->uc.uc_stack.ss_sp = malloc(thread->uc.uc_stack.ss_size);
    thread->stack_id = VALGRIND_STACK_REGISTER(thread->uc.uc_stack.ss_sp,
                                               thread->uc.uc_stack.ss_sp + thread->uc.uc_stack.ss_size);
    thread->uc.uc_link = NULL;
    makecontext(&thread->uc, (void (*)(void))wrap_func, 1, (intptr_t) thread);
    
    return 0;
}

thread_t thread_self(void) {
    return (thread_t) current_thread;
}

int thread_yield(void) {
  struct thread *save_head = TAILQ_FIRST(&run_queue);
  TAILQ_REMOVE(&run_queue, current_thread, queue_threads);
  if( save_head->blocked != 1 ){
      TAILQ_INSERT_TAIL(&run_queue, current_thread, queue_threads);
  }
  struct thread *new_current_thread = TAILQ_FIRST(&run_queue);
  current_thread = new_current_thread;
  
  swapcontext(&save_head->uc, &new_current_thread->uc);
  
  // free save_head ! pas maintenant !
  return 0;
}

int thread_join(thread_t thread, void **retval) {
  struct thread *target_thread = (struct thread *) thread;
  if (!target_thread->finished) {
    target_thread->waiting_threads = current_thread;
    struct thread *save_head = TAILQ_FIRST(&run_queue);
    TAILQ_REMOVE(&run_queue, current_thread, queue_threads);
    // if(dead_lock() && current_thread != main_thread){
    //   TAILQ_INSERT_TAIL(&run_queue, current_thread, queue_threads);
    //   return EDEADLK;
    //   }
  
    struct thread *new_current_thread = TAILQ_FIRST(&run_queue);
    current_thread = new_current_thread;
  
    swapcontext(&save_head->uc, &new_current_thread->uc);
  }
  
  if (retval != NULL) {
    *retval = target_thread->ret;
  }

  return 0;
}


void thread_exit(void *retval) {
    struct thread *save_head = TAILQ_FIRST(&run_queue);
    current_thread->finished = 1;
    current_thread->ret = retval;
    TAILQ_REMOVE(&run_queue, current_thread, queue_threads);
    if(current_thread == main_thread){
      TAILQ_INSERT_HEAD(&trash_queue, current_thread, queue_threads);
    }
    else{
      TAILQ_INSERT_TAIL(&trash_queue, current_thread, queue_threads);
    }

    if(save_head->waiting_threads != NULL){ 
      TAILQ_INSERT_HEAD(&run_queue, save_head->waiting_threads, queue_threads);
    }

    if(!TAILQ_EMPTY(&run_queue)){
      struct thread *new_current_thread = TAILQ_FIRST(&run_queue);
      current_thread = new_current_thread;

      //setcontext(&new_current_thread->uc);
      swapcontext(&save_head->uc, &new_current_thread->uc);
    }
    exit(0);
}