/* @(#) Queue implemented for a generic pointer type. */

struct pqueue {
	void		**data;
	size_t		max_count;
	size_t		count;
	size_t		head, tail;
	size_t		vhead, vtail;
};


long
pqueue_init(size_t max_count)
{
	/* TODO: */
}


/* __EOF__ */

