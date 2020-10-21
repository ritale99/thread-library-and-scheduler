// File:	mypthread.c

// List all group member's name: Rithvik Aleshetty, Minhesota Geusic
// username of iLab:
// iLab Server:

#include "mypthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

//main queue
static Queue * master_queue;

//mutex interrupt
static mypthread_mutex_t* conMutex = NULL;

//main context
const static mypthread_t main_thread_id = 0;
static ucontext_t scheduler_context;
static char * main_stack;
static uint init = 0;
static tcb * main_tcb;
static Node * main_node;

static Schedule_Proceed_State prevClosedState = 0;
//timer
struct sigaction sa;
struct itimerval timer;
struct itimerval pausedTimer;

suseconds_t interruption_time = 2000;
struct timeval delta;
//signal
static struct sigaction scheduler;

static mypthread_t highest_id = 0;

//race condition
static uint in_critical = 0;

//current running thread
static mypthread_t curr_thread_id;
static Node * curr_node;
static tcb * curr_tcb;

//private
static void thread_helper (void * (*function) (void*), void * arg){
	mypthread_exit(function(arg));
}
long GetMicrosSecond(struct timeval start, struct timeval end){
	return ((end.tv_sec- start.tv_sec)*1e6+end.tv_usec)-(start.tv_usec);
}

void deallocContext(tcb * ptr){
	if(ptr == NULL) return;
	free(ptr->thread_stack);
	ptr->thread_stack = NULL;
}
void deallocTCB (Node* ptr){
	if (ptr == NULL) return;
	free(ptr->data);
	free(ptr);
}
void stcf_adjust_curr_priority(){
	struct timeval delta;
	gettimeofday (&delta, 0);

	//curr_tcb->thread_priority += (double)((((delta.tv_sec - curr_tcb->last_recorded_time.tv_sec)*1e6) + delta.tv_usec) - (curr_tcb->last_recorded_time.tv_usec));
	curr_tcb->thread_priority += GetMicrosSecond(curr_tcb->last_recorded_time, delta) / interruption_time;
}
/* schedule by stcf but caused by interruption */
static void sched_stcf_by_interrupt(){

	stcf_adjust_curr_priority();

	Node * prev_node = NULL;
	Node * ptr_node = NULL;
	Node * next_node = NULL;

	tcb * prev_tcb_ptr = NULL;
	tcb * tcb_ptr = NULL;
	tcb * next_tcb_ptr = NULL;

	prev_node = curr_node;
	prev_tcb_ptr = curr_tcb;

	prev_tcb_ptr->thread_state = Scheduled;
	//if empty or the front node has lower priority than current we just enqueue it
	if ( master_queue->count == 0 ){
		//printf("is rear empty? %d\n", queue->rear == NULL);
		Enqueue(master_queue, prev_node);
	}else{
		ptr_node = master_queue->front;
		Node * prev = NULL;
		//sort the prev node based on priority (ascending)
		while(ptr_node != NULL){
			tcb_ptr = (tcb *) (ptr_node->data);
			if ( tcb_ptr->thread_priority >= prev_tcb_ptr->thread_priority ) {
				//curr_node->next = ptr_node->next;
				//ptr_node->next = curr_node->next;
				if ( master_queue->front == ptr_node ){
					master_queue->front = prev_node;
				}
				if ( prev != NULL ) {
					prev->next = prev_node;
				}
				prev_node->next = ptr_node;
				master_queue->count++;
				return;
			}
			prev = ptr_node;
			ptr_node = ptr_node->next;
		}
		if ( prev != NULL ) {
			Enqueue(master_queue, prev_node);
		}
	}
	//Enqueue(queue, prev_node);
}
/* schedule by stcf but caused by mutext */
static void sched_stcf_by_mutex(){
	//adjust by current time
	stcf_adjust_curr_priority();

	mypthread_mutex_t* mutex = conMutex;
	
	//point the next to the list of blocked threads
	//waking of these waiting threads
	//should we use master_queue instead?
	
//	Node * node = GetNode(curr_thread_id);
	curr_node->next = mutex->list;
	mutex->list = curr_node;

	/*
	curr_tcb->next = mutex->list
	mutex->list = currentItem; 
	*/
	return;
}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	//determine which sched_stcf we should use based on the previous closed state
	//ie if the previous thread invoke yield, prevClosedState = Yielded
	//and scheduler would subsequently use sched_stcf_by_interrupt()
	if (prevClosedState == TimerInterrupt ||
			prevClosedState == Yielded ||
			prevClosedState == Joined ){
		sched_stcf_by_interrupt();
		if ( prevClosedState == TimerInterrupt ) curr_tcb->thread_state = Scheduled;
		else if ( prevClosedState == Yielded ) curr_tcb->thread_state = Ready;
		else curr_tcb->thread_state = Blocked;
	}else if (prevClosedState == Mutexed){
		sched_stcf_by_mutex();
		curr_tcb->thread_state = Blocked;
	}else{
		deallocContext(curr_tcb);
		curr_tcb->thread_state = Done;
	}
	//we finish the schedule by finish state
	prevClosedState = Finished;

	//prepare the next node
	Node * node = Dequeue(master_queue);

	if ( node == NULL ) return;

	curr_tcb = (tcb*)(node->data);

	//let the current node be the next node
	curr_node = node;
	curr_thread_id = curr_tcb->thread_id;

	//set next thread to be in running state
	curr_tcb->thread_state = Running;

	//record the time when the next node start
	gettimeofday(&(curr_tcb->last_recorded_time), 0);

	//start timer
	setitimer(ITIMER_PROF, &timer, NULL);

	//start running next thread
	setcontext(&(curr_tcb->thread_context));
}
/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)
	// YOUR CODE HERE
}
/* scheduler */
static void schedule() {
	setitimer(ITIMER_PROF, &pausedTimer, NULL);
	//schedule policy
	#ifndef MLFQ
		sched_stcf();
	#else
		sched_mlfq();
	#endif
}

