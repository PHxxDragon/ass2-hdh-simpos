#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

/*
	UNDEFINED BEHAVIOR: if called when *q is already full
	TODO: put a new process to queue [q]
*/
void enqueue(struct queue_t * q, struct pcb_t * proc)
{
	int index;
	
	/// STANDARD ALGORITHM: insert into array-based max heap
	// the new process's index
	index = q->size++;
	
	// insert from bottom
	q->proc[index] = proc;
	
	// correct heap
	while (index != 0) // node at root -> stop
	  {
		// the only maybe wrongly-placed node is placed right -> stop
		if (q->proc[index]->priority <= q->proc[(index - 1) >> 1]->priority)
			break;
		
		// sieve the wrongly-placed node up
		q->proc[index] = q->proc[(index - 1) >> 1];
		index = (index - 1) >> 1;
		q->proc[index] = proc;
	  }
}

/*
	UNDEFINED BEHAVIOR: if called when *q is empty
	TODO: return a pcb whose priority is the highestin the queue [q] and remember to remove it from q
*/
struct pcb_t * dequeue(struct queue_t * q)
{
	struct pcb_t * proc;
	int index;
	int higherChild;
	
	// let the return pcb stay right after the queue at [q->proc[q->size]];
	proc = q->proc[0];
	q->proc[0] = q->proc[--q->size];
	q->proc[q->size] = proc;
	
	/// STANDARD ALGORITHM: remove root of max heap
	// index of the process disrupting heap order
	index = 0;
	
	// correct heap
	while ((index << 1) + 1 < q->size) // node is leaf -> stop
	  {
		higherChild = (index << 1) + 1; // left child's index
		
		// if right child exists AND has higher priority than left child,
		// then [higherChild] is right child's index
		if (higherChild + 1 < q->size
			&& q->proc[higherChild]->priority < q->proc[higherChild + 1]->priority)
			++higherChild; // right child's index
			
		// if node [higherChild] has lower priority,
		// then the only maybe wrongly-placed node is placed right -> stop
		if (q->proc[higherChild]->priority < q->proc[index]->priority)
			break;
		
		// swap node and node [higherChild]
		proc = q->proc[higherChild];
		q->proc[higherChild] = q->proc[index];
		q->proc[index] = proc;
		index = higherChild;
	  }
	
	// return
	return q->proc[q->size];
}

