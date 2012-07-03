/* @(#) debugsync facility - compile with -std=gnu99 */

#include <sys/types.h>
#include <string.h>

#include <stdlib.h>
#include <stdbool.h>

#include <pthread.h>
#include "debugsync.h"

#ifndef DS_SIG_MAXLEN
	#define DS_SIG_MAXLEN	32
#endif
#ifndef DS_MAX_COUNT
	#define	DS_MAX_COUNT	1024
#endif

struct ds_point {
	char	*name;
	bool	is_enabled;
};


static struct ds_global {
	pthread_mutex_t	mtx;
	pthread_cond_t	cond;
	char		signal[DS_SIG_MAXLEN + 1];

	struct ds_point	point[DS_MAX_COUNT];
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
	if (ds.count >= DS_MAX_COUNT)
		return NULL;

	size_t i = ds.count;

	ds.point[i].name = strndup(point_name, DS_SIG_MAXLEN);
	if (ds.point[i].name == NULL)
		return NULL;

	ds.point[i].is_enabled = false;
	++ds.count;

	return &ds.point[i];
}


static struct ds_point*
ds_lookup(const char *point_name)
{
	for(size_t i = 0; i < ds.count; ++i) {
		if (strcmp(point_name, ds.point[i].name) == 0)
			return &ds.point[i];
	}

	return NULL;
}


static inline int
wait_on(const char *point_name, bool *enabled)
{
	int rc = 0;
	while (rc == 0 && strncmp(ds.signal, point_name, sizeof(ds.signal)) != 0) {
		if (enabled && *enabled == false)
			return -1;
		rc = pthread_cond_wait(&ds.cond, &ds.mtx);
	}
	return rc;
}


int
ds_set(const char *point_name, bool enable)
{
	struct ds_point *pt = NULL;
	int rc = 0;

	(void) pthread_mutex_lock(&ds.mtx);
		if ((pt = ds_lookup(point_name)) == NULL)
			pt = ds_add(point_name);
		if (pt != NULL) {
			pt->is_enabled = enable;
			if (pt->is_enabled) {
				rc = wait_on(point_name, &pt->is_enabled);
				pt->is_enabled = false;
			}
		}
	(void) pthread_mutex_unlock(&ds.mtx);

	return (pt == NULL) ? -1 : rc;
}


int
ds_wait(const char *point_name)
{
	(void) pthread_mutex_lock(&ds.mtx);
		int rc = wait_on(point_name, NULL);
	(void) pthread_mutex_unlock(&ds.mtx);

	return rc;
}


int
ds_signal(const char *point_name)
{
	(void) pthread_mutex_lock(&ds.mtx);
		(void) strncpy(ds.signal, point_name, sizeof(ds.signal)-1);
		ds.signal[sizeof(ds.signal)-1] = '\0';

		int rc = pthread_cond_signal(&ds.cond);
	(void) pthread_mutex_unlock(&ds.mtx);

	return rc;
}


/* __EOF__ */

