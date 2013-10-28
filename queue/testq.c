/* @(#) FIFO queue unit-test app. */

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

enum {
	MIN_CAP = 2, MAX_CAP = 128,
	MAX_INPUT = 80
};


/* Interactively put/extract queue items. */
int
main(int argc, char *const argv[])
{
	int rc = 0, cap = 0, val = 0;
	queue_t q = 0;
	char input[MAX_INPUT] = "\0";
	FILE *fp = stdin;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s capacity\n", argv[0]);
		return 1;
	}

	cap = atoi(argv[1]);
	if (cap < MIN_CAP || cap > MAX_CAP) {
		fprintf(stderr, "%s: invalid capacity: %s, stay within %d..%d\n",
			argv[0], argv[1], MIN_CAP, MAX_CAP);
		return 1;
	}

	q = pqueue_create((size_t)cap);
	if (-1 == q) {
		perror("pqueue_create");
		return 1;
	}

	for (;;) {
		if (NULL == fgets(input, sizeof(input)-1, fp))
			break;
		if (0 == strcasecmp(input, "get")) {
			errno = 0;
			val = (int) pqueue_get(q);
			if (-1 == val && errno) {
				perror("pqueue_get");
				rc = 1; break;
			}

			printf("Q(%d/%d): %d\n", cap, pqueue_count(q), val);
		}
	}


	pqueue_free(q);
	return rc;
}

/* __EOF__ */

