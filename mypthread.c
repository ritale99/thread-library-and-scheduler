// File:	mypthread.c

// List all group member's name: Rithvik Aleshetty, Minhesota Geusic
// username of iLab:
// iLab Server:

#include "mypthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

//multi-level queue
Queue * master_queue;

//main context
const static mypthread_t main_thread_id = 0;
static tcb main_tcb;
static ucontext_t main_context;

//scheduler
static Schedule_Policy sched;

//current running thread
mypthread_t curr_thread_id;

/* this function is called when creating new thread */
static void thread_helper (void * (*function) (void*), void * arg){
	printf("Ended here\n");	//debug
	mypthread_exit(function(arg));
}

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
	if ( master_queue == NULL )
		master_queue = CreateQueue();
	if ( master_queue == NULL ) exit(1);
	if ( isempty (master_queue) ){
		Queue * queue_0 = CreateQueue();
		Node * queue_node = (Node *) malloc (sizeof(Node));
		queue_node->data = (void*) queue_0;
		Enqueue(master_queue, queue_node);
	}

	//initialization
	Node * thread_node = NULL;
	char * thread_stack = NULL;
	tcb * tcb_ptr = NULL;
	//currently we dont have a way to generate a fresh id
	//TODO: generate fresh id
	mypthread_t thread_id = FreshThreadID();
	//mypthread_t thread_id = 1;

	//allocate the stack based on the SIGSTKSZ size
	thread_stack = (char *)malloc(SIGSTKSZ);
	if ( thread_stack == NULL ) return -1;

	//allocate space for tcb
	tcb_ptr = (tcb*) malloc(sizeof(tcb));
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

	//assign thread_state to ready state
	tcb_ptr->thread_state = Ready;

	//assign thread_id
	tcb_ptr->thread_id = thread_id;

	//assign thread_stack to tcb
	tcb_ptr->thread_stack = thread_stack;

	if ( thread != NULL ) (*thread) = thread_id;

	//assign thread context
	ucontext_t * thread_context_ptr = &(tcb_ptr->thread_context);

	//assign current context to thread_context_ptr
	if ( getcontext(thread_context_ptr) == -1 ) {
		//we have received an error in an attempt
		//to get the thread
		free(thread_node->next);
		free(thread_node);

		free(thread_stack);

		free(tcb_ptr->joined_val);
		free(tcb_ptr->return_val);
		free(tcb_ptr->thread_stack);
		free(tcb_ptr);
		return -1;
	}

	//define the successor context when this context is done
	//it should be the main context
	thread_context_ptr->uc_link = &main_context;
	thread_context_ptr->uc_stack.ss_sp = thread_stack;
	thread_context_ptr->uc_stack.ss_size = SIGSTKSZ;

	//TODO: other things are needed to complete this function

	thread_node->data = (void*)tcb_ptr;

	//printf("%d\n", ((Queue *) (master_queue->front->data)) == NULL);

	//enqueue to the front node (run node)
	Enqueue((Queue *) (master_queue->front->data), thread_node);

	printf("thread: %d starting thread\n", thread_id);
	curr_thread_id = thread_id;

	makecontext( thread_context_ptr, (void(*) (void))thread_helper, 2, function, arg ) ;

	//we still need a scheduler, and not just swap to the thread_context_ptr
	
	swapcontext(&main_context, thread_context_ptr);

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// switch from thread context to scheduler context

	if ( master_queue == NULL ) return -1;
	if ( &main_context == NULL ) return -1;

	Node * curr_node = NULL;
	tcb * curr_tcb_node = NULL;
	ucontext_t * prev_context = NULL;
	mypthread_t prev_thread_id;

	//assign the current context to the previous context
	//because we will be changing it
	if(getcontext(prev_context) == -1) {
		printf("(132) Error, attempted to get context but failed\n");
		return -1;
	}
	//whichever thread called this is the the
	//thread with curr_thread_id
	prev_thread_id = curr_thread_id;
	curr_node = GetNode(prev_thread_id);
	if ( curr_node == NULL ) {
		printf("(139) Error, could not find the node: %d\n", prev_thread_id);
		return -1;
	}
	curr_tcb_node = (tcb*) curr_node;
	curr_tcb_node->thread_context = (*prev_context);
	//switch curr_thread_id to the main_thread_id
	curr_thread_id = main_thread_id;

	//do we switch to main_context?
	swapcontext(prev_context, &main_context);

	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
	if ( master_queue == NULL ) return;

	Node * curr_node = NULL;
	tcb * curr_tcb_ptr = NULL;
	ucontext_t prev_context;
	mypthread_t prev_thread_id;

	printf("Called\n");
	prev_thread_id = curr_thread_id;

	printf("Called 2 %d\n", prev_thread_id);

	curr_node = GetNode(prev_thread_id);

	printf("Called 3\n");

	if ( curr_node == NULL ) {
		printf("(188) Error, could not find the node: %d\n", prev_thread_id);
		return;
	}

	curr_tcb_ptr = (tcb*) (curr_node->data);
	curr_tcb_ptr->thread_state = Done;
	printf("Called 4\n");
	//assign the main context the value_ptr of this context
	if ( value_ptr != NULL ) main_tcb.joined_val = &value_ptr;	//do we use the joined_val?

	//remove from the queue
	//DequeueNode(master_queue, curr_node);
	mypthread_DequeueNode(curr_node);
	printf("Called 5\n");
	free(curr_tcb_ptr->return_val);
	free(curr_tcb_ptr->joined_val);
	free(curr_tcb_ptr->thread_stack);
	free(curr_tcb_ptr);
	printf("Called 6\n");
	curr_thread_id = main_thread_id;
	printf("thread: %d\n", curr_thread_id);
	//call scheduler?
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

        // wait for a specific thread to terminate
        // de-allocate any dynamic memory created by the joining thread
        // if the value_ptr is not null, the return value of the exiting thread will be passed back
        // ensures that the calling thread will not continue execution until the one it references exits

        tcb* curr_tcb_ptr = NULL;
        Node* curr_node = GetNode(thread);

        if (curr_node == NULL) {
                printf("Error, could not find the thread in queue: %u\n",thread);
                return -1;
        }

        curr_tcb_ptr = (tcb*)(curr_node->data);


        while(curr_tcb_ptr->thread_state != Done){
                mypthread_yield();
        }

        curr_tcb_ptr->return_val = value_ptr;
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
	//sched_stcf();