/* Handle timer interrupt for scheduler */
void Timer_Interrupt_Handler(){
	//the previous state that call the scheduler did it
	//from the timer interrupt
	prevClosedState = TimerInterrupt;
	swapcontext(&(curr_tcb->thread_context), &scheduler_context);
}

/* use to initialize main context and queue */
void mypthread_init(){
	//create queue
	master_queue = CreateQueue();

	if ( master_queue == NULL ) exit(1);

	//set up some stuff for the main thread
	char * scheduler_stack = (char *) malloc(SIGSTKSZ);
	main_tcb = (tcb*) malloc(sizeof(tcb));
	main_node = (Node *) malloc(sizeof(Node));

	//get context for scheduler
	getcontext(&scheduler_context);

	//set stack for scheduler
	scheduler_context.uc_stack.ss_sp = scheduler_stack;
	scheduler_context.uc_stack.ss_size = SIGSTKSZ;

	//set context's running fucntion to the schedule()
	makecontext(&scheduler_context, &schedule, 0);

	//setup main_tcb
	main_tcb->thread_id = main_thread_id;
	main_tcb->thread_priority = 0;
	main_tcb->thread_state = Scheduled;

	//get context for main_tcb
	getcontext(&(main_tcb->thread_context));
	main_tcb->thread_stack = (main_tcb->thread_context).uc_stack.ss_sp;
	main_tcb->thread_context.uc_link = NULL;

	//let the current tcb be the main tcb
	curr_tcb = main_tcb;
	curr_thread_id = main_tcb->thread_id;

	main_node->data = main_tcb;
	curr_node = main_node;

	//create timer interrupt
	memset(&scheduler, 0, sizeof(scheduler));
	scheduler.sa_handler = &Timer_Interrupt_Handler;
	sigaction(SIGPROF,&scheduler,NULL);

	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = interruption_time;
	timer.it_interval = timer.it_value;

	//setup pause timer
	pausedTimer.it_value.tv_sec = 0;
	pausedTimer.it_value.tv_usec = 0;

	init = 1;
	//start timer
	setitimer(ITIMER_PROF,&timer,NULL);
}

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr,
                      void *(*function)(void*), void * arg) {
	// create Thread Control Block
    // create and initialize the context of this thread
    // allocate space of stack for this thread to run
    // after everything is all set, push this thread int
    // YOUR CODE HERE

	if (!init) mypthread_init();

	//initialization
	ucontext_t * thread_context_ptr = NULL;
	Node * thread_node = NULL;
	char * thread_stack = NULL;
	tcb * tcb_ptr = NULL;
	mypthread_t thread_id = 1;

	//retreive a fresh id
	thread_id = FreshThreadID();

	//allocate the stack based on the SIGSTKSZ size
	thread_stack = (char *)malloc(SIGSTKSZ);
	if ( thread_stack == NULL ) return -1;

	//allocate space for tcb
	tcb_ptr = (tcb*) malloc(sizeof(tcb));
	if ( tcb_ptr == NULL ){
		free(thread_stack);
		return 0;
	}

	//allocate space for the thread_node
	thread_node = (Node *) malloc (sizeof(Node));
	if ( thread_node == NULL ) {
		free(tcb_ptr);
		free(thread_stack);
		return 0;
	}

	//assign thread_state to ready state
	tcb_ptr->thread_state = Scheduled;

	//assign thread_id
	tcb_ptr->thread_id = thread_id;

	//assign thread_stack to tcb
	tcb_ptr->thread_stack = thread_stack;

	//set new thread's priority as the highest (0)
	tcb_ptr->thread_priority = 0;

	if ( thread != NULL ) (*thread) = thread_id;

	//assign thread context
	thread_context_ptr = &(tcb_ptr->thread_context);

	//assign current context to thread_context_ptr
	if ( getcontext(thread_context_ptr) == -1 ) {
		//we have received an error in an attempt
		//to get the thread, free everything and end critical section
		free(thread_node->next);
		free(thread_node);
		free(thread_stack);
		free(tcb_ptr->joined_val);
		free(tcb_ptr->return_val);
		free(tcb_ptr->thread_stack);
		free(tcb_ptr);
		in_critical = 0;
		return 0;
	}

	//define the successor context when this context is done
	//it should be the scheduler_context
	thread_context_ptr->uc_link = &scheduler_context;
	thread_context_ptr->uc_stack.ss_sp = thread_stack;
	thread_context_ptr->uc_stack.ss_size = SIGSTKSZ;

	thread_node->data = tcb_ptr;

	//enqueue
	Enqueue(master_queue, thread_node);

	//printf("Making context for %d\n", thread_id);	//debug

	makecontext( thread_context_ptr, (void(*) (void))thread_helper, 2, function, arg ) ;

	PrintMyQueue();

    return 1;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {

	setitimer (ITIMER_PROF, &pausedTimer, NULL);

	//printf("\nThread %d requested to yield\n", curr_thread_id);	//debug

	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// switch from thread context to scheduler context
	prevClosedState = Yielded;
	//schedule();
	//swap to scheduler
	swapcontext(&(curr_tcb->thread_context), &scheduler_context);

	//printf("Thread %d return from yielding\n", curr_thread_id);
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread
	//stop timer
	setitimer (ITIMER_PROF, &pausedTimer, NULL);

	//printf("Ending thread %d\n", curr_thread_id);
	//the current thread that called scheduler context did it
	//from exit or Finished
	prevClosedState = Finished;

	//set up return val
	curr_tcb->return_val = value_ptr;
	if ( curr_tcb->joined_val != NULL )
		*(curr_tcb->joined_val) = value_ptr;

	setcontext(&scheduler_context);
}

/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate
	// de-allocate any dynamic memory created by the joining thread
	// if the value_ptr is not null, the return value of the exiting thread will be passed back
	// ensures that the calling thread will not continue execution until the one it references exits
	printf("attempting to join with %d\n", thread);
	tcb* requested_tcb = NULL;
	//get the requested thread's node
	Node* requested_node = GetNode(thread);

	if (requested_node == NULL) {
		printf("Error, could not find the thread in queue: %d\n",thread);
		//PrintMyQueue();
	    //if ( thread == 4 ) exit(0);
	    return 0;
    }

	requested_tcb = (tcb*)(requested_node->data);

	if ( value_ptr != NULL ) requested_tcb->joined_val = value_ptr;
	if (requested_tcb->thread_state != Done){
		//pause timer
		setitimer(ITIMER_PROF, &pausedTimer, NULL);

		//save current position so we can come back to this
		//position if thread is not done
		getcontext(&(curr_tcb->thread_context));

		//goto scheduler if the requested thread is not done
		if(requested_tcb->thread_state != Done){
			prevClosedState = Joined;
			setcontext(&scheduler_context);
		}
	}
	//remove requested node
	deallocTCB(requested_node);
	return 1;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
	//we may ignore mutexattr

	//check if it isn't initialized
	if(mutex == NULL) {printf("Error: mutex does not exist"); return -1;}

	//0 means available and and 1 means locked
	mutex->available = 0;

	//mutex->thread = curr_thread_id;
	
	mutex->thread = -1;

	//initialize data structures for this mutex
	//each mutex carries its blocked list of threads
	//mutex->list = CreateQueue();
	return 0;
};
/* aquire the mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        // use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //
        // context switch to the scheduler thread
	//printf("attempting lock");
	//test and set returns prior value and punches in 1
	if(__atomic_test_and_set(&(mutex->available),1)==1){
		//if we reached here it means acquiring mutex failed

		//pause timer
		setitimer (ITIMER_PROF, &pausedTimer, NULL);

		//this occurs when the current thread is trying to lock its own already locked mutex
		if(mutex->thread == curr_thread_id){
	//	printf("(556)Error: Cannot lock when already locked by current thread");
		}

		prevClosedState = Mutexed;
		conMutex = mutex; 

		//current thread enters block list of the current mutex
		//CHANGE: create new node first and then push to list?
		//Enqueue(mutex->list, GetNode(curr_thread_id));

		//switch to scheduler thread by swapping
		swapcontext(&(curr_tcb->thread_context), &scheduler_context); 

	}

	//Ensure the mutex thread is set to the current thread
	mutex->thread = curr_thread_id;

	//Continue to enter the critical section
	return 1;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// Release mutex and make it available again.
	// Put threads in block list to run queue so that they could compete for mutex later.
	//printf("attempting unlock");
	//make sure that we aren't unlocking the mutex from another thread
	if(mutex->thread != curr_thread_id){printf("(587)Error: Cannot unlock a mutex from a different thread"); exit(0);}

	//release the lock
	mutex->available = 0;
	mutex->thread = -1; 

	//here we do our restoration of blocks list of threads
	//Node * ptr = mutex->list->front;
	//Node * next;
	
	while(mutex->list != NULL){
		//remove each node from our blocked list
		Node* item  = mutex->list;
		mutex->list = mutex->list->next;
		
		Enqueue(master_queue, item);
		printf("Did an enqueue");

	
	}
//	printf("Finished Unlocking");

	//continue execution of the current thread
	return 1;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in mypthread_mutex_init
	//free(mutex->list);
	return 0;
}
// Feel free to add any other functions you need

// YOUR CODE HERE

//get a new mypthread_t
mypthread_t FreshThreadID(){
	mypthread_t highest_threadID = highest_id;
	highest_id++;
	return highest_threadID + 1;
}

/* Print out all the thread that exist within this queue */
void PrintMyQueue(){
	Node * thread_node = NULL;
	tcb * tcb_ptr = NULL;
	printf("Queue length: %d\n", master_queue->count);
	thread_node = master_queue->front;
	while(thread_node!=NULL){
		if(thread_node->data == NULL) {
			thread_node = thread_node->next;
			continue;
		}
		tcb_ptr = (tcb*) thread_node->data;
		printf("TCB %u\tID: %d, Status: %d, Priority: %d\n", thread_node,
				tcb_ptr->thread_id,
				tcb_ptr->thread_state,
				tcb_ptr->thread_priority);
		thread_node = thread_node->next;
	}

}

