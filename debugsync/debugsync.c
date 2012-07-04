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
	bool	is_enabled;
	bool	is_active;
	ssize_t block_count;
};


static struct ds_global {
	pthread_mutex_t	mtx;
	pthread_cond_t	cond;
	char		signal[DS_MAX_POINT_NAME_LEN + 1];

	struct ds_point	point[DS_MAX_POINT_COUNT];
	size_t		count;
} ds;


int
ds_init()
{
	int rc = 0;

	(void) pthread_mutex_init(&ds.mtx, NULL);
	(void) pthread_cond_init(&ds.cond, NULL);

	ds.signal[0] = '\0';
	ds.count = 0;

	return rc;
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

	ds.point[i].is_enabled = false;
	ds.point[i].is_active = false;
	ds.point[i].block_count = 0;

	++ds.count;

	return &ds.point[i];
}


static struct ds_point*
ds_lookup(const char *point_name, const char *origin, bool must_exist)
{
	for(size_t i = 0; i < ds.count; ++i)
		if (strcmp(point_name, ds.point[i].name) == 0)
			return &ds.point[i];
	if (must_exist)
		(void) fprintf(stderr, "%s:%s ERR [%s] not found\n",
			origin ? origin : "", __func__, point_name);
	return NULL;
}



int
ds_set(const char *point_name, bool enable, const char *origin)
{
	struct ds_point *pt = NULL;
	int rc = 0;
	(void) fprintf(stderr, "%s:%s [%s, %d] IN\n",
			origin ? origin : "",__func__,
			point_name, (int)enable);

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		if ((pt = ds_lookup(point_name, origin, false)) == NULL)
			pt = ds_add(point_name);
		if (pt == NULL)
			break;

		pt->is_enabled = enable;
		if (!pt->is_enabled)
			break;

		/* Set to true by waiting threads. */
		pt->is_active	= false;

		/* It is no longer feasible to join in ds_wait(). */
		pt->is_signalled = false;

		rc = pthread_cond_broadcast(&ds.cond);

		while (rc == 0 && (pt->block_count > 0) &&
			!pt->is_active && strcmp(ds.signal, point_name) != 0)
				rc = pthread_cond_wait(&ds.cond, &ds.mtx);

		pt->is_active	= false;

		if (!pt->is_signalled) {
			/* Woken up not by signal but by another sync point? */
			rc = -1;
			break;
		}

		/* TODO: how do we guard against re-entry ? */
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
	(void) fprintf(stderr, "%s:%s [%s] IN\n",
			origin ? origin : "", __func__, point_name);

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		while (rc == 0 && strcmp(ds.signal, point_name) != 0)
			rc = pthread_cond_wait(&ds.cond, &ds.mtx);

		/* Woke up after a sync point has been reached. */
		struct ds_point *pt = ds_lookup(point_name, origin, true);
		if (pt == NULL || (pt && !pt->is_enabled)) {
			rc = -1;
			break;
		}

		if (pt->is_signalled) {
			/* Woken up by ds_signal() - wrong. */
			rc = -1;
			break;
		}

		/* Woken up by ds_set() - OK. */
		pt->is_active = true;
		pt->block_count++;
	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	/* TODO: remove after DEBUG */
	if (pt && !pt->is_enabled) {
		(void) fprintf(stderr, "%s:%s point [%s] is disabled\n",
			origin ? origin : "", __func__, point_name);
		rc = -1;
	}

	(void) fprintf(stderr, "%s:%s [%s] OUT with rc=%d\n",
			origin ? origin : "", __func__,
			point_name, rc);
	return rc;
}


int
ds_signal(const char *point_name, const char *origin)
{
	int rc = 0;
	(void) fprintf(stderr, "%s:%s [%s] IN\n",
			origin ? origin : "", __func__, point_name);

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		struct ds_point *pt = ds_lookup(point_name, origin, true);
		if (pt == NULL || (pt && !pt->is_enabled)) {
			rc = -1;
			break;
		}

		(void) strncpy(ds.signal, point_name, sizeof(ds.signal)-1);
		ds.signal[sizeof(ds.signal)-1] = '\0';

		pt->is_signalled = true;
		pt->block_count--;

		rc = pthread_cond_broadcast(&ds.cond);
	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	(void) fprintf(stderr, "%s:%s [%s] OUT with rc=%d\n",
			origin ? origin : "", __func__, point_name, rc);
	return rc;
}


/* __EOF__ */

