/* @(#) debugsync facility */

#include <sys/types.h>

#include <stdlib.h>
#include <stdbool.h>

#include <pthread.h>

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
	char 		signal[DS_MAXLEN + 1];
	struct ds_point	*points;
	size_t		points_alloc,
			npoints;
} ds_data;


int
ds_init()
{
	int rc = 0;

	(void) pthread_mutex_init(&ds_data.mtx, NULL);
	(void) pthread_cond_init(&ds_data.cond, NULL);

	ds_data.signal[0] = '\0';
	ds_data.points = NULL;
	ds_data.point_count = 0;

	return rc;
}


void
ds_free()
{
	(void) pthread_cond_destroy(&ds_data.cond);
	(void) pthread_mutex_destroy(&ds_data.mtx);

	free(ds_data.points);
}


int
ds_set(const char *ds_point_name, bool enable)
{
	struct ds_point *pt = ds_lookup(ds_point_name); /* TODO: */
	if (pt == NULL) {
		pt = ds_add(ds_point_name);
		if (pt == NULL)
			return -1;	/* TODO: */
	}

	pt->is_enabled = enable;
	return 0;
}


int
ds_wait(const char *ds_point_name)
{
	int rc = 0;
	struct ds_point *pt = ds_lookup(ds_point_name); /* TODO: */
	if (pt == NULL)
		return -1;	/* TODO: */
	
	(void) pthread_mutex_lock(&ds_data.mtx);
	while (strncmp(ds_data.signal, ds_point_name) != 0 && rc == 0)
		rc = pthread_cond_wait(&ds_data.cond, &ds_data.mtx);
	(void) pthread_mutex_unlock(&ds_data.mtx);

	return rc;
}


int
ds_signal(const char *ds_point_name)
{
	int rc = 0;
	struct ds_point *pt = ds_lookup(ds_point_name); /* TODO: */
	if (pt == NULL)
		return -1;	/* TODO: */

	(void) pthread_mutex_lock(&ds_data.mtx);
		(void) strcpy(ds_data.signal, ds_point_name);
		rc = pthread_cond_signal(&ds_data.cond);
	(void) pthread_mutex_unlock(&ds_data.mtx);

	return rc;
}


/* __EOF__ */

