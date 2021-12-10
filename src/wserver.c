#include <stdio.h>
#include <stdlib.h>
#include "request.h"
#include "io_helper.h"
#include <pthread.h>
#include <semaphore.h>

char default_root[] = ".";

sem_t *empty;
sem_t *full;
pthread_mutex_t mutex;

typedef struct Connection // Le nom du type de la structure
{
	int connection_id;
	struct Connection *next;
} Connection; // L'alias de la structure est élément

typedef struct Queue
{
	Connection *first;
} Queue;

Queue *initialiser()
{
	// pthread_mutex_lock(&mutex);
	Queue *queue = malloc(sizeof(*queue));
	queue->first = NULL;
	// pthread_mutex_unlock(&mutex);
	return queue;
}

struct main_thread_arguments
{
	int threads;
	char *root_dir;
	int port;
	int listen_fd;
};

struct worker_thread_arguments
{
	Queue *queue;
};

void add_to_queue(Queue *queue, int newConnection)
{
	printf("Connect start : %d\n", newConnection);
	Connection *new = malloc(sizeof(*new)); // Alloue un espace mémoire qui servira à stocker la nouvelle connection avant de l'ajouter à la queue
	if (queue == NULL || new == NULL)
	{
		exit(EXIT_FAILURE);
	}

	new->connection_id = newConnection;
	new->next = NULL;

	if (queue->first != NULL) /* La queue n'est pas vide */
	{
		printf("enter queue\n");
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
		printf("first\n");
		queue->first = new;
	}
	printf("Connect end : %d\n", newConnection);
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
		printf("conn id deque : %d\n", connection_id);
		queue->first = queueElement->next;
		free(queueElement);
	}

	return connection_id;
}

void *worker_thread_function(void *queueVoid)
{
	Queue *queue = (Queue *)queueVoid;
	printf("consumer: begin\n");

	while (1)
	{
		// On vérifie qu'il existe une connection à traiter et on recupère le lock
		sem_wait(full);

		printf("start work\n");
		// On récupère la première connection ajoutée (FIFO) et on la traite
		int conn_fd = dequeue(queue);
		printf("OK conn\n");
		printf("conn_fd worker : %d\n", conn_fd);
		pthread_mutex_lock(&mutex);
		request_handle(conn_fd);
		pthread_mutex_unlock(&mutex);
		printf("Request success!\n");
		close_or_die(conn_fd);

		printf("end work\n");

		// On libère le lock et on signale qu'il y a une place de plus disponible dans le buffer
		sem_post(empty);

		printf("consumer: end\n");
	}

	return NULL;
}

void create_connection(char *root_dir, int port, int listen_fd, Queue *queue)
{
	printf("ok\n");
	struct sockaddr_in client_addr;
	int client_len = sizeof(client_addr);
	printf("ok2\n");
	int conn_fd = accept_or_die(listen_fd, (sockaddr_t *)&client_addr, (socklen_t *)&client_len);
	printf("ok3\n");
	printf("conn_fd : %d\n", conn_fd);

	// On vérifie que le buffer n'est pas plein et on recupère le lock
	sem_wait(empty);
	printf("empty ok\n");
	int connection_id = 0;
	while (queue->first != NULL)
	{
		Connection *queueElement = queue->first;

		connection_id = queueElement->connection_id;
		printf("conn id deque : %d\n", connection_id);
		queue->first = queueElement->next;
		free(queueElement);
	}

	pthread_mutex_lock(&mutex);

	add_to_queue(queue, conn_fd);

	// On libère le lock et on signale qu'il y a une nouvelle connection à traiter
	pthread_mutex_unlock(&mutex);
	sem_post(full);
}

void *master_thread_function(void *main_thread_argumentsVoid)
{
	printf("producer: begin\n");
	struct main_thread_arguments *argumentsMaster = (struct main_thread_arguments *)main_thread_argumentsVoid;

	int threads = argumentsMaster->threads;
	char *root_dir = argumentsMaster->root_dir;
	int port = argumentsMaster->port;
	int listen_fd = argumentsMaster->listen_fd;

	Queue *queue = initialiser();

	// pthread_t threads_pool[threads];

	struct worker_thread_arguments arguments;

	arguments.queue = queue;

	// pthread_mutex_lock(&mutex);

	for (int i = 0; i < threads; i++)
	{
		pthread_t worker_thread;
		pthread_create(&worker_thread, NULL, worker_thread_function, (void *)&arguments);
	}
	// pthread_mutex_unlock(&mutex);

	while (1)
	{
		// On ajoute la nouvelle connection à la fin du buffer (FIFO)
		create_connection(root_dir, port, listen_fd, (void *)&arguments);
	}

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

	if (strstr(root_dir, "..") != NULL)
	{
		printf("Not in subtree of the server\n");
		exit(EXIT_FAILURE);
	}

	// run out of this directory
	chdir_or_die(root_dir);

	// now, get to work
	int listen_fd = open_listen_fd_or_die(port);

	// réinitialisation des sémaphores
	sem_unlink("/empty");
	sem_unlink("/full");

	// Création des sémaphores
	empty = sem_open("/empty", O_CREAT, 0644, buffer_size);

	full = sem_open("/full", O_CREAT, 0644, 0);

	pthread_mutex_init(&mutex, NULL);

	struct main_thread_arguments arguments;

	arguments.threads = threads;
	arguments.root_dir = root_dir;
	arguments.port = port;
	arguments.listen_fd = listen_fd;

	pthread_t master_thread;
	pthread_create(&master_thread, NULL, master_thread_function, (void *)&arguments);
	pthread_join(master_thread, NULL);

	return 0;
}
