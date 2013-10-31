/* @(#) Queue implemented for a generic pointer type. */
/*
Copyright (c) 2011-2013 Pavel V. Cherenkov (pcherenkov@gmail.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies, 
either expressed or implied, of the FreeBSD Project.
*/

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


/* Return the number of queued elements. */
size_t
pqueue_count(queue_t q)
{
	struct pqueue *pq = (struct pqueue*)q;
	return pq->count;
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
pqueue_is_empty(queue_t q)
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
		errno = ENOENT;
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
		errno = ENOMEM;
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
		(u_long)pq->count, (u_long)pq->max_count,
		(long)pq->head, (long)pq->tail);
	for (n = 0, i = pq->head; n < pq->count; ++n, ++i) {
		if (i >= (ssize_t)pq->max_count)
			i = 0;
		fprintf(fp, "%s%p", (n > 0) ? ", " : "", pq->data[i]);
	}
	fprintf(fp, "}\n");
}


/* __EOF__ */

