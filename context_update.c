#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define NUM_COROUTINES 2

ucontext_t contexts[NUM_COROUTINES];
int current_coroutine = 0;

void f1() {
    printf("Début de la tâche 1\n");
    for (int i = 0; i < 3; i++) {
        printf("Tâche 1 - Étape %d\n", i+1);
        swapcontext(&contexts[0], &contexts[1]);
    }
    printf("Fin de la tâche 1\n");
}

void f2() {
    printf("Début de la tâche 2\n");
    for (int i = 0; i < 3; i++) {
        printf("Tâche 2 - Étape %d\n", i+1);
        swapcontext(&contexts[1], &contexts[0]);
    }
    printf("Fin de la tâche 2\n");
}

int main() {
    // Initialisation des contextes
    getcontext(&contexts[0]); 
    contexts[0].uc_stack.ss_size = 64*1024;
    contexts[0].uc_stack.ss_sp = malloc(contexts[0].uc_stack.ss_size);
    contexts[0].uc_link = NULL;
    makecontext(&contexts[0], (void (*)(void)) f1, 0);

    getcontext(&contexts[1]);
    contexts[1].uc_stack.ss_size = 64*1024;
    contexts[1].uc_stack.ss_sp = malloc(contexts[1].uc_stack.ss_size);
    contexts[1].uc_link =NULL;// &contexts[0];
    makecontext(&contexts[1], (void (*)(void)) f2, 0);

    // Lancement de la première tâche
    setcontext(&contexts[1]);
    printf("fin de main\n");

    return 0;
}