#else
	//sched_mlfq();
#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	if ( master_queue == NULL ) return;


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

//get a new mypthread_t
mypthread_t FreshThreadID(){
	if ( master_queue == NULL ) return 0;
	if ( isempty(master_queue) ) return 0;

	//start at zero, then find the highest thread_id
	//I don't know how else to do it

	mypthread_t highest_threadID = main_thread_id;
	Node * queue_level_node = master_queue->front;
	Node * thread_node = NULL;
	Queue * inner_queue = NULL;
	tcb * tcb_ptr = NULL;
	while(queue_level_node != NULL){
		if ( queue_level_node->data == NULL) {
			queue_level_node = queue_level_node->next;
			continue;
		}
		inner_queue = (Queue*)(queue_level_node->data);
		thread_node = inner_queue->front;
		while ( thread_node != NULL ) {
			//error checking
			if ( thread_node->data == NULL ) {
				thread_node = thread_node->next;
				continue;
			}
			tcb_ptr = (tcb*) (thread_node->data);

			if (tcb_ptr->thread_id > highest_threadID)
				highest_threadID = tcb_ptr->thread_id;

			thread_node = thread_node->next;
		}
		queue_level_node = queue_level_node->next;
	}
	return highest_threadID + 1;
}

//Get the node associated with the given threadID
Node * GetNode(mypthread_t threadID){
	if (master_queue == NULL) return NULL;
	if (isempty(master_queue)) return NULL;

	Node * thread_node = NULL;
	Queue * inner_queue = FindQueueContainingThreadID(threadID);

	if ( inner_queue == NULL ) return NULL;

	thread_node = inner_queue->front;
	//loop through each thread node
	while ( thread_node != NULL ) {
		//check if thread node has the matching threadID
		if ( thread_node->data != NULL &&
				((tcb *)thread_node->data)->thread_id == threadID) {
			return thread_node;
		}
		thread_node = thread_node->next;
	}
	return NULL;
}

int mypthread_DequeueNode ( Node * node ){
	if (master_queue == NULL) return -1;
	if (isempty(master_queue)) return -1;
	Queue * inner_queue = FindQueueContainingNode(node);
	if (inner_queue == NULL) return -1;
	return DequeueNode(inner_queue, node);
}

Queue * FindQueueContainingNode (Node * node){
	if (master_queue == NULL) return NULL;
	if (isempty(master_queue)) return NULL;
	Node * queue_level_node = master_queue->front;
	Node * thread_node = NULL;
	Queue * inner_queue = NULL;
	//loop through each queue level
	while ( queue_level_node != NULL ){
		if ( queue_level_node->data == NULL) {
			queue_level_node = queue_level_node->next;
			continue;
		}
	//get the first thread node
	inner_queue = (Queue*)(queue_level_node->data);
	thread_node = inner_queue->front;
	//loop through each thread node
		while ( thread_node != NULL ) {
		//check if thread node has the matching threadID
			if ( thread_node == node) {
				return inner_queue;
			}
			thread_node = thread_node->next;
		}
		queue_level_node = queue_level_node->next;
	}
	return NULL;
}
Queue * FindQueueContainingThreadID (mypthread_t threadID){
	if (master_queue == NULL) return NULL;
	if (isempty(master_queue)) return NULL;
	Node * queue_level_node = master_queue->front;
	Node * thread_node = NULL;
	Queue * inner_queue = NULL;
	//loop through each queue level
	while ( queue_level_node != NULL ){
		if ( queue_level_node->data == NULL) {
			queue_level_node = queue_level_node->next;
			continue;
		}
		//get the first thread node
		inner_queue = (Queue*)(queue_level_node->data);
		thread_node = inner_queue->front;
		//loop through each thread node
		while ( thread_node != NULL ) {
			//check if thread node has the matching threadID
			if ( thread_node->data != NULL &&
					((tcb *)thread_node->data)->thread_id == threadID) {
				return inner_queue;
			}
			thread_node = thread_node->next;
		}
		queue_level_node = queue_level_node->next;
	}
	return NULL;
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
Node * Peek(Queue *queue){
	if ( queue == NULL ) return NULL;
	return queue->front;
}
