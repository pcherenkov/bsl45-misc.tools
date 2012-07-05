/* @(#) debugsync facility - compile with -std=gnu99 */

#include <sys/types.h>
#include <string.h>

#include <stdlib.h>
#include <stdbool.h>

#include <stdio.h>
#include <assert.h>

#include <pthread.h>
#include "debugsync.h"

#ifdef DEBUG
    #define TRACE( expr ) (expr)
#else
    #define TRACE( expr ) ((void)0)
#endif


/*
 * Module-scope numeric constants.
 */
enum {
	/* Maximum length of a sync point's name. */
	DS_MAX_POINT_NAME_LEN = 32,
	/* Maximum number of sync points allowed. */
	DS_MAX_POINT_COUNT  = 256
};


/*
 * Debug sync point data.
 */
struct ds_point {
	/* Unique name to use as ID. */
	char	*name;
	/* Enabled state indicator. */
	bool	is_enabled;
	/* Sync-wait state indicator. */
	bool	in_syncwait;
	/* # of active waiting sections. */
	size_t nblocked;
};


/*
 * Module-scope variables in a single structure.
 */
static struct ds_global {
	/* On/off switch for ALL debug sync operations. */
	bool		activated;

	/* Mutex, cond pair to signal state changes. */
	pthread_mutex_t	mtx;
	pthread_cond_t	cond;

	/* Debug sync points. */
	struct ds_point	point[DS_MAX_POINT_COUNT];
	size_t		count;
#ifdef DEBUG
	FILE*		log;
#endif
} ds;


/*
 * Implementation:
 */

int
ds_init(bool active)
{
	(void) memset(&ds.point[0], 0, sizeof(ds.point));

	ds.activated	= active;
	ds.count	= 0;
# ifdef DEBUG
	ds.log		= stderr;
# endif

	(void) pthread_mutex_init(&ds.mtx, NULL);
	(void) pthread_cond_init(&ds.cond, NULL);

	return 0;
}


static void
disable_all()
{
	bool has_pending_waits = false;
	for (size_t i = 0; i < ds.count; ++i) {
		if (ds.point[i].is_enabled) {
			ds.point[i].is_enabled = false;

			if (ds.point[i].nblocked > 0)
				has_pending_waits = true;
		}
	}
	if (has_pending_waits)
		pthread_cond_broadcast(&ds.cond);
}


void
ds_disable_all()
{
	(void) pthread_mutex_lock(&ds.mtx);
	if (ds.activated)
		disable_all();
	(void) pthread_mutex_unlock(&ds.mtx);
}


