/* @(#) checking clock_gettime resolution */
/* gcc -W -Wall --pedantic -Wl,--no-as-needed -lev -lrt ./clkgtm.c */

#include <sys/types.h>

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <assert.h>

#include "ev.h"

#if _POSIX_C_SOURCE < 199309L
#define _POSIX_C_SOURCE 199309L
#endif


static struct timespec*
timespec_sub(const struct timespec *a, const struct timespec *b,
		struct timespec *r)
{
	assert(a && b && r);

	r->tv_sec  = a->tv_sec  - b->tv_sec;
	r->tv_nsec = a->tv_nsec - b->tv_nsec;
	if (r->tv_nsec < 0) {
		r->tv_nsec += 1000000000;
		r->tv_sec--;
	}

	printf("a=%ld.%ld b=%ld.%ld r=%ld.%09ld\n",
		a->tv_sec, a->tv_nsec, b->tv_sec, b->tv_nsec,
		r->tv_sec, r->tv_nsec);

	return r;
}


static int
measure_sec(long nsec)
{
    int rc = 0;
    struct timespec before, after, delta, slptm;
    ev_tstamp evb, eva;

    slptm.tv_sec = 0;
    slptm.tv_nsec = nsec;

    rc = clock_gettime(CLOCK_MONOTONIC, &before);
    if (rc) {
        perror("clock_gettime");
        return -1;
    }
    evb = ev_time();
    nanosleep(&slptm, NULL);
    eva = ev_time();
    rc = clock_gettime(CLOCK_MONOTONIC, &after);
    if (rc) {
        perror("clock_gettime");
        return -1;
    }

    (void) timespec_sub(&after, &before, &delta);
    (void) printf("ev before=%.09f, ev after=%.09f, delta=%.09f\n",
                    evb, eva, eva - evb);

    return 0;
}


int main(int argc, char* const argv[])
{
    long nsec = 0;

    if (argc < 2) {
        (void) fprintf(stderr, "Usage: %s nanoseconds\n", argv[0]);
        return 1;
    }

    nsec = atol(argv[1]);
    if (nsec <= 0) {
        (void) fprintf(stderr, "%s: invalid value: %s\n", argv[0], argv[1]);
        return 2;
    }

    return measure_sec(nsec);
}



/* __EOF__ */

