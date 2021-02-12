#include <stdio.h>
#include <pthread.h>

void *fun(void*arg) {
    printf("Welcome!\n");
    return NULL;
}

int main(void) {
    pthread_t pthread;
    pthread_create(&pthread, NULL, fun, NULL);
    pthread_join(pthread, NULL);
    printf("Bye");
    return 0;
}