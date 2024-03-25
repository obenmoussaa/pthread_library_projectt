#include <stdio.h>
#include "thread.h"
#include "queue.h"
#include <ucontext.h> /* ne compile pas avec -std=c89 ou -std=c99 */


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

struct  thread_queue_entry
{
    thread_t * thread_ptr;
    SIMPLEQ_ENTRY(thread_queue_entry);
};

SIMPLEQ_HEAD(thread_queue, thread_queue_entry);






/* recuperer l'identifiant du thread courant.
 */
thread_t thread_self(void);

/* creer un nouveau thread qui va exécuter la fonction func avec l'argument funcarg.
 * renvoie 0 en cas de succès, -1 en cas d'erreur.
 */
int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg);

/* passer la main à un autre thread.
 */
int thread_yield(void);

/* attendre la fin d'exécution d'un thread.
 * la valeur renvoyée par le thread est placée dans *retval.
 * si retval est NULL, la valeur de retour est ignorée.
 */
int thread_join(thread_t thread, void **retval);

/* terminer le thread courant en renvoyant la valeur de retour retval.
 * cette fonction ne retourne jamais.
 *
 * L'attribut noreturn aide le compilateur à optimiser le code de
 * l'application (élimination de code mort). Attention à ne pas mettre
 * cet attribut dans votre interface tant que votre thread_exit()
 * n'est pas correctement implémenté (il ne doit jamais retourner).
 */
void thread_exit(void *retval) __attribute__ ((__noreturn__));