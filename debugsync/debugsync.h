/* @(#) debugsync facility interface */

#ifndef DEBUGSYNC_H_20120703
#define DEBUGSYNC_H_20120703

#include <stdbool.h>

#ifdef NDEBUG

#	define	DSYNC_SET(name, enable)
#	define	DSYNC_WAIT(name)
#	define	DSYNC_UNBLOCK(name)

#else

#	define	DSYNC_SET(name, enable)	ds_exec(name, enable, __func__)
#	define	DSYNC_WAIT(name)	ds_wait(name, __func__)
#	define	DSYNC_UNBLOCK(name)	ds_unblock(name, __func__)

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize debug sync framework (set up internal structures).
 */
int ds_init();

/**
 * Release resources allocated for debug sync framework.
 */
void ds_destroy();

/**
 * Disable all data sync points (defined so far).
 */
void ds_disable_all();

/**
 * Create a sync point or set status of an existing one.
 *
 * @param point_name name of the sync point.
 * @param enable enable or disable the sync point.
 *
 */
int ds_exec(const char *point_name, bool enable, const char *origin);

int ds_wait(const char *point_name, const char *origin);

int ds_unblock(const char *point_name, const char *origin);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGSYNC_H_20120703 */
/* __EOF__ */

