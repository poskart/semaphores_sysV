#include "synchro.h"
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>

/* Queue definition:
 *
 * 		[ _ _ _ _ _ _ ... _ _ _ _ _ fl bi ei]
 *
 *	_ - fields of proper array (QUEUE_SIZE empty places)
 *	bi - begin index - index of first element in the queue (first to be popped)
 *	ei - end index - index behind the last element in the queue (behind last added)
 *	fl - flag, which indicated if there are any elements in the queue
 *
 */

#define QUEUE_SIZE 15

#define NPRODUCERS 3
#define NCONSUMERS 5

struct queueObject
{
	pid_t buffer[QUEUE_SIZE];
	bool isAnyInQueue;
	unsigned beginIndex;
	unsigned endIndex;
};


int shmid;
int semid;
struct queueObject *queue;
struct sembuf sem = {0, 0, 0};

int child_pid_array[8];

void producerTask(int type);
void consumerTask(int type);

/* Handler for parent process - clearing memory */
void ctrlC_handler(int dummy);

/* Handler for child processes - detaching memory */
void sigterm_handler(int dummy);

enum { MUTX, FULL, EMPT };


int main(int argc, char *argv[])
{
	int pid;
	int i = 0;

	/* convert a pathname and a project identifier to a System V IPC
       key which can be suitable for use with
       msgget(2), semget(2), or shmget(2).*/
	key_t key = ftok(argv[0], 1);


	/*
	 * Allocate space for the queue
	 *
	 * When the shared memory segment is created, it shall be initialized with all zero values.
	 * http://pubs.opengroup.org/onlinepubs/009695399/functions/shmget.html
	 */
	if(	(shmid = shmget(key, sizeof(struct queueObject), S_IRUSR | S_IWUSR | IPC_CREAT)) == -1)
	{
		perror("shmget\n");
		error(1);
	}

	/* Allocate 3 semaphores and get its semid */
	if(	(semid = semget(key, 3, S_IRUSR | S_IWUSR | IPC_CREAT)) == -1)
	{
		perror("semget\n");
		error(1);
	}

	/*
	 * Attach shared momory (descibed by shmid) to memory of base process
	 */
	if((queue = shmat(shmid, NULL, 0)) == -1)
	{
		perror("shmat\n");
		error(1);
	}

	memset(queue->buffer, 0, QUEUE_SIZE * sizeof(pid_t));
	queue->beginIndex = 0;
	queue->endIndex = 0;
	queue->isAnyInQueue = false;

	sem_init(semid, MUTX, 1);
	sem_init(semid, FULL, 0);
	sem_init(semid, EMPT, QUEUE_SIZE);

	/*
	 * Call producers
	 */
	for(i = 0 ; i < NPRODUCERS; i++)
	{
		pid = fork();
		child_pid_array[i] = pid;	// save pid
		if(pid == 0)
		{
			producerTask(i + 1);
		}
	}

	/*
	 * Call consumers
	 */
	for(i = 0 ; i < NCONSUMERS; i++)
	{
		pid = fork();
		child_pid_array[i + NPRODUCERS] = pid;	// save pid
		if(pid == 0)
		{
			consumerTask(i + 1);
		}
	}

	// Set handler function for SIGINT signal (Ctrl + C)
	if (signal(SIGINT, ctrlC_handler) == SIG_ERR)
		printf("\ncan't catch SIGINT\n");
	else
		printf("SIGINT set successfully...\n");

	/*
	 * Buffer monitoring
	 */

	int elem_counter;
	while(1)
	{
		elem_counter = 0;
		sem_action(semid, MUTX, -1, &sem);


		/* If queue not empty */
		if(queue->isAnyInQueue)
		{
			i = queue->beginIndex;
			printf("\nQueue content:\n");
			if(queue->beginIndex == queue->endIndex)
				printf("(Queue full)\n");
			do
			{
				elem_counter++;
				printf("%d\t", queue->buffer[i]);
				i = (i + 1) % QUEUE_SIZE;
			}while(i != queue->endIndex);
			printf("\t__%d__ elements in the queue\n", elem_counter);
		}
		else
		{
			printf("\nEmpty buffer...\n");
		}
		sem_action(semid, MUTX, 1, &sem);
		sleep(1);
	}

	return 0;
}

void producerTask(int type)
{
	int i;
	/*
	 * Attach shared momory (descibed by shmid) to memory of new process
	 */
	if((queue = shmat(shmid, NULL, 0)) == -1)
	{
		perror("shmat\n");
		error(1);
	}
	// Set handler function for SIGTERM signal
	if (signal(SIGTERM, sigterm_handler) == SIG_ERR)
		printf("\ncan't catch SIGTERM\n");
	else
		printf("SIGTERM set successfully...\n");

	while(1)
	{
		sem_action(semid, EMPT, -type, &sem);
		sem_action(semid, MUTX, -1, &sem);
		for(i = 0 ; i < type; i++)
		{
			queue->buffer[queue->endIndex] = getpid();
			queue->endIndex = (queue->endIndex + 1) % (QUEUE_SIZE);
		}
		/* There is at least one element in the queue */
		queue->isAnyInQueue = 1;
		sem_action(semid, MUTX, 1, &sem);
		sem_action(semid, FULL, type, &sem);
		printf("\nP(%d) %d\n", type, getpid());
		sleep(1);
	}
}

void consumerTask(int type)
{
	int value, i;
	/*
	 * Attach shared momory (descibed by shmid) to memory of new process
	 */
	if((queue = shmat(shmid, NULL, 0)) == -1)
	{
		perror("shmat\n");
		error(1);
	}
	// Set handler function for SIGTERM signal
	if (signal(SIGTERM, sigterm_handler) == SIG_ERR)
		printf("\ncan't catch SIGTERM\n");
	else
		printf("SIGTERM set successfully...\n");

	while(1)
	{
		sem_action(semid, FULL, -type, &sem);
		sem_action(semid, MUTX, -1, &sem);
		for(i = 0 ; i < type; i++)
		{
			value = queue->buffer[queue->beginIndex];
			queue->beginIndex = (queue->beginIndex + 1) % (QUEUE_SIZE);
		}
		/* If there was the last element, clear the flag */
		if(queue->beginIndex == queue->endIndex)
			queue->isAnyInQueue = 0;
		sem_action(semid, MUTX, 1, &sem);
		sem_action(semid, EMPT, type, &sem);
		printf("\nC(%d) %d\n", type, getpid());
		sleep(3);
	}
}

/* Handler for parent process - clearing memory */
void ctrlC_handler(int signo)
{
	if(signo == SIGINT)
	{
		int i;
		// Terminate all child processes
		kill(0, SIGTERM);

		int returnStatus;
		// Wait for child processes to terminate
		for(i = 0 ; i < NCONSUMERS + NPRODUCERS; i++)
			waitpid(child_pid_array[i], &returnStatus, 0);


		// Detach memory
		shmdt(queue);

		// Deallocate semaphores
		semctl(semid, 0, IPC_RMID);

		// Deallocate memory
		shmctl(shmid, IPC_RMID, NULL);
	}
}

/* Handler for child processes - detaching memory */
void sigterm_handler(int signo)
{
	if(signo == SIGTERM)
	{
		sem_action(semid,MUTX, -1, &sem);
		shmdt(queue);
		sem_action(semid,MUTX, 1, &sem);
	}
}
