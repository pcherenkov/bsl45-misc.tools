/* @(#) Queue implemented for a generic pointer type. */

#include <sys/types.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <assert.h>

#include "queue.h"


/** Queue metadata. */
struct pqueue {
	/** Dynamic array of pointers (records). */
	void		**data;
	/** Max number of data records. */
	size_t		max_count;
	/** Number of items queued. */
	size_t		count;
	/** Indices of queue's head (first in) and tail (last in). */
	ssize_t		head, tail;
};


/* Create a new pointer queue. */
queue_t
pqueue_create(size_t capacity)
{
	struct pqueue *pq = calloc(1, sizeof(*pq));

	if (NULL == pq) return -1;

	pq->data = calloc(capacity, sizeof(pq->data[0]));
	if (NULL == pq->data) {
		free(pq);
		errno = ENOMEM;
		return -1;
	}

	pq->max_count = capacity;
	pq->count = 0;
	pq->head = pq->tail = -1;

	return (queue_t)pq;
}


/* Release resources allocated for the queue. */
void
pqueue_free(queue_t q)
{
	struct pqueue *pq = (struct pqueue*)q;

	if (0 == q) return;

	if (pq->data)
		free(pq->data);
	free(pq);
}


/* Return non-zero if queue is empty. */
int
pqueue_empty(queue_t q)
{
	struct pqueue *pq = (struct pqueue*)q;
	return 0 == pq->count;
}


/* Extract the oldest queue item, return -1 if queue is empty. */
void*
pqueue_get(queue_t q)
{
	struct pqueue *pq = (struct pqueue*)q;
	void *val = NULL;

	if (0 == pq->count) {
		errno = ERANGE;
		return (void*)-1;
	}

	assert(pq->head >= 0 && pq->head < (ssize_t)pq->max_count);
	val = pq->data[pq->head];
	pq->count--;

	if (0 == pq->count) {
		pq->head = pq->tail = -1;
	}
	else {
		pq->head++;
		if (pq->head >= (ssize_t)pq->max_count)
			pq->head = 0;
	}

	return val;
}


/* Append the queue, return -1 if error. */
int
pqueue_put(queue_t q, void *val)
{
	struct pqueue *pq = (struct pqueue*)q;

	if (pq->count >= pq->max_count) {
		errno = ERANGE;
		return -1;
	}

	pq->count++;
	if (1 == pq->count) {
		pq->head = pq->tail = 0;
	}
	else {
		pq->tail++;
		if (pq->tail >= (ssize_t)pq->max_count)
			pq->tail = 0;
	}

	assert(pq->tail >= 0 && pq->tail < (ssize_t)pq->max_count);
	pq->data[pq->tail] = val;
	return 0;
}


/* Output queue metadata and contents. */
void
pqueue_dump(queue_t q, FILE *fp)
{
	struct pqueue *pq = (struct pqueue*)q;
	ssize_t i = 0;
	size_t n = 0;

	fprintf(fp, "pq(count=%lu/%lu, head=%ld, tail=%ld) {",
		(u_long)pq->count, (u_long)pq->max_count, (long)pq->head, (long)pq->tail);
	for (n = 0, i = pq->head; n < pq->count; ++n, ++i) {
		if (i >= (ssize_t)pq->max_count)
			i = 0;
		fprintf(fp, "%s%p", (n > 0) ? ", " : "", pq->data[i]);
	}
	fprintf(fp, "}\n");
}


/* __EOF__ */

