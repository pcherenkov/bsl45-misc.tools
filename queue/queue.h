/* @(#) Queue (FIFO) interfaces. */

#ifndef QFIFO_H_20131024
#define QFIFO_H_20131024

#include <sys/types.h>
#include <stdint.h>

#include <stdio.h>

typedef long queue_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a new queue.
 *
 * @param capacity	maximum # of items in the queue.
 *
 * @return handle for the newly-created queue or -1 if error.
 */
queue_t pqueue_create(size_t capacity);


/**
 * Release resources allocated for the queue.
 *
 * @param q		queue to free.
 */
void pqueue_free(queue_t q);


/**
 * Extract the oldest queue item, return -1 if queue is empty.
 *
 * @param q		queue to extract from.
 *
 * @return the oldest data item or (void*)-1 if queue is empty.
 */
void* pqueue_get(queue_t q);


/**
 * Append the queue, return -1 if error 
 *
 * @param q		queue to append.
 * @param val		data item to add.
 *
 * @return 0 if success, non-zero otherwise.
 */
int pqueue_put(queue_t q, void *val);


/* Type-cast helpers:
 */
inline static int
pqueue_iget(queue_t q)
{
	long lval = (long) pqueue_get(q);
	return (int) lval;
}


inline static int
pqueue_iput(queue_t q, int val)
{
	long lval = val;
	return pqueue_put(q, (void*)lval);
}


/**
 * Return non-zero if queue is empty.
 *
 * @param q	queue to test.
 */
int pqueue_is_empty(queue_t q);


/**
 * Return the number of queued elements.
 *
 * @param q	queue to examine.
 *
 * @return number of elements in the queue.
 */
size_t pqueue_count(queue_t q);

/**
 * Output queue metadata and contents.
 *
 * @param q		queue to dump.
 * @param fp		destination file.
 */
void pqueue_dump(queue_t q, FILE *fp);


#ifdef __cplusplus
}
#endif

#endif /* QFIFO_H_20131024 */

/* __EOF__ */

