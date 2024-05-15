#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "thread.h"
#include "queue.h"
#include <valgrind/valgrind.h>
#include <errno.h>
#include <ucontext.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#define TIMESLICE_DURATION 300
#include <stdatomic.h>

#define STACK_SIZE (64 * 1024)

struct thread
{
  thread_t id;
  int blocked;
  int finished;
  struct thread *waiting_threads;
  void *(*func)(void *);
  void *funcarg;
  ucontext_t uc;
  void *ret;
  int stack_id; // valgrind peut determiner quelle thread est responsable d une fuite memoire
  TAILQ_ENTRY(thread)
  queue_threads;
};

struct thread *main_thread;
struct thread *current_thread = NULL;

TAILQ_HEAD(tailq, thread)
run_queue = TAILQ_HEAD_INITIALIZER(run_queue);
TAILQ_HEAD(trash, thread)
dead_queue = TAILQ_HEAD_INITIALIZER(dead_queue); // Pour la liberation des ressources
#ifdef ENABLEPREEMPTION
struct itimerval timer;
struct sigaction sa;
#endif
int valgrind_stackid;




#ifdef ENABLEPREEMPTION

void schedule_restart()
{
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = TIMESLICE_DURATION;
  timer.it_value = timer.it_interval;
  setitimer(ITIMER_REAL, &timer, NULL);
}
// Timer signal handler
void timer_handler(int signum)
{
  (void)signum;

  schedule_restart();
  thread_yield();
}

// Scheduler initialization
void scheduler_init()
{
  // Configure the timer signal handler
  sa.sa_handler = timer_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGALRM, &sa, NULL);

  // Configure the timer to send the signal after each timeslice
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = TIMESLICE_DURATION;
  timer.it_value = timer.it_interval;
  setitimer(ITIMER_REAL, &timer, NULL);
}

#endif

// #endif
//   __attribute__((constructor)) is used to mark a function as a constructor,
//  which means it will be automatically called before the main() function when
//  the program starts.
// ici pour initialiser main_thread
__attribute__((constructor)) void init_thread(void)
{
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
  #ifdef ENABLEPREEMPTION
    printf("enable preemption defined\n");
    scheduler_init();
  #endif
}

__attribute__((destructor)) void free_threads(void)
{
  if (!main_thread->finished)
  {
    main_thread->finished = 1;
    TAILQ_INSERT_HEAD(&dead_queue, main_thread, queue_threads);
    // TAILQ_REMOVE(&run_queue, main_thread, queue_threads);
    // free(main_thread);
  }
  while (!TAILQ_EMPTY(&dead_queue))
  {
    struct thread *save_head = TAILQ_FIRST(&dead_queue);
    TAILQ_REMOVE(&dead_queue, save_head, queue_threads);
    if (save_head != main_thread)
    {
      free(save_head->uc.uc_stack.ss_sp);
      VALGRIND_STACK_DEREGISTER(save_head->stack_id);
    }
    free(save_head);
  }
}

void wrap_func(struct thread *thread)
{
  void *retval = thread->func(thread->funcarg);
  if (!thread->finished)
  {
    thread_exit(retval);
  }
}

int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg)
{
  struct thread *thread = malloc(sizeof(*thread));
  thread->id = *newthread;
  thread->waiting_threads = NULL;
  thread->blocked = 0;
  thread->func = func;
  thread->funcarg = funcarg;
  thread->ret = NULL;
  *newthread = (thread_t)thread;
  thread->finished = 0;
  TAILQ_INSERT_TAIL(&run_queue, thread, queue_threads);
  getcontext(&thread->uc);
  thread->uc.uc_stack.ss_size = STACK_SIZE;
  thread->uc.uc_stack.ss_sp = malloc(thread->uc.uc_stack.ss_size);
  thread->stack_id = VALGRIND_STACK_REGISTER(thread->uc.uc_stack.ss_sp,                                // debut de la pile
                                             thread->uc.uc_stack.ss_sp + thread->uc.uc_stack.ss_size); // fin
  thread->uc.uc_link = NULL;
  makecontext(&thread->uc, (void (*)(void))wrap_func, 1, (intptr_t)thread);
  return 0;
}

thread_t thread_self(void)
{
  return (thread_t)current_thread;
}

