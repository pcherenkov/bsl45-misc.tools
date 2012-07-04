/* @(#) debugsync facility interface */

#ifndef DEBUGSYNC_H_20120703
#define DEBUGSYNC_H_20120703

#include <stdbool.h>

enum {
	/* Maximum length of a sync point's name. */
	DS_MAX_POINT_NAME_LEN = 32,
	/* Maximum number of sync points allowed. */
	DS_MAX_POINT_COUNT  = 256
};


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
 * Create a sync point or set status of an existing one.
 *
 * @param point_name name of the sync point.
 * @param enable enable or disable the sync point.
 *
 */
int ds_set(const char *point_name, bool enable, const char *origin);

int ds_wait(const char *point_name, const char *origin);

int ds_signal(const char *point_name, const char *origin);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGSYNC_H_20120703 */
/* __EOF__ */

