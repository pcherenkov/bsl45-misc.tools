/* @(#) b64 encode/decode unit test. */

#include <sys/types.h>
#include <stdio.h>

#include <string.h>
#include <stdlib.h>

#include "b64.h"

int
main(int argc, char *const argv[])
{
	int rc = 0;
	const char *mode = NULL, *str = NULL;
	char *buf = NULL, *res = NULL;
	size_t buflen = 0;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s encode|decode {string}\n",
			argv[0]);
		return 1;
	}

	mode = argv[1];
	str = argv[2];
	buflen = (strlen(str) / 3 + 1) * 4 + 1;
	buf = calloc(1, buflen);
	if (NULL == buf) {
		fprintf(stderr, "Failed to allocate %ld bytes.\n",
			(long)buflen);
		return 1;
	}

	if (0 == strcasecmp("encode", mode))
		res = b64_encode(str, buf, buflen);
	else if (0 == strcasecmp("decode", mode))
		res = b64_decode(str, buf, buflen);
	else {
		fprintf(stderr, "Invalid mode: %s\n", mode);
		res = NULL;
	}

	if (res)
		printf("%s\n", res);
	else
		fprintf(stderr, "ERROR processing %s\n", str);

	free(buf);
	return rc;
}

/* __EOF__ */

