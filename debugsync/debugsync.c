/* @(#) debugsync facility - compile with -std=gnu99 */

#include <sys/types.h>
#include <string.h>

#include <stdlib.h>
#include <stdbool.h>

#include <stdio.h>

#include <pthread.h>
#include "debugsync.h"


struct ds_point {
	char	*name;
	bool	is_enabled;	/* Point will fire on entry. */
	bool	in_syncwait;	/* Waiting sections not yet finished. */
	ssize_t nblocked;	/* Number of waiting threads. */
};


static struct ds_global {
	pthread_mutex_t	mtx;
	pthread_cond_t	cond;

	struct ds_point	point[DS_MAX_POINT_COUNT];
	size_t		count;
} ds;


int
ds_init()
{
	ds.count = 0;
	(void) pthread_mutex_init(&ds.mtx, NULL);
	(void) pthread_cond_init(&ds.cond, NULL);

	return 0;
}


void
ds_disable_all()
{
	(void) pthread_mutex_lock(&ds.mtx);
		size_t flipped = 0;
		for (size_t i = 0; i < ds.count; ++i) {
			if (ds.point[i].is_enabled) {
				ds.point[i].is_enabled = false;
				++flipped;
			}
		}
		if (flipped > 0)
			pthread_cond_broadcast(&ds.cond);
	(void) pthread_mutex_unlock(&ds.mtx);
}


void
ds_destroy()
{
	(void) pthread_cond_destroy(&ds.cond);
	(void) pthread_mutex_destroy(&ds.mtx);

	for (size_t i = 0; i < ds.count; ++i)
		free(ds.point[i].name);
}



static struct ds_point*
ds_add(const char *point_name)
{
	if (ds.count >= DS_MAX_POINT_COUNT)
		return NULL;

	size_t i = ds.count;

	ds.point[i].name = strndup(point_name,
				DS_MAX_POINT_NAME_LEN);
	if (ds.point[i].name == NULL)
		return NULL;

	/* Disabled points must be set so. */
	ds.point[i].is_enabled = true;

	ds.point[i].in_syncwait = false;
	ds.point[i].nblocked = 0;

	++ds.count;

	return &ds.point[i];
}


static struct ds_point*
ds_lookup(const char *point_name)
{
	for(size_t i = 0; i < ds.count; ++i)
		if (strcmp(point_name, ds.point[i].name) == 0)
			return &ds.point[i];
	return NULL;
}


static struct ds_point*
ds_get(const char *point_name, const char *origin)
{
	struct ds_point *pt = ds_lookup(point_name);
	if (pt == NULL)
		pt = ds_add(point_name);

	if (pt == NULL) {
		(void) fprintf(stderr, "%s:%s failed to get [%s]\n",
				origin, __func__, point_name);
	}

	return pt;
}


int
ds_exec(const char *point_name, bool enable, const char *origin)
{
	struct ds_point *pt = NULL;
	int rc = 0;
	(void) fprintf(stderr, "%s:%s [%s, %d] IN\n",
			origin ? origin : "",__func__,
			point_name, (int)enable);

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		pt = ds_get(point_name, origin);
		if (pt == NULL)
			break;

		pt->is_enabled = enable;

		if (pt->is_enabled && pt->in_syncwait) {
			(void) fprintf(stderr, "%s:%s [%s] is BUSY\n",
				origin ? origin: "", __func__, point_name);
			rc = -1;
			break;
		}

		pt->in_syncwait = true; /* Lock the sync point. */

		(void) fprintf(stderr, "%s:%s RAISE [%s], %s\n",
			origin ? origin : "", __func__, pt->name,
			pt->is_enabled ? "enabled" : "disabled");

		rc = pthread_cond_broadcast(&ds.cond);

		(void) fprintf(stderr, "%s:%s HOLD [%s]\n",
			origin ? origin : "", __func__, pt->name);

		while (rc == 0 && pt->in_syncwait && pt->is_enabled)
				rc = pthread_cond_wait(&ds.cond, &ds.mtx);

		(void) fprintf(stderr, "%s:%s UNLOCK [%s] %s %s\n",
			origin ? origin : "", __func__, pt->name,
			pt->is_enabled ? "enabled" : "disabled",
			pt->in_syncwait ? "+S" : "-S");

		pt->in_syncwait = false; /* Sync point unlocked. */

		if (!pt->is_enabled) {
			(void) fprintf(stderr, "%s:%s [%s] has been DISABLED\n",
				origin ? origin: "", __func__, pt->name);
			rc = -1;
			break;
		}

	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	(void) fprintf(stderr, "%s:%s [%s, %d] OUT with rc=%d\n",
			origin ? origin : "",__func__,
			point_name, (int)enable, rc);
	return (pt == NULL) ? -1 : rc;
}


int
ds_wait(const char *point_name, const char *origin)
{
	int rc = 0;
	struct ds_point *pt = NULL;

	(void) fprintf(stderr, "%s:%s [%s] IN\n",
			origin ? origin : "", __func__, point_name);

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		pt = ds_get(point_name, origin);
		if (pt == NULL)
			break;

		pt->nblocked++;

		(void) fprintf(stderr, "%s:%s [%s] WAIT with [%ld] blocked\n",
			origin ? origin : "", __func__, pt->name, (long)pt->nblocked);

		while (rc == 0 && !pt->in_syncwait && pt->is_enabled)
			rc = pthread_cond_wait(&ds.cond, &ds.mtx);

		(void) fprintf(stderr, "%s:%s [%s] WOKE UP with [%ld] blocked, %s %s\n",
			origin ? origin : "", __func__, pt->name, (long)pt->nblocked,
			pt->is_enabled ? "enabled" : "disabled",
			pt->in_syncwait ? "+S" : "-S");

		if (!pt->is_enabled) {
			(void) fprintf(stderr, "%s:%s [%s] has been DISABLED\n",
				origin ? origin: "", __func__, point_name);
			rc = -1;
			break;
		}
	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	(void) fprintf(stderr, "%s:%s [%s] OUT with rc=%d\n",
			origin ? origin : "", __func__,
			point_name, rc);
	return pt ? rc : -1;
}


int
ds_unblock(const char *point_name, const char *origin)
{
	int rc = 0;
	struct ds_point *pt = NULL;

	(void) fprintf(stderr, "%s:%s [%s] IN\n",
			origin ? origin : "", __func__, point_name);

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		pt = ds_get(point_name, origin);
		if (pt == NULL)
			break;

		--pt->nblocked;
		(void) fprintf(stderr, "%s:%s [%s] %ld blocked\n",
			origin ? origin : "", __func__, pt->name,
			(long)pt->nblocked);

		if (pt->nblocked == 0) {
			(void) fprintf(stderr, "%s:%s [%s] - syncwait END\n",
				origin ? origin : "", __func__, pt->name);

			pt->in_syncwait = false;
			rc = pthread_cond_broadcast(&ds.cond);
		}
	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	(void) fprintf(stderr, "%s:%s [%s] OUT with rc=%d\n",
			origin ? origin : "", __func__, point_name, rc);
	return pt ? rc : -1;
}


/* __EOF__ */

