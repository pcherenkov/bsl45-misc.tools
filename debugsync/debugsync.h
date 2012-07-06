/* @(#) debugsync facility interface */

#ifndef DEBUGSYNC_H_20120703
#define DEBUGSYNC_H_20120703

#include <sys/types.h>
#include <stdbool.h>

#ifdef NDEBUG

#	define	DSYNC_SET(name)
#	define	DSYNC_WAIT(name)
#	define	DSYNC_UNBLOCK(name)

#else

#	define	DSYNC_SET(name)		ds_exec(name)
#	define	DSYNC_WAIT(name)	ds_wait(name)
#	define	DSYNC_UNBLOCK(name)	ds_unblock(name)

#endif


/* Debug sync activation flags. */
enum {
	/* Disable all framework functionality. */
	DS_INACTIVE 		= 1,
	/* [De-]Activation flag is *NOT* propagated across threads. */
	DS_LOCAL_ACTIVATION	= (1 << 1)
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize debug sync framework.
 *
 * @param active if false disables all debug sync operations.
 *
 * @return 0 if framework initialized, non-zero otherwise.
 */
int ds_init(u_int32_t activation_flags);

/**
 * Release resources allocated for debug sync framework.
 */
void ds_destroy();

/**
 * Toggle the framework's status.
 *
 * @param active [dis]allows debug sync operations.
 */
void ds_activate(bool activate);

/**
 * Enable or disable an existing syn point.
 *
 * @param point_name name of the sync point.
 * @param enable enable or disable sync point's execution.
 *
 * @return 0 if the sync point's statue was successfully changed.
 */
int
ds_enable(const char *point_name, bool enable);

/**
 * Disable all known sync points.
 */
void ds_disable_all();

/**
 * Execute (pass through) a sync point.
 *
 * @param point_name name of the sync point.
 *
 * @return 0 if the sync point has been passed successfully, non-zero otherwise.
 */
int ds_exec(const char *point_name);

/**
 * Wait for a sync point (existing or a new one) to be reached.
 *
 * @param point_name name of the sync point.
 *
 * @return 0 if the sync point was successfully reached, non-zero otherwise.
 */
int ds_wait(const char *point_name);

/**
 * Attempt to unblock a sync point holding for a waiting block.
 *
 * @param point_name name of the sync point.
 *
 * @return 0 if an event to unblock has been raised successfully, non-zero otherwise.
 */
int ds_unblock(const char *point_name);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGSYNC_H_20120703 */
/* __EOF__ */