void
ds_activate(bool active)
{
	(void) pthread_mutex_lock(&ds.mtx);
		if (ds.activated)
			disable_all(); /* Err out pending waits. */
		ds.activated = active;
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
create_new(const char *point_name)
{
	if (ds.count >= DS_MAX_POINT_COUNT)
		return NULL;

	size_t i = ds.count;

	ds.point[i].name = strndup(point_name,
				DS_MAX_POINT_NAME_LEN);
	if (ds.point[i].name == NULL)
		return NULL;

	/* Create enabled points by default:
	 * consider the case when ds_wait() creates a
	 * sync point *before* control reaches ds_exec().
	 */
	ds.point[i].is_enabled = true;

	ds.point[i].in_syncwait = false;
	ds.point[i].nblocked = 0;

	++ds.count;

	return &ds.point[i];
}


static struct ds_point*
look_up(const char *point_name)
{
	for(size_t i = 0; i < ds.count; ++i)
		if (strcmp(point_name, ds.point[i].name) == 0)
			return &ds.point[i];
	return NULL;
}


static struct ds_point*
acquire(const char *point_name, const char *origin)
{
	(void)origin;

	struct ds_point *pt = look_up(point_name);
	if (pt == NULL)
		pt = create_new(point_name);

	if (pt == NULL) {
		TRACE((void) fprintf(ds.log, "%s:%s failed to get [%s]\n",
				origin, __func__, point_name));
	}

	return pt;
}


int
ds_enable(const char *point_name, bool enable, const char *origin)
{
	struct ds_point *pt = NULL;
	int rc = 0;

	(void)origin;
	TRACE((void) fprintf(ds.log, "%s:%s [%s, %d] IN\n",
			origin ? origin : "",__func__,
			point_name, (int)enable));

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		if (!ds.activated)
			break;

		pt = look_up(point_name);
		if (pt == NULL)
			break;

		pt->is_enabled = enable;

		/* If disabled - err out pending waits. */
		if (!pt->is_enabled && pt->nblocked > 0)
			rc = pthread_cond_broadcast(&ds.cond);

	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	TRACE((void) fprintf(ds.log, "%s:%s [%s, %d] OUT with rc=%d\n",
			origin ? origin : "",__func__,
			point_name, (int)enable, rc));
	return (pt == NULL) ? -1 : rc;
}


int
ds_exec(const char *point_name, const char *origin)
{
	struct ds_point *pt = NULL;
	int rc = 0;

	(void)origin;
	TRACE((void) fprintf(ds.log, "%s:%s [%s] IN\n",
			origin ? origin : "",__func__,
			point_name));

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		if (!ds.activated)
			break;

		pt = acquire(point_name, origin);
		if (pt == NULL)
			break;

		if (pt->is_enabled && pt->in_syncwait) {
			TRACE((void) fprintf(ds.log, "%s:%s [%s] is BUSY\n",
				origin ? origin: "", __func__, point_name));
			rc = -1;
			break;
		}

		pt->in_syncwait = true; /* Lock the sync point. */

		TRACE((void) fprintf(ds.log, "%s:%s RAISE [%s], %s\n",
			origin ? origin : "", __func__, pt->name,
			pt->is_enabled ? "enabled" : "disabled"));

		rc = pthread_cond_broadcast(&ds.cond);

		TRACE((void) fprintf(ds.log, "%s:%s HOLD [%s]\n",
			origin ? origin : "", __func__, pt->name));

		while (rc == 0 && pt->in_syncwait && pt->is_enabled)
				rc = pthread_cond_wait(&ds.cond, &ds.mtx);

		TRACE((void) fprintf(ds.log, "%s:%s UNLOCK [%s] %s %s\n",
			origin ? origin : "", __func__, pt->name,
			pt->is_enabled ? "enabled" : "disabled",
			pt->in_syncwait ? "+S" : "-S"));

		pt->in_syncwait = false; /* Sync point unlocked. */

		if (!pt->is_enabled) {
			TRACE((void) fprintf(ds.log, "%s:%s [%s] has been DISABLED\n",
				origin ? origin: "", __func__, pt->name));
			rc = -1;
			break;
		}

	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	TRACE((void) fprintf(ds.log, "%s:%s [%s] OUT with rc=%d\n",
			origin ? origin : "",__func__,
			point_name, rc));
	return (pt == NULL) ? -1 : rc;
}


int
ds_wait(const char *point_name, const char *origin)
{
	int rc = 0;
	struct ds_point *pt = NULL;

	(void)origin;
	TRACE((void) fprintf(ds.log, "%s:%s [%s] IN\n",
			origin ? origin : "", __func__, point_name));

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		if (!ds.activated)
			break;

		pt = acquire(point_name, origin);
		if (pt == NULL)
			break;

		pt->nblocked++;

		TRACE((void) fprintf(ds.log, "%s:%s [%s] WAIT with [%ld] blocked\n",
			origin ? origin : "", __func__, pt->name, (long)pt->nblocked));

		/* Wait for the point to reach "sync-wait" state,
		 * interrupt the wait if sync point gets disabled.
		 */
		while (rc == 0 && !pt->in_syncwait && pt->is_enabled)
			rc = pthread_cond_wait(&ds.cond, &ds.mtx);

		TRACE((void) fprintf(ds.log, "%s:%s [%s] WOKE UP with [%ld] blocked, %s %s\n",
			origin ? origin : "", __func__, pt->name, (long)pt->nblocked,
			pt->is_enabled ? "enabled" : "disabled",
			pt->in_syncwait ? "+S" : "-S"));

		if (!pt->is_enabled) {
			TRACE((void) fprintf(ds.log, "%s:%s [%s] has been DISABLED\n",
				origin ? origin: "", __func__, point_name));
			rc = -1;
			break;
		}
	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	(void) TRACE(fprintf(ds.log, "%s:%s [%s] OUT with rc=%d\n",
			origin ? origin : "", __func__,
			point_name, rc));
	return pt ? rc : -1;
}


int
ds_unblock(const char *point_name, const char *origin)
{
	int rc = 0;
	struct ds_point *pt = NULL;

	(void)origin;
	TRACE((void) fprintf(ds.log, "%s:%s [%s] IN\n",
			origin ? origin : "", __func__, point_name));

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		if (!ds.activated)
			break;

		pt = look_up(point_name);
		if (pt == NULL) {
			TRACE((void)fprintf(ds.log, "%s:%s [%s] does not exist\n",
				origin ? origin: "", __func__, point_name));
			break;
		}

		assert(pt->nblocked > 0);
		--pt->nblocked;

		TRACE((void) fprintf(ds.log, "%s:%s [%s] %ld blocked\n",
			origin ? origin : "", __func__, pt->name,
			(long)pt->nblocked));

		if (pt->nblocked == 0) {
			TRACE((void) fprintf(ds.log, "%s:%s [%s] - syncwait END\n",
				origin ? origin : "", __func__, pt->name));

			pt->in_syncwait = false;
			rc = pthread_cond_broadcast(&ds.cond);
		}
	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	TRACE((void) fprintf(ds.log, "%s:%s [%s] OUT with rc=%d\n",
			origin ? origin : "", __func__, point_name, rc));
	return pt ? rc : -1;
}


/* __EOF__ */

