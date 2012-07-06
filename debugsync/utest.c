/* @(#) unit test for debug sync facility */

#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include <pthread.h>

#include "debugsync.h"


static void*
t1_start(void *ptr)
{
	long rc = 0;

	(void) fprintf(stderr, "== %s IN [%p]\n", __func__, ptr);
	do {
		rc = ds_set("ANNY", true, __func__);
		if (rc) break;

		rc = ds_wait("MAIN_DONE", __func__);
	} while(0);

	(void) fprintf(stderr, "== %s OUT [%ld]\n", __func__, rc);
	return (void*)rc;
}


static void*
t2_start(void *ptr)
{
	long rc = 0;

	(void) fprintf(stderr, "== %s IN [%p]\n", __func__, ptr);
	do {
		rc = ds_set("BENNY", true, __func__);
		if (rc) break;

		rc = ds_wait("MAIN_DONE", __func__);
	} while(0);

	(void) fprintf(stderr, "== %s OUT [%ld]\n", __func__, rc);
	return (void*)rc;
}


static void*
t3_start(void *ptr)
{
	long rc = 0;

	(void) fprintf(stderr, "== %s IN [%p]\n", __func__, ptr);
	do {
		rc = ds_set("RABBA", false, __func__);
		if (rc) break;

		(void) sleep(2);
		rc = ds_signal("ANNY", __func__);
		if (rc) break;

		(void) sleep(2);
		rc = ds_signal("BENNY", __func__);
		if (rc) break;

		rc = ds_wait("MAIN_DONE", __func__);
	} while(0);

	(void) fprintf(stderr, "== %s OUT [%ld]\n", __func__, rc);
	return (void*)rc;
}


int main()
{
	int rc = 0;

	struct tspec {
		pthread_t t;
		void *(*start) (void *);
	} thr[3] = {{0, &t1_start}, {0, &t2_start}, {0, &t3_start}};

	const size_t N_THREADS = sizeof(thr)/sizeof(thr[0]);
	size_t i = 0;
	void *ret = NULL;

	do {
		rc = ds_init();
		if (rc) break;

		rc = ds_set("MAIN_DONE", false, __func__);

		for (i = 0; i < N_THREADS; ++i) {
			(void) fprintf(stderr, "%s: starting t%ld\n", __func__, (long)i+1);
			rc = pthread_create(&thr[i].t, NULL, thr[i].start, NULL);
			if (rc) break;
		}

		const unsigned int nap = 10;
		(void) fprintf(stderr, "%s: taking a nap for %d seconds\n",
			__func__, nap);
		(void) sleep(nap);
		(void) fprintf(stderr, "%s: woke up\n", __func__);

		rc = ds_signal("MAIN_DONE", __func__);
		if (rc) break;
	} while(0);

	for(i = 0; i < N_THREADS; ++i) {
		if (thr[i].t) {
			(void) fprintf(stderr, "%s: joining t%ld .. ",
				__func__, (long)i+1);
			rc = pthread_join(thr[i].t, &ret);
			(void) fprintf(stderr, "%s\n", rc ? "FAILED" : "done");
		}
	}

	ds_destroy();

	(void) fprintf(stderr, "%s exiting [%d]\n", __func__, rc);
	return rc;
}

/* __EOF__ */

