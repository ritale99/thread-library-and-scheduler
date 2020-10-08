// File:	mypthread.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "mypthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

//multi-level queue
Queue * master_queue;

//main context
static ucontext_t main_context;

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {
	// create Thread Control Block
    // create and initialize the context of this thread
    // allocate space of stack for this thread to run
    // after everything is all set, push this thread int
    // YOUR CODE HERE

	//if master queue is empty
	//we create a new master queue,
	//and the first level queue as well
	if ( master_queue == NULL ) master_queue = (Queue *) malloc (sizeof(Queue));
	if ( isEmpty (master_queue) ){
		Queue * queue_1 = (Queue*)malloc (sizeof(Queue));
		Node * node = (Node *) malloc (sizeof(Node));
		node->data = queue_1;
		Enqueue(master_queue, node);
	}

	//We need to figure out based on the scheduler which queue level to send this thread too
	//right now I'm assuming the first level

	//initialization
	Node * thread_node = NULL;
	char thread_stack [] = NULL;
	tcb * tcb_ptr = NULL;

	//do we have to create a pthread?

	//allocate the stack based on the SIGSTKSZ size
	thread_stack = malloc(SIGSTKSZ);
	if ( thread_stack == NULL ) return -1;
	//allocate space for tcb
	tcb_ptr = (tcb) malloc(sizeof(tcb));
	if ( tcb_ptr == NULL ){
		free(thread_stack);
		return -1;
	}
	//allocate space for the thread_node
	thread_node = (Node *) malloc (sizeof(Node));
	if ( thread_node == NULL ) {
		free(tcb_ptr);
		free(thread_stack);
		return -1;
	}

	//assign thread_id
	tcb_ptr->thread_id = 0;

	//assign thread_state to ready state
	tcb_ptr->thread_state = Ready;

	//assign thread_t
	tcb_ptr->thread_t = thread;

	//create thread context
	ucontext_t * thread_context_ptr = &(tcb_ptr->thread_context);

	//assign current context to thread_context_ptr
	if ( getcontext(thread_context_ptr) == -1 ) {
		//we have received an error in an attempt
		//to get the thread
		free(thread_stack);
		free(tcb_ptr);
		return -1;
	}

	//define the successor context when this context is done
	//it should be the main context
	thread_context_ptr->uc_link = &main_context;
	//set the thread stack
	thread_context_ptr->uc_stack.ss_sp = thread_stack;
	thread_context_ptr->uc_stack.ss_size = sizeof(thread_stack);

	makecontext( thread_context_ptr, function, arg ) ;

	//TODO: other things are needed to complete this function

	//currently let the thread_node data be the TCB
	thread_node->data = tcb_ptr;

	//enqueue to the front node (run node)
	Enqueue(master_queue->front, thread_node);

	//scheduler

    return tcb_ptr->thread_id;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// with from thread context to scheduler context

	if ( master_queue == NULL ) return -1;
	if ( main_context == NULL ) return -1;
	//get current context
	//swap context with main
	//put thread in ready state

	Node * curr_tcb_node = NULL;
	ucontext_t * curr_context = NULL;

	if ( getcontext(curr_context) == -1 ) return -1;
	if ( curr_context == NULL ) return -1;

	curr_tcb_node = GetNode(master_queue, curr_context);

	if ( curr_tcb_node == NULL
			|| ((tcb *) curr_tcb_node)->) return -1;

	// YOUR CODE HERE
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library
	// should be contexted switched from thread context to this
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

// schedule policy
#ifndef MLFQ
	// Choose STCF
#else
	// Choose MLFQ
#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE

//Get the node associated with the given threadID
Node * GetNode(Queue * master_queue, int threadID){
	if (master_queue == NULL) return NULL;
	if (isempty(master_queue)) return NULL;
	//get the first queue from the master_queue
	Node * queue_ptr = master_queue->front;
	//loop through each queue level
	while ( queue_ptr != NULL ){
		if (queue_ptr->data != NULL) {
			//get the first thread node
			Node * thread_ptr = ((Queue*)queue_ptr->data)->front;
			//loop through each thread node
			while ( thread_ptr != NULL ) {
				//check if thread node has the matching threadID
				if ( thread_ptr->data != NULL &&
						((tcb *)thread_ptr->data)->thread_id == threadID) {
					return thread_ptr;
				}
				thread_ptr = thread_ptr->next;
			}
		}
		queue_ptr = queue_ptr->next;
	}
}

//Get the tcb node based on the threadContext
Node * GetNode ( Queue * master_queue , ucontext_t * threadContext ){
	if (master_queue == NULL) return NULL;
	if (isempty(master_queue)) return NULL;
	//get the first queue from the master_queue
	Node * queue_ptr = master_queue->front;
	//loop through each queue level
	while ( queue_ptr != NULL ){
		if (queue_ptr->data != NULL) {
			//get the first thread node
			Node * thread_ptr = ((Queue*)queue_ptr->data)->front;
			//loop through each thread node
			while ( thread_ptr != NULL ) {
				//check if thread node has the matching threadID
				if ( thread_ptr->data != NULL &&
						((tcb *)thread_ptr->data)->thread_context == *threadContext) {
						return thread_ptr;
				}
				thread_ptr = thread_ptr->next;
			}
		}
		queue_ptr = queue_ptr->next;
	}
}
//Generic Queue
Queue * CreateQueue(){
	Queue * queue = (Queue *) (malloc(sizeof(Queue)));
	if ( queue == NULL ) return NULL;
	queue->count = 0;
	queue->front = NULL;
	queue->rear = NULL;
	return queue;
}
int isempty(Queue *q) {
    return (q->rear == NULL);
}
int DequeueNode( Queue * queue , Node * node){
	if ( node == NULL ) return 0;
	if ( queue == NULL ) return 0;
	Node * ptr = queue->front;
	Node * prevPtr = NULL;
	while ( ptr != NULL ) {
		if ( ptr == node ){
			if (prevPtr == NULL){
				queue->front = NULL;
				queue->rear = NULL;
				queue->count--;
				return 1;
			}
			else{
				prevPtr->next = ptr->next;
				ptr->next = NULL;
				queue->count--;
				return 1;
			}
		}
		prevPtr = ptr;
		ptr = ptr->next;
	}
	return 0;
}
void Enqueue(Queue * queue , Node * node){
	if ( node == NULL ) return;
	if ( queue == NULL ) return;
	if ( !isempty(queue) ){
		queue->rear->next = node;
		queue->rear = node;
	}
	else {
		queue->front = queue->rear = node;
	}
	queue->count++;
}
Node * Dequeue(Queue * queue){
	if ( queue == NULL || isempty(queue) ) return NULL;
	Node * rtn;
	rtn = queue->front;
	queue->front = queue->front->next;
	queue->count--;
	return rtn;
}
Node * GetNode (Queue * queue, void * data) {
	if ( data == NULL ) return NULL;
	if ( queue == NULL ) return NULL;
	if ( !isempty(queue) ) return NULL;
	Node * rtn = queue->front;
	while ( rtn != NULL ) {
		if ( rtn->data == data ) return rtn;
		rtn = rtn->next;
	}
	return NULL;
}
