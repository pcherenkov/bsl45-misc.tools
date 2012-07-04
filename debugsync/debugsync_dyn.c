/* @(#) debugsync facility */

#include <sys/types.h>
#include <string.h>

#include <stdlib.h>
#include <stdbool.h>

#include <pthread.h>
#include "debugsync.h"

#ifndef DS_MAXLEN
	#define DS_MAXLEN	32
#endif

struct ds_point {
	char	name[DS_MAXLEN + 1];	/* sync point name	*/
	bool	is_enabled;
};


static struct ds_global {
	pthread_mutex_t	mtx;
	pthread_cond_t	cond;
	char		signal[DS_MAXLEN + 1];
	struct ds_point	**point;
	size_t		capacity,
			count;
} ds;


int
ds_init()
{
	int rc = 0;

	(void) pthread_mutex_init(&ds.mtx, NULL);
	(void) pthread_cond_init(&ds.cond, NULL);

	ds.signal[0] = '\0';
	ds.point = NULL;
	ds.count = 0;

	return rc;
}


void
ds_destroy()
{
	(void) pthread_cond_destroy(&ds.cond);
	(void) pthread_mutex_destroy(&ds.mtx);

	for (size_t i = 0; i < ds.count; ++i)
		free(ds.point[i]);
	free(ds.point);
}


inline static int
ds_cmp(const void *p1, const void *p2)
{
	const struct ds_point	*a = *(struct ds_point**)p1,
				*b = *(struct ds_point**)p2;
	return strcmp(a->name, b->name);
}


inline static void
swap_point(size_t i, size_t j)
{
	struct ds_point *tmp = ds.point[i];
	ds.point[i] = ds.point[j];
	ds.point[j] = tmp;
	/* Better performance with 3rd value. */
}


static struct ds_point*
ds_add(const char *point_name)
{
#ifndef DS_EXPAND_BY
	#define DS_EXPAND_BY	8
#endif
	if (ds.capacity <= ds.count) {
		ds.point = realloc(ds.point,
			(DS_EXPAND_BY + ds.capacity) * sizeof(ds.point[0]));
		if (ds.point == NULL)
			return NULL;

		ds.capacity += DS_EXPAND_BY;
	}

	size_t i = ds.count;
	++ds.count;

	ds.point[i] = malloc(sizeof(ds.point[0]));
	if (ds.point[i] == NULL)
		return NULL;

	(void) strncpy(ds.point[i]->name, point_name, sizeof(ds.point[i]->name)-1);
	ds.point[i]->name[sizeof(ds.point[i]->name)-1] = '\0';

	ds.point[i]->is_enabled = false;

	while (i > 0 && ds_cmp(&ds.point[i], &ds.point[i-1]) < 0) {
		swap_point(i, i-1);
		--i;
	}

	return ds.point[i];
}


static struct ds_point*
ds_lookup(const char *point_name)
{
	struct ds_point key = {"", false};

	(void) strncpy(key.name, point_name, sizeof(key.name)-1);
	key.name[sizeof(key.name)-1] = '\0';

	return (struct ds_point*)bsearch(&key, &ds.point[0],
		ds.count, sizeof(ds.point[0]), &ds_cmp);
}


static inline int
wait_on(const char *point_name, bool* enabled)
{
	int rc = 0;
	while (	rc == 0 && strncmp(ds.signal, point_name, sizeof(ds.signal)) != 0) {
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
	int rc = 0;

	(void) pthread_mutex_lock(&ds.mtx);
		rc = wait_on(point_name, NULL);
	(void) pthread_mutex_unlock(&ds.mtx);

	return rc;
}


int
ds_signal(const char *point_name)
{
	int rc = 0;

	(void) pthread_mutex_lock(&ds.mtx);
		(void) strncpy(ds.signal, point_name, sizeof(ds.signal)-1);
		ds.signal[sizeof(ds.signal)-1] = '\0';

		rc = pthread_cond_signal(&ds.cond);
	(void) pthread_mutex_unlock(&ds.mtx);

	return rc;
}


/* __EOF__ */

