/* @(#) Operations on struct rusage (add, subtract, clear...) */

#ifndef RUSAGE_OPS_H
#define RUSAGE_OPS_H

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "nbsd_time.h"

#define rusage_clear(rp)                                        \
    do {                                                        \
        timerclear((rp)->ru_utime);                             \
        timerclear((rp)->ru_stime);                             \
        (rp)->ru_maxrss = (rp)->ru_ixrss = (rp)->ru_idrss =     \
        (rp)->ru_isrss = (rp)->ru_minflt = (rp)->ru_majflt =    \
        (rp)->ru_nswap = (rp)->ru_inblock = (rp)->ru_oublock =  \
        (rp)->ru_msgsnd = (rp)->ru_msgrcv = (rp)->ru_nsignals = \
        (rp)->ru_nvcsw = (rp)->ru_nivcsw = 0;                   \
    } while(0)

#define rusage_sub(rap, rbp, rrp)                               \
    do {                                                        \
        timersub(&((rap)->ru_utime), &((rbp->ru_utime), &((rrp)->ru_utime)));   \
        timersub(&((rap)->ru_stime), &((rbp->ru_stime), &((rrp)->ru_stime)));   \
        (rrp)->ru_maxrss = (rap)->ru_maxrss - (rbp)->ru_maxrss;                 \
        (rrp)->ru_ixrss = (rap)->ru_ixrss - (rbp)->ru_ixrss;                    \
        (rrp)->ru_idrss = (rap)->ru_idrss - (rbp)->ru_idrss;                    \
        (rrp)->ru_isrss = (rap)->ru_isrss - (rbp)->ru_isrss;                    \
        (rrp)->ru_minflt = (rap)->ru_minflt - (rbp)->ru_minflt;                 \
        (rrp)->ru_majflt = (rap)->ru_majflt - (rbp)->ru_majflt;                 \
        (rrp)->ru_nswap = (rap)->ru_nswap - (rbp)->ru_nswap;                    \
        (rrp)->ru_inblock = (rap)->ru_inblock - (rbp)->ru_inblock;              \
        (rrp)->ru_oublock = (rap)->ru_oublock - (rbp)->ru_oublock;              \
        (rrp)->ru_msgsnd = (rap)->ru_msgsnd - (rbp)->ru_msgsnd;                 \
        (rrp)->ru_msgrcv = (rap)->ru_msgrcv - (rbp)->ru_msgrcv;                 \
        (rrp)->ru_nsignals = (rap)->ru_nsignals - (rbp)->ru_nsignals;           \
        (rrp)->ru_nvcsw = (rap)->ru_nvcsw - (rbp)->ru_nvcsw;                    \
        (rrp)->ru_nicsw = (rap)->ru_nicsw - (rbp)->ru_nicsw;                    \
    } while(0)



#endif /* RUSAGE_OPS_H */

/* __EOF__ */

