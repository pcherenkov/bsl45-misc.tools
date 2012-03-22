/* @(#) profiling API */

#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "nbsd_time.h"
#include "rusage_ops.h"

#include "prfm.h"


static void
capture(struct prfm_metrics *t, int32_t flags, int who)
{
    int rc = 0;
    
    if (flags & prfm_TIME) {
        rc = clock_gettime(CLOCK_MONOTONIC, &t->tm);
#ifndef NDEBUG
        if (rc) {
            perror("clock_gettime");
            abort();
        }
#endif
	}

    if (flags & prfm_RUSAGE) {
        rc = getrusage(who, &t->ru);
#ifndef NDEBUG
        if (rc) {
            perror("getrusage");
            abort();
        }
#endif
	}
}


static void
reset(struct prfm *r)
{
    if (r->flags & prfm_TIME) {
        timespecclear(&r->accum.tm);
        timespecclear(&r->start.tm);
    }


    if (r->flags & prfm_RUSAGE) {
        rusage_clear(&r->accum.ru);
        rusage_clear(&r->start.ru);
    }
}


void
prfm_xinit(struct prfm *r, int32_t flags, int rusage_who)
{
	r->flags = flags;
    reset(r);
    r->who = rusage_who;
}


void
prfm_destroy(struct prfm *r)
{
    reset(r);
    r->flags = 0;
}


void
prfm_start(struct prfm *r)
{
    capture(&r->start, r->flags, r->who);
}


void
prfm_stop(struct prfm *r)
{
    struct prfm_metrics curr;

    capture(&curr, r->flags, r->who);

    if (r->flags & prfm_TIME) {
        timespecsub(&curr.tm, &r->start.tm, &curr.tm);
        timespecadd(&curr.tm, &r->accum.tm, &r->accum.tm);
    }

    if (r->flags & prfm_RUSAGE) {
        rusage_sub(&curr.ru, &r->start.ru, &curr.ru);
        rusage_add(&curr.ru, &r->accum.ru, &r->accum.ru);
    }
}


void
prfm_probe(struct prfm *r)
{
    capture(&r->accum, r->flags, r->who);
    timespecclear(&r->accum.tm);
}


static void
rusage_dump(FILE *fp, const struct rusage *ru)
{
    (void) fprintf(fp, "== resource usage:\n");
    (void) fprintf(fp, "  user_time: %ld.%06ld, sys_time: %ld.%06ld\n",
        ru->ru_utime.tv_sec, ru->ru_utime.tv_usec,
        ru->ru_stime.tv_sec, ru->ru_stime.tv_usec);
    (void) fprintf(fp, "  maxrss: %ld, ixrss: %ld, idrss: %ld, isrss: %ld\n",
        ru->ru_maxrss, ru->ru_ixrss, ru->ru_idrss, ru->ru_isrss);
    (void) fprintf(fp, "  minflt: %ld, majflt: %ld, nswap=%ld\n",
        ru->ru_minflt, ru->ru_majflt, ru->ru_nswap);
    (void) fprintf(fp, "  inblock: %ld, outblock: %ld, msgsnd: %ld, msgrcv: %ld\n",
        ru->ru_inblock, ru->ru_oublock, ru->ru_msgsnd, ru->ru_msgrcv);

    (void) fprintf(fp, "  nsignals: %ld, nvcsw: %ld, nivcsw: %ld\n",
        ru->ru_nsignals, ru->ru_nvcsw, ru->ru_nivcsw);

    (void) fputs("==\n", fp);
}


static void
tmspec_dump(FILE* fp, const struct timespec *tms)
{
    (void) fprintf(fp, "== time:\t%ld.%09ld\n",
            tms->tv_sec, tms->tv_nsec);
}


void
prfm_dump(FILE *fp, const struct prfm *r, const char* fmt, ...)
{
    va_list ap;
    const struct rusage *ru = &r->accum.ru;

    if (fmt) {
	    va_start(ap, fmt);
            vfprintf(fp, fmt, ap);
	    va_end(ap);
    }

    if (r->flags & prfm_TIME)
    	tmspec_dump(fp, &r->accum.tm);

    if (r->flags & prfm_RUSAGE)
    	rusage_dump(fp, ru);
}


/* __EOF__ */

