#include <stdio.h>
#include <stdlib.h>
#include "request.h"
#include "io_helper.h"
#include <pthread.h>
#include <semaphore.h>

char default_root[] = ".";

int[] buffer;

sem_t *empty;
sem_t *full;
sem_t *mutex;

typedef struct Connection // Le nom du type de la structure
{
	int connection_id;
	Connection *next;
} Connection; // L'alias de la structure est élément

typedef struct Queue
{
	Connection *first;
} Queue;

void add_to_queue(Queue *queue, int newConnection)
{
	Connection *new = malloc(sizeof(*new)); // Alloue un espace mémoire qui servira à stocker la nouvelle connection avant de l'ajouter à la queue
	if (queue == NULL || new == NULL)
	{
		exit(EXIT_FAILURE);
	}

	new->connection_id = newConnection;
	new->next = NULL;

	if (queue->first != NULL) /* La queue n'est pas vide */
	{
		/* On se positionne à la fin de la queue */
		Connection *currentElement = queue->first;
		while (currentElement->next != NULL)
		{
			currentElement = currentElement->next;
		}
		currentElement->next = new;
	}
	else /* La queue est vide, notre élément est le first */
	{
		queue->first = new;
	}
}

int dequeue(Queue *queue)
{
	if (queue == NULL)
	{
		exit(EXIT_FAILURE);
	}

	int connection_id = 0;

	/* On vérifie s'il y a quelque chose à défiler */
	if (queue->first != NULL)
	{
		Connection *queueElement = queue->first;

		connection_id = queueElement->connection_id;
		queue->first = queueElement->next;
		free(queueElement);
	}

	return connection_id;
}

void *worker_thread_function()
{

	printf("consumer: begin\n");

	// On vérifie qu'il existe une connection à traiter et on recupère le lock
	sem_wait(full);
	sem_wait(mutex);

	// On récupère la première connection ajoutée (FIFO) et on la traite
	int conn_fd = dequeue();
	request_handle(conn_fd);
	close_or_die(conn_fd);

	// On libère le lock et on signale qu'il y a une place de plus disponible dans le buffer
	sem_post(mutex);
	sem_post(empty);

	printf("consumer: end\n");
	return NULL;
}

void *master_thread_function(int *threads)
{
	pthread_t[&threads] threads_pool;
	for (int i = 0; i < threads; i++){
		pthread_t 
		pthread_create(&pro, NULL, producer, NULL);
	}
	
	
	printf("producer: begin\n");

	// On vérifie que le buffer n'est pas plein et on recupère le lock
	sem_wait(empty);
	sem_wait(mutex);

	// On ajoute la nouvelle connection à la fin du buffer (FIFO)
	add_to_queue(param);

	// On libère le lock et on signale qu'il y a une nouvelle connection à traiter
	sem_post(mutex);
	sem_post(full);
	printf("producer: put %d\n", i);
   
    printf("producer: end\n");
    return NULL;
}

//
// ./wserver [-d <basedir>] [-p <portnum>] [-t <number_of_thread>] [-b <buffer_size>]
//
int main(int argc, char *argv[])
{
	int c;
	char *root_dir = default_root;
	int port = 10000;
	int threads = 1;
	int buffer_size = 1;

	while ((c = getopt(argc, argv, "d:p:t:b:")) != -1)
		switch (c)
		{
		case 'd':
			root_dir = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			threads = atoi(optarg);
			break;
		case 'b':
			buffer_size = atoi(optarg);
			break;
		default:
			fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-t threads] [-b buffers]\n");
			exit(1);
		}

	// run out of this directory
	chdir_or_die(root_dir);

	// now, get to work
	int listen_fd = open_listen_fd_or_die(port);

	// Création des sémaphores
	empty = sem_open("/empty", O_CREAT, 0644, buffer_size);

    full = sem_open("/full", O_CREAT, 0644, 0);

    mutex = sem_open("/mutex", O_CREAT, 0644, 1);

	while (1)
	{
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		int conn_fd = accept_or_die(listen_fd, (sockaddr_t *)&client_addr, (socklen_t *)&client_len);
		request_handle(conn_fd);
		close_or_die(conn_fd);
	}
	return 0;
}
