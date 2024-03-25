#include <stdio.h>
#include "thread.h"
#include <stdlib.h>

void *thread_function(void *arg) {
    int *val = (int *)arg;
    printf("Thread function received argument: %d\n", *val);
    *val *= 2; // Modify the argument
    thread_exit(NULL);
}

int main() {
    thread_t thread1, thread2;
    int arg1 = 10, arg2 = 20;
    printf("%p\n",  thread_self() );
    printf("Creating threads...\n");

    if (thread_create(&thread1, thread_function, (void *)&arg1) == -1) {
        perror("Error creating thread 1");
        exit(EXIT_FAILURE);
    }

    if (thread_create(&thread2, thread_function, (void *)&arg2) == -1) {
        perror("Error creating thread 2");
        exit(EXIT_FAILURE);
    }

    printf("Threads created.\n");

    // Wait for threads to finish
    void *retval;
    thread_join(thread1, &retval);
    printf("Thread 1 joined.\n");

    thread_join(thread2, &retval);
    printf("Thread 2 joined.\n");

    printf("Modified arguments: %d, %d\n", arg1, arg2);

    return 0;
}
