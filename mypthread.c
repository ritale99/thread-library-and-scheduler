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
const static mypthread_t main_thread_id = 0;
tcb main_tcb;
ucontext_t main_context;
char * main_stack;
uint init = 0;
//scheduler
Schedule_Policy sched;

//current running thread
mypthread_t curr_thread_id;

//private
/* use to initialize main context and queue */
static void mypthread_init(){
	printf("initializing\n");
	master_queue = CreateQueue();

	if ( master_queue == NULL ) exit(1);

	main_tcb.thread_id = main_thread_id;
	main_tcb.thread_state = Running;

	//currently create one level
	Queue * queue_0 = CreateQueue();
	Node * queue_node = (Node *) malloc (sizeof(Node));
	queue_node->data = (void*) queue_0;
	Enqueue(master_queue, queue_node);

	init = 1;
}
static void thread_helper (void * (*function) (void*), void * arg){
	mypthread_exit(function(arg));
}
/* scheduler */
static void schedule() {
	//when this function is called we should assume that the main_thread called it
	if ( main_thread_id != 0 ) {
		printf("(31) here\n");
		return;
	}

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

	//currently we just go to the next ready thread

	Node * inner_queue_node = NULL;
	Node * thread_node = NULL;
	Queue * inner_queue = NULL;

	Queue * some_queue = NULL;

	tcb * tcb_ptr = NULL;
	mypthread_t nextThreadID;
	ucontext_t nextThreadContext;

	uint threadCountAtQueue = 0;
	uint count = 0;

	inner_queue_node = master_queue->front;

	printf("starting schedule\n");

	while(inner_queue_node != NULL){
		inner_queue = (Queue*)(inner_queue_node->data);
		threadCountAtQueue = inner_queue->count;
		if (threadCountAtQueue == count) {
			printf("no nodes\n");
			inner_queue_node = inner_queue_node->next;
			continue;
		}
		thread_node = Dequeue(inner_queue);
		count++;
		if ( thread_node == NULL || thread_node->data == NULL ) {
			printf("(303) Error, attempting to dequeue empty node\n");
			continue;
		}
		Enqueue(inner_queue, thread_node);
		tcb_ptr = (tcb*)(thread_node->data);
		printf("thread: %d\n", tcb_ptr->thread_id);
		some_queue = inner_queue;

		if ( tcb_ptr->thread_state == Running ) {
			printf("it was running\n");
			tcb_ptr = NULL;
			if ( inner_queue->count == 1 ) inner_queue_node = inner_queue_node->next;
			continue;
		}
		break;
	}
	if ( tcb_ptr == NULL ) return;
	//1.save the main context? (not needed)
	//2.set new curr_thread_id
	//3.get new context
	//4.swap context

	printf("prevID: %d\tcurrID: %d\n", curr_thread_id, tcb_ptr->thread_id);
	tcb_ptr->thread_state = Running;
	curr_thread_id = tcb_ptr->thread_id;
	//tcb_ptr->thread_priority = 1;
	//printf("(89) swapped to %d this is it prio %d\n", curr_thread_id, tcb_ptr->thread_priority);

	swapcontext(&(main_tcb.thread_context), &(tcb_ptr->thread_context));

// schedule policy
/*#ifndef MLFQ
	//sched_stcf();
#else
	//sched_mlfq();
#endif*/

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



/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {
	// create Thread Control Block
    // create and initialize the context of this thread
    // allocate space of stack for this thread to run
    // after everything is all set, push this thread int
    // YOUR CODE HERE
	//initialization
	ucontext_t * thread_context_ptr = NULL;
	Node * thread_node = NULL;
	char * thread_stack = NULL;
	tcb * tcb_ptr = NULL;
	mypthread_t thread_id = 1;

	if (!init) mypthread_init();

	thread_id = FreshThreadID();

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
	thread_context_ptr = &(tcb_ptr->thread_context);

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
	thread_context_ptr->uc_link = &(main_tcb.thread_context);
	thread_context_ptr->uc_stack.ss_sp = thread_stack;
	thread_context_ptr->uc_stack.ss_size = SIGSTKSZ;

	thread_node->data = (void*)tcb_ptr;

	//enqueue to the front node (run node)

	Enqueue((Queue *) (master_queue->front->data), thread_node);

	printf("Making context for %d\n", thread_id);	//debug

	makecontext( thread_context_ptr, (void(*) (void))thread_helper, 2, function, arg ) ;

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	if ( master_queue == NULL ) return -1;

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// switch from thread context to scheduler context
	Node * curr_node = NULL;
	tcb * curr_tcb_node = NULL;
	mypthread_t prev_thread_id;

	printf("Thread %d request to yield\n", curr_thread_id);

	if ( curr_thread_id == main_thread_id ) {
		//if its the main thread we should call schedule
		schedule();
	}else {
		curr_node = GetNode(curr_thread_id);
		//URGENT: if their node does not exist, what do we do????
		if ( curr_node == NULL ) {
			printf("(139) Error, could not find the node: %d\n", curr_thread_id);
			return -1;
		}
		curr_tcb_node = (tcb*) (curr_node->data);

		if ( curr_tcb_node == NULL ) {
			printf("(270) Error main tcb is null\n");
		}
		curr_tcb_node->thread_state = Ready;
		//switch curr_thread_id to the main_thread_id
		curr_thread_id = main_thread_id;
		swapcontext(&(curr_tcb_node->thread_context), &(main_tcb.thread_context));
	}
	//idk if we need to call scheduler, probably not

	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	printf("Ending thread %d\n", curr_thread_id);

	Node * curr_node = NULL;
	tcb * curr_tcb_ptr = NULL;

	curr_node = GetNode(curr_thread_id);

	if ( curr_node == NULL ) {
		printf("(188) Error, could not find the node: %d\n", curr_thread_id);
		exit(0);
	}

	curr_tcb_ptr = (tcb*) (curr_node->data);
	curr_tcb_ptr->thread_state = Done;

	//assign the main context the value_ptr of this context
	if ( value_ptr != NULL ) main_tcb.joined_val = &value_ptr;	//do we use the joined_val?

	//remove from the queue
	if ( mypthread_DequeueNode(curr_node) == -1 ) {
		printf("(314) error in attempting to dequeue node\n");	//remove before submission
	}
	//curr_tcb_ptr->thread_id = 0;
	free(curr_tcb_ptr->return_val);
	free(curr_tcb_ptr->joined_val);
	free(curr_tcb_ptr->thread_stack);

	curr_tcb_ptr->return_val = NULL;
	curr_tcb_ptr->joined_val = NULL;
	curr_tcb_ptr->thread_stack = NULL;

	free(((tcb*) (curr_node->data)));

	//&curr_tcb_ptr = NULL;

	curr_node->data = NULL;

	free(curr_node->data);
	free(curr_node);

	//&curr_node = NULL;

	curr_thread_id = main_thread_id;
	//exit(0);
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	 // wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread
	// if the value_ptr is not null, the return value of the exiting thread will be passed back
	// ensures that the calling thread will not continue execution until the one it references exits

	tcb* curr_tcb_ptr = NULL;
	tcb * sample = NULL;
	//get the requested thread's node
	Node* curr_node = GetNode(thread);

	if (curr_node == NULL) {
		printf("Error, could not find the thread in queue: %u\n",thread);
	    return -1;
    }

	//convert the requested node into tcb
	curr_tcb_ptr = (tcb*)(curr_node->data);
	while(curr_node != NULL && curr_node->data != NULL &&
			curr_tcb_ptr->thread_state != Done){
		//PrintMyQueue();
		mypthread_yield();
		printf("still waiting on thread: %d\n", curr_tcb_ptr->thread_id);
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


// Feel free to add any other functions you need

// YOUR CODE HERE

//get a new mypthread_t
mypthread_t FreshThreadID(){
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

/* Print out all the thread that exist within this queue */
void PrintMyQueue(){
	Node * inner_queue_node = NULL;
	Node * thread_node = NULL;
	Queue* inner_queue = NULL;
	tcb * tcb_ptr = NULL;
	int queueLevel = 0;
	inner_queue_node = master_queue->front;
	while(inner_queue_node != NULL){
		inner_queue = (Queue*)inner_queue_node->data;
		thread_node = inner_queue->front;
		if ( inner_queue->count == 0 ) {
			printf("empty queue\n");
		}
		while(thread_node!=NULL){
			if(thread_node->data == NULL) {
				thread_node = thread_node->next;
				continue;
			}
			tcb_ptr = (tcb*) thread_node->data;
			printf("TCB ID: %d, Status: %d, Priority: %d in queue level: %d\n", tcb_ptr->thread_id, tcb_ptr->thread_state, tcb_ptr->thread_priority, queueLevel);
			thread_node = thread_node->next;
		}
		queueLevel++;
		inner_queue_node = inner_queue_node->next;
	}
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
    return (q->count == 0);
}
int DequeueNode( Queue * queue , Node * node){
	if ( node == NULL ) return -1;
	if ( queue == NULL ) return -1;
	Node * ptr = queue->front;
	Node * prevPtr = NULL;
	while ( ptr != NULL ) {
		if ( ptr == node ){
			if (prevPtr == NULL){
				queue->front = NULL;
				queue->rear = NULL;
				queue->count--;
				printf("Called here\n");
				return 1;
			}
			else{
				prevPtr->next = ptr->next;
				ptr->next = NULL;
				queue->count--;
				printf("Called here 2\n");
				return 1;
			}
		}
		prevPtr = ptr;
		ptr = ptr->next;
	}
	return -1;
}
void Enqueue(Queue * queue , Node * node){
	if ( node == NULL ) {
		printf("node is null\n");
		return;
	}
	if ( queue == NULL ) {
		printf("queue is null\n");
		return;
	}
	if ( !isempty(queue) ){
		//printf("insert node %u to queue\n", node);
		queue->rear->next = node;
		queue->rear = node;
	}
	else {
		//printf("double\n");
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
	if (queue->count == 0) queue->rear = NULL;
	return rtn;
}
Node * Peek(Queue *queue){
	if ( queue == NULL ) return NULL;
	return queue->front;
}
