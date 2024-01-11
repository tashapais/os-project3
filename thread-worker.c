// File:	thread-worker.c

// List all group member's name:
// username of iLab: 
// iLab Server: iLab1.cs.rutgers.edu

#include "thread-worker.h"
#include <ucontext.h>

//not default
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

//Global counter for total context switches and 
//average turn around and response time
long tot_cntx_switches=0;
double avg_turn_time=0;
double avg_resp_time=0;


// INITAILIZE ALL YOUR OTHER VARIABLES HERE
// YOUR CODE HERE
int init = 0;
ucontext_t sched_ctx, main_ctx, cur_ctx;
tcb *main_tcb,  *cur_tcb = NULL;
runqueue** runq;
struct sigaction sa;
struct itimerval timer;
static long Id_counter = 2;
long complete_counter = 0;
static void sched_psjf();
//struct timespec start;
//struct timespec end;

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {

	if (!init) make_scheduler_benchmark_ctx();
       // - create Thread Control Block (TCB)
	tcb* TCB = (tcb*) malloc(sizeof(tcb));
        //create and initialize the context of this worker thread
	ucontext_t* uctx = malloc(sizeof(uctx));
        getcontext(uctx);
        uctx->uc_stack.ss_sp= malloc(8192);  //remember to free TCB->context->uc_stack.ss_sp
        uctx->uc_stack.ss_size = 8192;
        uctx->uc_link = &sched_ctx;
        makecontext(uctx, (void (*)(void))function, 1, arg);	
   
       	TCB->context = uctx;
	TCB->Id = Id_counter;
	*thread = Id_counter;
	TCB->status = 1;
	TCB->elapsed = 0;
	TCB->next_thread = NULL;
	TCB->thread_joining = NULL;
	TCB->mutex_waiting = NULL;
	TCB->response = 0;
	Id_counter += 1;


	if (init==1)//final stage of init where we add main tcb to runq and switch to sched
	{
		init = 2;
		cur_tcb = TCB;
		main_tcb = (tcb*) malloc(sizeof(tcb));
		getcontext(&main_ctx);
		main_tcb->next_thread = NULL;
		main_tcb->thread_joining = NULL;
		main_tcb->context = &main_ctx;
		main_tcb->context->uc_link = &sched_ctx;
		main_tcb->status = 1;
		main_tcb->Id = 1;
		cur_tcb = main_tcb;
		swapcontext(main_tcb->context, &sched_ctx);
	}
	add_to_queue(TCB);
	clock_gettime(CLOCK_REALTIME, &(TCB->init_enqueue_time));

    return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield(){
	
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	cur_tcb->status = 0;
#ifdef DEBUG
	printf("thread %d status 0 for yeild\n", cur_tcb->Id);
#endif
	swapcontext(cur_tcb->context, &sched_ctx);
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread
	// YOUR CODE HERE		
	cur_tcb->status = -1; //done(exit)
	cur_tcb->return_value = value_ptr;
	setcontext(&sched_ctx);
};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
	tcb* worker_thread = getTCB(thread);	 //get worker_t to join
	if (worker_thread == NULL) return 0; //worker_thread already exited
	worker_thread->thread_joining = cur_tcb;
	worker_yield();
	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex
	mutex->blocked_list_tail = NULL;
	mutex->locked = 0;
	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {

        // - use the built-in test-and-set atomic function to test the mutex
        // - if the mutex is acquired successfully, enter the critical section
        // - if acquiring mutex fails, push current thread into block list and
        // context switch to the scheduler thread
	if (mutex->locked == 0)
	{
		if(mutex->locked == 1) puts("somethingwrong");
		mutex->locked = 1;
#ifdef DEBUG 
		printf("thread %d using mutex\n", cur_tcb->Id); 
#endif

	}
	else
	{
		cur_tcb->status = 0;
		cur_tcb->mutex_waiting = mutex;		
#ifdef DEBUG 
		printf("thread %d waiting for mutex\n", cur_tcb->Id); 
#endif
		worker_yield();
		//cur_tcb->next_thread = mutex->blocked_list_tail;
		//mutex->blocked_list_tail = cur_tcb;

	}
        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	tcb* c = runq[0]->tail;
	while (c!=NULL)
	{
		if (c->mutex_waiting == mutex)
		{
			c->mutex_waiting = NULL;
			c->status = 1;
#ifdef DEBUG
			printf("thread %d unlocked mutex for thread %d\n", cur_tcb->Id,c->Id);
#endif
		}
		c = c->next_thread;
	}
	mutex->locked = 0;
#ifdef DEBUG
	printf("thread %d unlocked mutex for all threads\n", cur_tcb->Id);
#endif	




	return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function

	// - invoke scheduling algorithms according to the policy (PSJF or MLFQ)

	// YOUR CODE HERE
	// Set the timer up (start the timer)

	timer.it_value.tv_usec = QUANTUM; //I think this is 200ms, messing with this value makes or breaks parallel cal and vector multiply :(
	timer.it_value.tv_sec = 0;
	timer.it_interval.tv_usec = 0; 
	timer.it_interval.tv_sec = 0;
	
	if (init == 2)
	{
		add_to_queue(cur_tcb);
		dequeue();
		init = 3;
	}
	else
	{	
		//moving this getcontext() line, down 5 lines, took me awhile to fix. Was saving context to wrong thread.
		//getcontext(cur_tcb->context);	
		tcb* cur = cur_tcb;
		remove_cur();
		get_cur();
		add_to_queue(cur);		
		getcontext(cur_tcb->context);	
				
	}

	// Set up what the timer should reset to after the timer goes off
	
//	timer.it_value.tv_usec = QUANTUM; //I think this is 200ms, messing with this value makes or breaks parallel cal and vector multiply :(
//	timer.it_value.tv_sec = 0;
//	timer.it_interval.tv_usec = 0; 
//	timer.it_interval.tv_sec = 0;
	// Set up the current timer to go off in 1 second
	// Note: if both of the following values are zero
	//       the timer will not be active, and the timer
	//       will never go off even if you set the interval value

// - schedule policy
#ifndef MLFQ
	// Choose PSJF
	sched_psjf();
#else 
	// Choose MLFQ
	sched_mlfq();
#endif
	//remember to free stuff, sched stk
}

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf() {
	// - your own implementation of PSJF
	// (feel free to modify arguments and return types)
	
	// YOUR CODE HERE
	while (cur_tcb != NULL)
	{
		if (cur_tcb->status == 0)
		{	
#ifdef DEBUG
			printf("status of thread %d is  0 moving on to next thread\n", cur_tcb->Id);
#endif
			
			tcb* cur = cur_tcb;
			remove_cur();
			get_cur();
			add_to_queue(cur);
			continue;
		}
		cur_tcb->status = 1;
#ifdef DEBUG
		printf("context switching to thread %d with status %d current q = ",cur_tcb->Id, cur_tcb->status);

		tcb* t = runq[0]->tail;
		while (t!=NULL)
		{
			printf("%d ",t->Id);
			t=t->next_thread;
		}
		printf("\n");
#endif


		//in sched_ctx, calculate stats before main thread exits
		if (cur_tcb->Id == 1 && cur_tcb->next_thread == NULL)
		{
			avg_turn_time = avg_turn_time/(Id_counter-2);
			avg_resp_time = avg_resp_time/(Id_counter-2);
		}

		if (!cur_tcb->response && cur_tcb->Id != 1)
		{
			cur_tcb->response = 1;
			struct timespec start = cur_tcb->init_enqueue_time;
			struct timespec response_time;
			clock_gettime(CLOCK_REALTIME, &response_time);
			avg_resp_time += (response_time.tv_sec - start.tv_sec) * 1000 + (response_time.tv_nsec - start.tv_nsec) / 1000000;
		}
		tot_cntx_switches++;
		setitimer(ITIMER_PROF, &timer, NULL);

		




		if (swapcontext(&sched_ctx, cur_tcb->context) == 0 )
		{
			//once back in sched ctx turn timer off
			timer.it_value.tv_usec = 0;
			timer.it_value.tv_sec = 0;
			setitimer(ITIMER_PROF, &timer, NULL);

			//check to see if thread actually finished
			if (cur_tcb->status == 1 ||cur_tcb->status == -1)
			{
				clock_gettime(CLOCK_REALTIME, &(cur_tcb->exit_time));
				struct timespec start = cur_tcb->init_enqueue_time;
				struct timespec end = cur_tcb->exit_time;
				avg_turn_time += (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;

				//i think i can get rid of this
				cur_tcb->status = -1;
				if (cur_tcb->thread_joining != NULL)
				{
					cur_tcb->thread_joining->status = 1;
				}
#ifdef DEBUG
				printf("thread %d exited current q = ",cur_tcb->Id);
#endif

				
				tcb* next_cur = runq[0]->tail;
				while (next_cur != NULL && next_cur->next_thread !=cur_tcb)
				{
#ifdef DEBUG
					printf("%d ",next_cur->Id);
#endif
					next_cur = next_cur->next_thread;
				}	
			        free(cur_tcb->context->uc_stack.ss_sp);
				free(cur_tcb->context);
       	        	 	free(cur_tcb);
				cur_tcb = next_cur;
				cur_tcb->next_thread = NULL;
#ifdef DEBUG
				printf("next cur should be %d\n",cur_tcb->Id);
#endif
			}
					
		}
		else
		{
			tcb* cur = cur_tcb;
			remove_cur();
			get_cur();
			add_to_queue(cur);
		}
	}
}


/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

//DO NOT MODIFY THIS FUNCTION
/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void) {

       fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
       fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
       fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}


// Feel free to add any other functions you need

// YOUR CODE HERE

//this method runs once, during the first call to worker_create()
void make_scheduler_benchmark_ctx()
{	
	init = 1; // scheduler ctx will be initialized after the first and only run of this method
	//make runq
#ifndef MLFQ
	// Choose PSJF
	runq = malloc(sizeof(runqueue*));
	runq[0] = malloc(sizeof(runqueue));
	runq[0]->head = NULL;
	runq[0]->tail = NULL;
#else 
	// Choose MLFQ
#endif
	
	//make signal handler	
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &schedule;
	sigaction (SIGPROF, &sa, NULL);
	//make timer	
	timer.it_interval.tv_usec = 0; 
	timer.it_interval.tv_sec = 0;

	//make scheduler ctx
        getcontext(&sched_ctx);
        sched_ctx.uc_stack.ss_sp= malloc(SIGSTKSZ);
        sched_ctx.uc_stack.ss_size = SIGSTKSZ;
        sched_ctx.uc_link = NULL; 
        makecontext(&sched_ctx, schedule, 0);	
}

void add_to_queue(tcb* thread)
{

	if (runq[0]->tail == NULL)
	{
		runq[0]->tail = thread;
		thread->next_thread = NULL;
	}
	else
	{
		thread->next_thread = runq[0]->tail;
		runq[0]->tail = thread;
	}
	runq[0]->num_ready++;
#ifdef DEBUG
	printf("added thread %d to q funcaddr %p\n", thread->Id, thread->context->uc_stack.ss_sp);
#endif

	return;
}

tcb* getTCB(worker_t thread)
{
	tcb* cur = runq[0]->tail;
	tcb* prev = NULL;
#ifdef DEBUG
	printf("trying to get thread %d\n", thread);
#endif
	while(cur != NULL)
	{

#ifdef DEBUG
		printf("have thread %d\n",cur->Id);
#endif


		if (cur->Id == thread) return cur;
		prev = cur;
		cur = cur->next_thread;
	}
	return NULL;
}

void dequeue()
{
	tcb* cur = runq[0]->tail;
	tcb* prev = cur;
	while(cur != NULL)
	{
		prev = cur;
		cur = cur->next_thread;
	}

	if (cur != NULL)
	{
		prev->next_thread = NULL;
		cur_tcb = prev;
#ifdef DEBUG
		printf("dequeueing thread %d, should be the next context switch\n", cur_tcb->Id);
#endif
	}

}


void remove_cur()
{
	tcb* c = runq[0]->tail;
	tcb* p = NULL;
	while (c != NULL && c != cur_tcb)
	{
		p = c;
		c = c->next_thread;
	}
	if(p!=NULL)
	{	
		p->next_thread = NULL;
#ifdef DEBUG
		printf("removed thread %d from q\n",p->Id);
#endif
	}

}

void get_cur()
{	
	tcb* c = runq[0]->tail;
	tcb* p = NULL;
	while (c != NULL)
	{
		p = c;
		c = c->next_thread;

	}
	cur_tcb = p;
#ifdef DEBUG
	printf("new cur is thread %d with status %d\n", cur_tcb->Id,cur_tcb->status);
#endif
}