int thread_yield(void)
{
  struct thread *save_head = current_thread;
  TAILQ_REMOVE(&run_queue, current_thread, queue_threads);
  // si le thread en tete de la file n 'est pas bloqué alors il est prêt à s executer,
  // alors current thread doit être préémenté et mis à la fin de la file pour que 1st thread peut s executer

  TAILQ_INSERT_TAIL(&run_queue, current_thread, queue_threads);

  struct thread *new_current_thread = TAILQ_FIRST(&run_queue);
  current_thread = new_current_thread;
  // le ca ou la liste continet qu'un seul élement.
  swapcontext(&save_head->uc, &new_current_thread->uc);

  return 0;
}

int thread_join(thread_t thread, void **retval)
{
  struct thread *target_thread = (struct thread *)thread;
  if (!target_thread->finished)
  {
    target_thread->waiting_threads = current_thread;
    struct thread *save_head = TAILQ_FIRST(&run_queue);
    TAILQ_REMOVE(&run_queue, current_thread, queue_threads);
    // le thread qui prendra la main est le premier de la file
    struct thread *new_current_thread = TAILQ_FIRST(&run_queue);
    current_thread = new_current_thread;
    // c'est pas important de donner la main au tread qui attend.
    swapcontext(&save_head->uc, &new_current_thread->uc);
  }

  if (retval != NULL)
  {
    *retval = target_thread->ret;
  }

  return 0;
}

void thread_exit(void *retval)
{
  struct thread *save_head = TAILQ_FIRST(&run_queue);
  current_thread->finished = 1;
  current_thread->ret = retval;
  TAILQ_REMOVE(&run_queue, current_thread, queue_threads);
  if (current_thread == main_thread)
  {
    TAILQ_INSERT_HEAD(&dead_queue, current_thread, queue_threads);
  }
  else
  {
    TAILQ_INSERT_TAIL(&dead_queue, current_thread, queue_threads);
  }

  if (save_head->waiting_threads != NULL)
  {
    TAILQ_INSERT_HEAD(&run_queue, save_head->waiting_threads, queue_threads);
  }
  if (!TAILQ_EMPTY(&run_queue))
  {
    struct thread *new_current_thread = TAILQ_FIRST(&run_queue);
    current_thread = new_current_thread;
    swapcontext(&save_head->uc, &new_current_thread->uc);
  }
  else
  {
    setcontext(&main_thread->uc);
  }
  exit(0); // ce sert à rien.
           // s'il y a paersonne dans le liste
}

/**************Mutex*********************/

int thread_mutex_init(thread_mutex_t *mutex)
{
  if (mutex != NULL)
  {
    mutex->thread_lock = NULL;
    mutex->is_destroyed = 0;
    TAILQ_INIT(&mutex->wait_queue);
    return 0;
  }
  return 1;
}

int thread_mutex_destroy(thread_mutex_t *mutex)
{
  mutex->thread_lock = NULL;
  mutex->is_destroyed = 1;
  return 0;
}

int thread_mutex_lock(thread_mutex_t *mutex)
{
  if (mutex->is_destroyed)
  {
    return 1;
  }

  // Si la section critique est occupée
  while (mutex->thread_lock != NULL)
  {
    if (!TAILQ_EMPTY(&run_queue))
    {
      struct thread *head_thread = TAILQ_FIRST(&run_queue);
      TAILQ_REMOVE(&run_queue, current_thread, queue_threads);
      current_thread->blocked = 1;
      TAILQ_INSERT_TAIL(&mutex->wait_queue, current_thread, queue_threads);

      struct thread *new_current_thread = TAILQ_FIRST(&run_queue);
      current_thread = new_current_thread;

      swapcontext(&head_thread->uc, &new_current_thread->uc);
    }
  }

  mutex->thread_lock = (thread_t)current_thread;

  return 0;
}

int thread_mutex_unlock(thread_mutex_t *mutex)
{
  if (mutex->is_destroyed)
  {
    return 1;
  }
  mutex->thread_lock = NULL;

  if (!TAILQ_EMPTY(&mutex->wait_queue))
  {
    struct thread *head_thread = TAILQ_FIRST(&mutex->wait_queue);
    TAILQ_REMOVE(&mutex->wait_queue, head_thread, queue_threads);
    head_thread->blocked = 0;
    TAILQ_INSERT_TAIL(&run_queue, head_thread, queue_threads);
  }
  return 0;
}
// libération dans le thread_join et dans le destrcuteur.