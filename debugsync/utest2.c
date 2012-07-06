/* @(#) unit test for the debugsync facility */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include "debugsync.h"


static void*
run(const char *name, unsigned int pnap, unsigned int anap, const char *origin)
{
	long rc = 0;

	(void)fprintf(stderr, "0x%lx:%s [%s] pnap=%u, anap=%u BEGIN\n",
		(long)pthread_self(), origin ? origin : "?", name, pnap, anap);
	do {
		if (pnap) {
			(void) fprintf(stderr, "%s: taking P-nap for %u sec\n", origin, pnap);
			(void) sleep(pnap);
			(void) fprintf(stderr, "%s: P-woke up\n", origin);
		}

		rc = ds_wait(name);
		if (rc) break;

		if (anap) {
			(void) fprintf(stderr, "%s: taking A-nap for %u sec\n", origin, anap);
			(void) sleep(anap);
			(void) fprintf(stderr, "%s: A-woke up\n", origin);
		}

		rc = ds_unblock(name);
		if (rc) break;
	} while(0);
	(void)fprintf(stderr, "0x%lx:%s END (%ld)\n", (long)pthread_self(), origin, rc);

	return (void*)rc;
}


static void*
syncpt(const char *name, unsigned int pnap, unsigned int anap, const char *origin)
{
	long rc = 0;

	(void)fprintf(stderr, "0x%lx:%s [%s] pnap=%u, anap=%u BEGIN\n",
		(long)pthread_self(), origin ? origin : "?", name, pnap, anap);
	do {
		if (pnap) {
			(void) fprintf(stderr, "%s: P-nap (%u) sec\n", origin, pnap);
			(void) sleep(pnap);
			(void) fprintf(stderr, "%s: P-woke up\n", origin);
		}

		rc = ds_exec(name);
		if (rc) break;

		if (anap) {
			(void) fprintf(stderr, "%s: taking A-nap (%u) sec\n", origin, anap);
			(void) sleep(anap);
			(void) fprintf(stderr, "%s: A-woke up\n", origin);
		}

	} while(0);
	(void)fprintf(stderr, "0x%lx:%s END (%ld)\n", (long)pthread_self(), origin, rc);

	return (void*)rc;
}

static void* t1_run(void *arg) { (void)arg; return run("hollywood", 1, 2, __func__); }
static void* t2_run(void *arg) { (void)arg; return run("hollywood", 2, 3, __func__); }
static void* t3_run(void *arg) { (void)arg; return run("hollywood", 8, 1, __func__); }

static void* t4_run(void *arg) { (void)arg; return syncpt("dania", 3, 2, __func__); }
static void* t5_run(void *arg) { (void)arg; return run("dania", 0, 2, __func__); }
static void* t6_run(void *arg) { (void)arg; return run("dania", 1, 1, __func__); }

int main()
{
	int rc = 0, jrc = 0;

	static struct thr {
		pthread_t tid;
		 void *(*start) (void *);
	} t[] = {	{0, &t1_run}, {0, &t2_run}, {0, &t3_run},
			{0, &t4_run}, {0, &t5_run}, {0, &t6_run}
		};
	static const size_t THR_COUNT = sizeof(t) / sizeof(t[0]);

	(void) fprintf(stderr, "%s: started.\n", __func__);
	do {
		rc = ds_init(0);
		if (rc) break;

		for (size_t i = 0; rc == 0 && i < THR_COUNT; ++i)
			rc = pthread_create(&t[i].tid, NULL, t[i].start, NULL);
		if (rc) {
			errno = rc;
			perror("pthread_create");
			break;
		}

		(void) fprintf(stderr, "%s: PAUSING\n", __func__);
		sleep(3);
		(void) fprintf(stderr, "%s: DONE PAUSING\n", __func__);

		rc = DSYNC_SET("hollywood");
		if (rc) break;

		(void) fprintf(stderr, "%s: on we go.\n", __func__);
	} while(0);

	sleep(3);
	ds_disable_all();

	for (size_t i = 0; i < THR_COUNT; ++i) {
		(void) fprintf(stderr, "Joining thread %ld (0x%lx):\t", (long)i+1,
			(long)t[i].tid);
		jrc = pthread_join(t[i].tid, NULL);
		(void) fprintf(stderr, "%s (%d)\n", jrc ? "FAILED" : "OK", jrc);
		if (jrc)
			rc = jrc;
	}

	ds_destroy();

	(void) fprintf(stderr, "%s: exit (%d).\n", __func__, rc);
	return rc;
}


/* __EOF__ */

