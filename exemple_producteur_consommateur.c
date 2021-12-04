#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <pthread.h>

#define N 10

int buffer;
int count = 0;
int loops = 10;

sem_t *empty;
sem_t *full;
sem_t *mutex;

void put(int value)
{
    count = 1;
    buffer = value;
}

int get()
{
    count = 0;
    return buffer;
}

void *producer()
{
    printf("producer: begin\n");
    for (int i = 0; i < loops; i++)
    {
        sem_wait(empty);
        sem_wait(mutex);
        put(i);
        sem_post(mutex);
        sem_post(full);
        printf("producer: put %d\n", i);
    }
    printf("producer: end\n");
    return NULL;
}

void *consumer()
{
    printf("consumer: begin\n");
    for (int i = 0; i < loops; i++)
    {
        sem_wait(full);
        sem_wait(mutex);
        int value = get();
        sem_post(mutex);
        sem_post(empty);
        printf("consumer: get %d\n", value);
    }
    printf("consumer: end\n");
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t pro, con;

    empty = sem_open("/empty", O_CREAT, 0644, N);

    full = sem_open("/full", O_CREAT, 0644, 0);

    mutex = sem_open("/mutex", O_CREAT, 0644, N);

    pthread_create(&pro, NULL, producer, NULL);

    pthread_create(&con, NULL, consumer, NULL);

    pthread_join(pro, NULL);
    pthread_join(con, NULL);

    return EXIT_SUCCESS;
}