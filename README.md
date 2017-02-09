# semaphores_sysV
Multi producer - consumer problem with synchronization using UNIX system V semaphores (C).

The buffer in this program is cyclic-buffer of pid_t values with defined size (15 default), which is shared between consumer and producer processes.

There are 3 producer processes (although algorithm is general), which push their process pid values into the buffer in the following way:
- first producer writes one its pid_t to the buffer
- second producer writes two its pid_t values to the buffer
- third producer writes three pid_t values to the buffer
- (etc.)

There are 5 consumers (although algorithm is general), which pull pid_t values from buffer, as in the case of producers:
- first consumer consumes one pid_t
- second consumer consumes two pid_t
- etc.

Pushing and pulling from buffer must be kept in order, in which particular processes came to the critical section e.g. if 
double producer was first in the critical section, other producer process (e.g. triple producer) must not get into buffer and must wait for double producer to finish its work.
Consumer processes must behaves like producer processes - only one consumer can pull from buffer, until it is fed up, and then the next in the queue can start consume.

Author: Piotr Poskart, January 2017 ©

