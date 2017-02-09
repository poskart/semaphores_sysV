/*
 * synchro.h
 *
 *  Created on: 9 gru 2016
 *      Author: piotr
 */

#ifndef SYNCHRO_H_
#define SYNCHRO_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>


union semun
{
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};


/*
 * Perform operation on semaphore:
 *
 * 	semid - id of semaphore
 * 	sem_number - semaphore number in the set of semaphores
 * 	sem_operation - int value which abs. value will be added or substracted
 * 					from sem. value, depending on the sign
 * 	sembuf - pointer to sembuf structure
 */
void sem_action(int semid, int sem_number, int sem_operation, struct sembuf * ptr);


/*
 * Initialize semaphore with the given value initial_value
 * http://www.linuxpl.org/LPG/node52.html
 */
void sem_init(int semid, int semaphore_number, int initial_value);


#endif /* SYNCHRO_H_ */
