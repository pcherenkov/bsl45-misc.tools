/* @(#) Queue implemented for a generic pointer type. */

struct pqueue {
	void		**data;
	size_t		max_count;
	size_t		count;
	ssize_t		head, tail;
};


/* Create a new pointer queue. */
long
pqueue_create(size_t max_count)
{
	struct pqueue *pq = calloc(1, sizeof(*pq));

	if (NULL == pq) return -1;

	pq->data = calloc(max_count, sizeof(pq->data[0]));
	if (NULL == pq->data) {
		free(pq);
		errno = ENOMEM;
		return -1;
	}

	pq->max_count = max_count;
	pq->count = 0;
	pq->head = pq->tail = -1;

	return (long)pq;
}


/* Release resources allocated for the queue. */
void
pqueue_free(long q)
{
	struct pqueue *pq = (struct pqueue*)q;

	if (0 == q) return;

	if (pq->data)
		free(pq->data);
	free(pq);
}


/* Return non-zero if queue is empty. */
int
pqueue_empty(long q)
{
	struct pqueue *pq = (struct pqueue*)q;
	return 0 == pq->count;
}


/* Get the next item, return -1. */
void*
pqueue_next(long q)
{
/* TODO: */
}



/* __EOF__ */

