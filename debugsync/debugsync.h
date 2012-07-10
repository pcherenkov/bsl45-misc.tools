/* @(#) debugsync facility interface */

/****************************************************************
 * Debug synchronization facility provides the capability to
 * 'insert' synchronization points into multi-threaded code.
 *
 * A a synchronization gets 'inserted' before a code
 * section one wants to test (in thread A). In order to 
 * put the system into a condition suitable for the test, 
 * other threads may need to execute their test-specific
 * code sections right before the tested sections executes
 * (in thread A).
 *
 * TODO: terminology and algorithm.
 *
 *
 *
 ****************************************************************/


#ifndef DEBUGSYNC_H_20120703
#define DEBUGSYNC_H_20120703

#include <sys/types.h>
#include <stdbool.h>

#ifdef NDEBUG

#	define  DSYNC_ACTIVATE(activate)
#	define	DSYNC_SET(name)
#	define  DSYNC_ENABLE(name, enable)
#	define	DSYNC_WAIT(name)
#	define	DSYNC_UNBLOCK(name)

#else

#	define  DSYNC_ACTIVATE(activate)	ds_activate(activate)
#	define	DSYNC_SET(name)			ds_exec(name)
#	define  DSYNC_ENABLE(name, enable)	ds_enable(name, enable)
#	define	DSYNC_WAIT(name)		ds_wait(name)
#	define	DSYNC_UNBLOCK(name)		ds_unblock(name)

#endif


/** Debug sync activation flags. */
enum {
	/** Enable framework functionality. */
	DS_ACTIVE 		= 1,
	/** On-the-fly activation is propagated across threads. */
	DS_GLOBAL		= (1 << 1)
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

