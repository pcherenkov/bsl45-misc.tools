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


/**
 * Module-scope numeric constants.
 */

enum {
	/** Maximum length of a sync point's name. */
	DS_MAX_POINT_NAME_LEN = 32,
	/** Maximum number of sync points allowed. */
	DS_MAX_POINT_COUNT  = 256
};


/**
 * Debug sync point data.
 */
struct ds_point {
	/** Unique name to use as ID. */
	char		*name;
	/** Enabled state indicator. */
	bool		is_enabled;
	/** Sync-wait state indicator. */
	bool		in_syncwait;
	/** # of active waiting sections. */
	size_t 		nblocked;
};


/**
 * Module-scope variables in a single structure.
 */
static struct ds_global {
	/**  Activation control flags (see header for details). */
	u_int32_t	activation;

	/** Mutex, cond pair to signal state changes. */
	pthread_mutex_t	mtx;
	pthread_cond_t	cond;

	/** Debug sync points. */
	struct ds_point	point[DS_MAX_POINT_COUNT];
	size_t		count;
#ifdef DEBUG
	FILE*		log;
#endif
} ds;


/*
 * Local-scope functions: 
 */


/** True if debug sync is inactive.
 * @return true if the framework is disabled (inactive).
 */
inline static bool inactive() { return ds.activation & DS_INACTIVE; }


/** True if debug sync is inactive AND activation is local.
 * @return true if debug sync is inactive AND activation is local.
 */
inline static bool
local_inactive()
{
	return ds.activation & (DS_INACTIVE | DS_LOCAL_ACTIVATION);
}


/** Set the activation flag.
 * @param activate Activate if true, disable if false.
 */
inline static void
do_activate(bool activate)
{
	if (activate)
		ds.activation &= ~DS_INACTIVE;
	else
		ds.activation |= DS_INACTIVE;
}


/** Disable all sync points, wake up pending wait sections. */
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


/** Create a new sync point.
 *
 * @param point_name Name of the new sync point.
 *
 * @return pointer to the newly-created sync point or NULL.
 */
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


/** Locate a sync point by name.
 *
 * @param point_name Name of the sync point.
 *
 * @return pointer to the named sync point, if found, otherwise - NULL.
 */
static struct ds_point*
look_up(const char *point_name)
{
	/* NB: This does not scale to large ds.count, but neither
	 * is ds.count expected to be large enough to need a faster
	 * lookup.
	 */
	for(size_t i = 0; i < ds.count; ++i)
		if (strcmp(point_name, ds.point[i].name) == 0)
			return &ds.point[i];
	return NULL;
}


/** Locate a sync point by name, create it if not found.
 *
 * @param point_name Name of the sync point.
 *
 * @return pointer to the named sync point, or NULL if error.
 */
static struct ds_point*
acquire(const char *point_name)
{
	struct ds_point *pt = look_up(point_name);
	if (pt == NULL)
		pt = create_new(point_name);

	if (pt == NULL) {
		TRACE((void) fprintf(ds.log, "0x%lx:%s failed to get [%s]\n",
				(long)pthread_self(), __func__, point_name));
	}

	return pt;
}


/*
 * Implementation:
 */


int
ds_init(u_int32_t activation_flags)
{
	(void) memset(&ds.point[0], 0, sizeof(ds.point));

	ds.activation	= activation_flags;
	ds.count	= 0;
# ifdef DEBUG
	ds.log		= stderr;
# endif

	(void) pthread_mutex_init(&ds.mtx, NULL);
	(void) pthread_cond_init(&ds.cond, NULL);

	return 0;
}


void
ds_disable_all()
{
	if (local_inactive())
		return;

	(void) pthread_mutex_lock(&ds.mtx);
	if (!inactive())
		disable_all();
	(void) pthread_mutex_unlock(&ds.mtx);
}


void
ds_activate(bool activate)
{
	if (local_inactive() && activate)
	/* No need to lock if local & enabling (no chance of pending waits). */
		do_activate(activate);
	else {
		(void) pthread_mutex_lock(&ds.mtx);
			if (!inactive()) /* Err out pending waits. */
				disable_all();
			do_activate(activate);
		(void) pthread_mutex_unlock(&ds.mtx);
	}
}


void
ds_destroy()
{
	(void) pthread_cond_destroy(&ds.cond);
	(void) pthread_mutex_destroy(&ds.mtx);

	for (size_t i = 0; i < ds.count; ++i)
		free(ds.point[i].name);
}


int
ds_enable(const char *point_name, bool enable)
{
	struct ds_point *pt = NULL;
	int rc = 0;

	if (local_inactive())
		return 0;

	TRACE((void) fprintf(ds.log, "0x%lx:%s [%s, %d] IN\n",
			(long)pthread_self,__func__,
			point_name, (int)enable));

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		if (inactive())
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

	TRACE((void) fprintf(ds.log, "0x%lx:%s [%s, %d] OUT with rc=%d\n",
			(long)pthread_self(),__func__,
			point_name, (int)enable, rc));
	return (pt == NULL) ? -1 : rc;
}