//Get the node associated with the given threadID
Node * GetNode(mypthread_t threadID){
	if (master_queue == NULL) return NULL;
	if (isempty(master_queue)) return NULL;
	Node * thread_node = NULL;
	thread_node = master_queue->front;
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
//Generic Queue
/* Create an empty queue */
Queue * CreateQueue(){
	Queue * queue = (Queue *) (malloc(sizeof(Queue)));
	if ( queue == NULL ) return NULL;
	queue->count = 0;
	queue->front = NULL;
	queue->rear = NULL;
	return queue;
}
/* test if queue is empty */
int isempty(Queue *q) {
    return (q->count == 0);
}
/* remove a given node from the given queue */
int DequeueNode( Queue * queue , Node * node){
	if ( node == NULL ) return -1;
	if ( queue == NULL ) return -1;
	Node * ptr = queue->front;
	Node * prevPtr = NULL;
	while ( ptr != NULL ) {
		if ( ptr == node ){
			queue->count--;
			if (prevPtr == NULL){
				queue->front = ptr->next;
				if(queue->rear == ptr) queue->rear = prevPtr;
				printf("Called here\n");
				return 1;
			}
			else{
				prevPtr->next = ptr->next;
				ptr->next = NULL;
				//set rear to the previous node
				//if the removed node is the rear node
				if ( queue->rear == ptr ) queue->rear = prevPtr;
				printf("Called here 2\n");
				return 1;
			}
		}
		prevPtr = ptr;
		ptr = ptr->next;
	}
	return -1;
}
/* enqueue a node to a given queue */
void Enqueue(Queue * queue , Node * node){
	if ( node == NULL ) {
		printf("node is null\n");
		return;
	}
	if ( queue == NULL ) {
		printf("queue is null\n");
		return;
	}
	if ( queue->count != 0 ){
		queue->rear->next = node;
		queue->rear = node;
		//PrintMyQueue();
	}
	else {
		queue->front = queue->rear = node;
	}
	queue->count++;
}
/* remove the front node from the given queue */
Node * Dequeue(Queue * queue){
	if ( queue == NULL || queue->count == 0 ) return NULL;
	Node * rtn;
	rtn = queue->front;
	queue->front = queue->front->next;
	rtn->next = NULL;
	queue->count--;
	if (queue->count == 0) queue->rear = NULL;
	return rtn;
}
/* look at front node of given queue */
Node * Peek(Queue *queue){
	if ( queue == NULL ) return NULL;
	return queue->front;
}
