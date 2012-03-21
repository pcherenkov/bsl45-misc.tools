/* @(#) performance measurement utilities */

#ifndef PRFM_H_20120321
#define PRFM_H_20120321

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <stdio.h>
#include <time.h>


struct tmrec {
    struct timespec     tm;
    struct rusage       ru;
};

struct prfm {
    int                 who;
    int32_t             flags;
    struct tmrec        last,
                        sum;
};
enum {
    prfm_TIME    = 1,
    prfm_RUSAGE  = (1 << 1)
};

#ifdef __cplusplus
extern "C" {
#endif

void
prfm_init(struct prfm *r, int32_t flags, int rusage_who);

void
prfm_start(struct prfm *r);

void
prfm_stop(struct prfm *r);

void
prfm_dump(FILE *fp, const struct prfm *r, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* PRFM_H_20120321 */

/* __EOF__ */