int
ds_exec(const char *point_name)
{
	struct ds_point *pt = NULL;
	int rc = 0;

	if (local_inactive())
		return 0;

	TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] IN\n",
			(long)pthread_self(),__func__,
			point_name));

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		if (inactive())
			break;

		pt = acquire(point_name);
		if (pt == NULL)
			break;

		if (pt->is_enabled && pt->in_syncwait) {
			TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] is BUSY\n",
				(long)pthread_self(), __func__, point_name));
			rc = -1;
			break;
		}

		/* No waiters - bail out. */
		if (pt->nblocked == 0) {
			TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] is IDLE\n",
				(long)pthread_self(), __func__, point_name));
			break;
		}

		pt->in_syncwait = true; /* Lock the sync point. */

		TRACE((void) fprintf(ds.log, "0x%lx:%s RAISE [%s], %s\n",
			(long)pthread_self(), __func__, pt->name,
			pt->is_enabled ? "enabled" : "disabled"));

		rc = pthread_cond_broadcast(&ds.cond);

		TRACE((void) fprintf(ds.log, "0x%lx:%s HOLD [%s]\n",
			(long)pthread_self(), __func__, pt->name));

		while (rc == 0 && pt->in_syncwait && pt->is_enabled)
				rc = pthread_cond_wait(&ds.cond, &ds.mtx);

		TRACE((void) fprintf(ds.log, "0x%lx:%s UNLOCK [%s] %s %s\n",
			(long)pthread_self(), __func__, pt->name,
			pt->is_enabled ? "enabled" : "disabled",
			pt->in_syncwait ? "+S" : "-S"));

		pt->in_syncwait = false; /* Sync point unlocked. */

		if (!pt->is_enabled) {
			TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] has been DISABLED\n",
				(long)pthread_self(), __func__, pt->name));
			rc = -1;
			break;
		}

	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] OUT with rc=%d\n",
			(long)pthread_self(),__func__,
			point_name, rc));
	return (pt == NULL) ? -1 : rc;
}


int
ds_wait(const char *point_name)
{
	int rc = 0;
	struct ds_point *pt = NULL;

	if (local_inactive())
		return 0;

	TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] IN\n",
			(long)pthread_self(), __func__, point_name));

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		if (inactive())
			break;

		pt = acquire(point_name);
		if (pt == NULL)
			break;

		pt->nblocked++;

		TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] WAIT with [%ld] blocked\n",
			(long)pthread_self(), __func__, pt->name, (long)pt->nblocked));

		/* Wait for the point to reach "sync-wait" state,
		 * interrupt the wait if sync point gets disabled.
		 */
		while (rc == 0 && !pt->in_syncwait && pt->is_enabled)
			rc = pthread_cond_wait(&ds.cond, &ds.mtx);

		TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] WOKE UP with [%ld] blocked, %s %s\n",
			(long)pthread_self(), __func__, pt->name, (long)pt->nblocked,
			pt->is_enabled ? "enabled" : "disabled",
			pt->in_syncwait ? "+S" : "-S"));

		if (!pt->is_enabled) {
			TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] has been DISABLED\n",
				(long)pthread_self(), __func__, point_name));
			rc = -1;
			break;
		}
	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	(void) TRACE(fprintf(ds.log, "0x%lx:%s [%s] OUT with rc=%d\n",
			(long)pthread_self(), __func__,
			point_name, rc));
	return pt ? rc : -1;
}


int
ds_unblock(const char *point_name)
{
	int rc = 0;
	struct ds_point *pt = NULL;

	if (local_inactive())
		return 0;

	TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] IN\n",
			pthread_self(), __func__, point_name));

	(void) pthread_mutex_lock(&ds.mtx);
	do {
		if (inactive())
			break;

		pt = look_up(point_name);
		if (pt == NULL) {
			TRACE((void)fprintf(ds.log, "0x%lx:%s [%s] does not exist\n",
				(long)pthread_self(), __func__, point_name));
			break;
		}

		assert(pt->nblocked > 0);
		--pt->nblocked;

		TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] %ld blocked\n",
			(long)pthread_self(), __func__, pt->name,
			(long)pt->nblocked));

		if (pt->nblocked == 0) {
			TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] - syncwait END\n",
				pthread_self(), __func__, pt->name));

			pt->in_syncwait = false;
			rc = pthread_cond_broadcast(&ds.cond);
		}
	} while(0);
	(void) pthread_mutex_unlock(&ds.mtx);

	TRACE((void) fprintf(ds.log, "0x%lx:%s [%s] OUT with rc=%d\n",
			(long)pthread_self(), __func__, point_name, rc));
	return pt ? rc : -1;
}


/* __EOF__ */

