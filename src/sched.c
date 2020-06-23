#include "queue.h"
#include "sched.h"
#include <pthread.h>

static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

int queue_empty(void) {
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
	ready_queue.size = 0;
	run_queue.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

/*
	TODO: get a process from [ready_queue]
*/
struct pcb_t * get_proc(void) {
	struct pcb_t * proc;
	int index;
	/*TODO: get a process from [ready_queue]. return the highest priority one.
	 * Remember to use lock to protect the queue.
	 * */
	
	pthread_mutex_lock(&queue_lock);
	// if [ready queue] is empty,
	// then push all processes in [run_queue] back to [ready_queue]
	if (empty(&ready_queue)) {
		// SOLUTION 1: intuitive
		while (!empty(&run_queue))
			enqueue(&ready_queue, dequeue(&run_queue));
		
		// SOLUTION 2: time-efficient
		/*
		for (index = 0; index < run_queue.size; ++index)
			ready_queue.proc[index] = run_queue.proc[index];
		
		ready_queue.size = run_queue.size;
		run_queue.size = 0;
		*/
	}
	
	// if [ready_queue] is still empty,
	// then return NULL,
	// else return highest priority one
	if (empty(&ready_queue))
		proc = NULL;
	else
		proc = dequeue(&ready_queue);
	
	pthread_mutex_unlock(&queue_lock);
	
	return proc;
}

void put_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}


