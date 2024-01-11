// File:	worker_t.h

// List all group member's name:
// username of iLab:
// iLab Server:

#ifndef WORKER_T_H
#define WORKER_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_WORKERS macro */
#define USE_WORKERS 1
#define QUANTUM 10000
//#define DEBUG 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

typedef uint worker_t;

typedef struct TCB {
	/* add important states in a thread control block */
	// thread Id
	worker_t Id;
	// thread status
	// 2=blocked 1=running 0=ready -1=done(exit)
	int status; 
	// thread context	
	ucontext_t* context;
	// thread stack
	// thread priority
	// And more ...
	// YOUR CODE HERE
	// next thread
	struct TCB* next_thread;
	//time in running state
	double elapsed;
	// number of quantums?
	// return value?
	void* return_value;
	//thread to join (used in worker_join)
	struct TCB* thread_joining;
	struct worker_mutex_t* mutex_waiting;
	struct timespec init_enqueue_time;
	struct timespec exit_time;
	int response; //flag for response time (time from enqueue to actually being scheduled for the first time)
} tcb; 

/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */

	// YOUR CODE HERE
	tcb* blocked_list_tail;
	//tcb* blocked_list_head;
	int locked; //1=locked 0=not_locked
	struct TCB* owner;
} worker_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE
typedef struct runqueue {
	uint num_ready;
	tcb* head;
	tcb* tail;
} runqueue;





/* Function Declarations: */

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level worker threads voluntarily */
int worker_yield();

/* terminate a thread */
void worker_exit(void *value_ptr);

/* wait for thread termination */
int worker_join(worker_t thread, void **value_ptr);

/* initial the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex);

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex);

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex);


/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void);




//Aux Functions
void test();
void makescheduler();
tcb* getTCB(worker_t thread);
void remove_cur();
void get_cur();
void sig_timer();
void free_cur_thread();
void requeue();
void dequeue();
void make_scheduler_benchmark_ctx();
void add_to_queue(tcb* TCB);

#ifdef USE_WORKERS
#define pthread_t worker_t
#define pthread_mutex_t worker_mutex_t
#define pthread_create worker_create
#define pthread_exit worker_exit
#define pthread_join worker_join
#define pthread_mutex_init worker_mutex_init
#define pthread_mutex_lock worker_mutex_lock
#define pthread_mutex_unlock worker_mutex_unlock
#define pthread_mutex_destroy worker_mutex_destroy
#endif

#endif
