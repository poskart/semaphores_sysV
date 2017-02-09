/*
 * synchro.c
 *
 *  Created on: 9 gru 2016
 *      Author: piotr
 */

#include "synchro.h"

/*
 * Perform operation on semaphore:
 *
 * 	semid - id of semaphore
 * 	sem_number - semaphore number in the set of semaphores
 * 	sem_operation - int value which abs. value will be added or substracted
 * 					from sem. value, depending on the sign
 * 	sembuf - pointer to sembuf structure
 */
void sem_action(int semid, int sem_number, int sem_operation, struct sembuf * ptr)
{
	ptr->sem_num = sem_number;
	ptr->sem_op = sem_operation;
	ptr->sem_flg = SEM_UNDO;
  if(semop(semid, ptr, 1) == -1)
  {
		perror("semop: ");
		if(sem_operation > 0)
			perror("V\n");
		else
			perror("P\n");
		error(1);
  }
}

/*
 * Initialize semaphore with the given value initial_value
 * http://www.linuxpl.org/LPG/node52.html
 */
void sem_init(int semid, int semaphore_number, int initial_value)
{
	union semun argument;
	argument.val = initial_value;
	if(semctl(semid, semaphore_number, SETVAL, argument) == -1)
	{
		perror("semctl\n");
		error(1);
	}
}
