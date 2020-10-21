// File:	mypthread_t.h

// List all group member's name: Rithvik Aleshetty, Minhesota Geusic
// username of iLab:
// iLab Server:

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

/* include lib header files that you need here: */
//already added
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

//custom
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
//define all possible states for the thread
typedef enum Thread_State {
	Running = 0,
	Ready = 1,
	Scheduled = 2,
	Blocked = 3,
	Done = 4
} Thread_State;

typedef enum Schedule_Proceed_State {
	TimerInterrupt = 0,
	Yielded = 1,
	Mutexed = 2,
	Joined = 3,
	Finished = 4
} Schedule_Proceed_State;

// YOUR CODE HERE
typedef struct Node{
	void * data;
	struct Node * next;
} Node;

typedef struct Queue{
	int count;
	Node * front;
	Node * rear;
} Queue;

typedef uint mypthread_t;

typedef struct threadControlBlock {
	/* add important states in a thread control block */
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	// YOUR CODE HERE
	//state of thread
	Thread_State thread_state;
	//context switcher
	ucontext_t thread_context;
	//thread type?
	mypthread_t thread_id;
	//thread priority
	double thread_priority;
	char * thread_stack;
	//return val
	void ** joined_val;
	void * return_val;
	//scheduler information
	double elapsed_time_quantum;
	struct timeval last_recorded_time;

} tcb;

/* mutex struct definition */
typedef struct mypthread_mutex_t {
	//thread which locked the critical section
	mypthread_t thread;

	//the list of threads which are being blocked
	Queue* list;

	//the flag variable we manipulate with atomic instruction
	volatile unsigned int available;
} mypthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)


/* Function Declarations: */

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU procession to other user level threads voluntarily */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initial the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);


void PrintMyQueue();

/* get fresh thread id */
mypthread_t FreshThreadID ();

/* get node associated with thread id*/
Node * GetNode (mypthread_t threadID);

/* get the queue containing the given node */
Queue * FindQueueContainingNode (Node * node);

/* get the queue containing the given threadID */
Queue * FindQueueContainingThreadID (mypthread_t threadID);

/* remove given node from the master queue */
int mypthread_DequeueNode(Node * node);

Queue * CreateQueue();

int isempty(Queue * q);

int DequeueNode(Queue *queue, Node *node);

void Enqueue(Queue *queue, Node *node);

Node* Dequeue(Queue *queue);

Node * Peek(Queue *queue);

#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
//#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
//#define pthread_mutex_init mypthread_mutex_init
//#define pthread_mutex_lock mypthread_mutex_lock
//#define pthread_mutex_unlock mypthread_mutex_unlock
//#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#endif
