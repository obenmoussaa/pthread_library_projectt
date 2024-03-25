#include <stdio.h>
#include "thread.h"
#include "queue.h"
#include <ucontext.h> /* ne compile pas avec -std=c89 ou -std=c99 */
#include <stdlib.h>


#define STACK_SIZE 1000
/* identifiant de thread
 * NB: pourra être un entier au lieu d'un pointeur si ca vous arrange,
 *     mais attention aux inconvénient des tableaux de threads
 *     (consommation mémoire, cout d'allocation, ...).
 */
typedef void * thread_t;

struct thread_node_t
{
    ucontext_t context;
    unsigned int * id;
    void* retval;
    void* funcarg;
    void* (* func) (void* );
};

struct thread_queue_entry
{
    thread_t * thread_ptr;
    SIMPLEQ_ENTRY(thread_queue_entry) entries;
};

SIMPLEQ_HEAD(thread_queue, thread_queue_entry) ready_queue;
ucontext_t main_context;
struct thread_queue ready_queue = SIMPLEQ_HEAD_INITIALIZER(ready_queue);

/* recuperer l'identifiant du thread courant.
 */
thread_t thread_self(void) {
    printf("%p\n", ready_queue);

    return (thread_t) &ready_queue;
}

/* creer un nouveau thread qui va exécuter la fonction func avec l'argument funcarg.
 * renvoie 0 en cas de succès, -1 en cas d'erreur.
 */
int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg) {
    struct thread_node_t *thread = malloc(sizeof(struct thread_node_t));
    if (thread == NULL) {
        return -1;
    }

    getcontext(&thread->context);
    thread->context.uc_stack.ss_sp = malloc(STACK_SIZE);
    thread->context.uc_stack.ss_size = STACK_SIZE;
    thread->context.uc_link = &main_context;
    makecontext(&thread->context, (void (*)(void))func, 1, funcarg);

    struct thread_queue_entry *entry = malloc(sizeof(struct thread_queue_entry));
    if (entry == NULL) {
        free(thread);
        return -1;
    }
    entry->thread_ptr = (thread_t *)thread;
    SIMPLEQ_INSERT_TAIL(&ready_queue, entry, entries);

    // thread_yield();

    *newthread = (thread_t)thread;
    return 0;
}

/* passer la main à un autre thread.
 */
int thread_yield(void) {
    struct thread_queue_entry *entry = SIMPLEQ_FIRST(&ready_queue);
    if (entry == NULL) {
        return -1;
    }

    struct thread_node_t *current_thread = (struct thread_node_t *)entry->thread_ptr;
    SIMPLEQ_REMOVE_HEAD(&ready_queue, entries);
    swapcontext(&current_thread->context, &main_context);

    return 0;
}

/* attendre la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 */
int thread_join(thread_t thread, void **retval) {
    struct thread_node_t *th = (struct thread_node_t *)thread;
    if (th == NULL) {
        return -1;
    }

    // thread_yield();

    if (retval != NULL) {
        *retval = th->retval;
    }

    free(th->context.uc_stack.ss_sp);
    free(th);

    return 0;
}


/* terminer le thread courant en renvoyant la valeur de retour retval.
 * cette fonction ne retourne jamais.
 *

 * L'attribut noreturn aide le compilateur à optimiser le code de
 * l'application (élimination de code mort). Attention à ne pas mettre
 * cet attribut dans votre interface tant que votre thread_exit()
 * n'est pas correctement implémenté (il ne doit jamais retourner).
 */
 __attribute__ ((__noreturn__))  void thread_exit(void *retval){
        // printf("hhh\n");
    struct thread_node_t *current_thread = (struct thread_node_t *)thread_self();
    current_thread->retval = retval;

    // thread_yield();

    struct thread_queue_entry *entry = SIMPLEQ_FIRST(&ready_queue);
    if (entry == NULL) {
        // Aucun autre thread en attente, on retourne au contexte principal
        setcontext(&main_context);
    } else {
        struct thread_node_t *next_thread = (struct thread_node_t *)entry->thread_ptr;
        SIMPLEQ_REMOVE_HEAD(&ready_queue, entries);
        swapcontext(&current_thread->context, &next_thread->context);
    }

    // Cette ligne ne devrait jamais être atteinte
    __builtin_unreachable();
}

