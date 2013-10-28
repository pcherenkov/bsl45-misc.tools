/* @(#) FIFO queue unit-test app. */

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "queue.h"

enum {
	MIN_CAP = 2, MAX_CAP = 128,
	MAX_INPUT = 80
};


/* Interactively put/extract queue items. */
int
main(int argc, char *const argv[])
{
	int rc = 0, cap = 0;
	long lval = 0;
	queue_t q = 0;
	char input[MAX_INPUT] = "\0", *p = NULL;
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

	setvbuf(stdout, (char*)NULL, _IOLBF, 0);

	printf("Q(%d/%d) ready\n", cap, (int)pqueue_count(q));
	for (;;) {
		if (NULL == fgets(input, sizeof(input)-1, fp))
			break;
		if (NULL != (p = strrchr(input, '\n')))
			*p = 0;
		if (NULL != (p = strrchr(input, '\r')))
			*p = 0;

		if (0 == strcasecmp(input, "get")) {
			errno = 0;
			lval = (long) pqueue_get(q);
			if (-1 == lval && errno) {
				perror("pqueue_get");
				continue;
			}

			printf("Q(%d/%d) >> %ld\n", cap, (int)pqueue_count(q), lval);
		}
		else if (0 == strcasecmp("dump", input) || 0 == strcasecmp("list", input)) {
			pqueue_dump(q, stdout);
		}
		else {
			lval = atol(input);
			if (0 == lval && strcmp("0", input)) {
				printf("Invalid input, try again\n");
				continue;
			}

			rc = pqueue_put(q, (void*)lval);
			if (rc) {
				perror("pqueue_put");
				continue;
			}

			printf("Q(%d/%d) << %ld\n", cap, (int)pqueue_count(q), lval);
		}
	}
	pqueue_free(q);

	printf("%s: exiting with rc=%d\n", argv[0], rc);
	return rc;
}

/* __EOF__ */

