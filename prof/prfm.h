/* @(#) performance measurement utilities */

#ifndef PRFM_H_20120321
#define PRFM_H_20120321

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <stdio.h>
#include <time.h>


/* Metrics used to measure performance. */
struct prfm_metrics {
    struct timespec     tm;     /* Time spent.      */
    struct rusage       ru;     /* Resource usage.  */ 
};


/* Container for performance-monitoring data. */
struct prfm {
    int                 who;    /* getrusage(2) who parameter. */
    int32_t             flags;  /* Content & behavior flags (see below). */
    struct prfm_metrics start,  /* Starting-point state/metrics. */
                        accum;  /* Accumulated state/metrics. */
};

/* Flags regulating available metrics. */
enum {
    prfm_TIME    = 1,           /* Measure time passed. */
    prfm_RUSAGE  = (1 << 1)     /* Measure resource usage. */
};


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize resources, set flags & options.
 * 
 * @param r             performance probe.
 * @param flags         metrics & behavior flags.
 * @param rusage_who    target/who for getrusage(2).   
 */
void
prfm_xinit(struct prfm *r, int32_t flags, int rusage_who);


/* Macros to simplify initialization of performance probes:
 *
 * prfm_init          self-probe of process scope, measure time & resources;
 * prfm_tm_init       same as above, but measure resources only (no time);
 * prfm_init_thr      self probe of (current) thread scope, time & resources
 */
#define prfm_init(r)        prfm_xinit(r, prfm_TIME|prfm_RUSAGE, RUSAGE_SELF)
#define prfm_tm_init(r)     prfm_xinit(r, prfm_TIME, RUSAGE_SELF)
#define prfm_init_thr(r)    prfm_xinit(r, prfm_TIME|prfm_RUSAGE, RUSAGE_THREAD)


/**
 * Release performance probe's resources.
 *
 * @param r   performance probe.
 */
void
prfm_destroy(struct prfm *r);


/**
 * Start probing iteration: capture metrics.
 *
 * @param r   performance probe.
 */
void
prfm_start(struct prfm *r);

/**
 * Stop (complete) probing for the (previously started) iteration; 
 *
 * @param r   performance probe.
 */
void
prfm_stop(struct prfm *r);

void
prfm_probe(struct prfm *r);

void
prfm_dump(FILE *fp, const struct prfm *r, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* PRFM_H_20120321 */

/* __EOF__ */

