/* @(#) Queue (FIFO) interfaces. */
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

