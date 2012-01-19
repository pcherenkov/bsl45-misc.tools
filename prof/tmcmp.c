/* @(#) timer resolution tests */
/* gcc -W -Wall --pedantic -Wl,--no-as-needed -lrt -o tmcmp ./tmcmp.c */

#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <time.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


static int
clk_diff (FILE* out, const char* msg,
          clock_t sc, clock_t ec,
          const struct tms *st, const struct tms *et)
{
    static long CLK_TICKS = 0;
    clock_t cdiff = ec - sc;

    if (!CLK_TICKS) {
        CLK_TICKS = sysconf (_SC_CLK_TCK);
    }

    if (0 > CLK_TICKS) return errno;
    if (!out) return 0;

    (void) fprintf (out, "%s: time [sec] cpu_clk=%.6f", msg,
                    (double)cdiff / CLOCKS_PER_SEC);
    if (st && et) {
        (void) fprintf (out, " user=%.6f, sys=%.6f",
            ((double)(et->tms_utime - st->tms_utime)) / CLK_TICKS,
            ((double)(et->tms_stime - st->tms_stime)) / CLK_TICKS);
    }
    (void) fputc ('\n', out);

    return 0;
}


static double
gtm_diff (FILE* out, const char* msg, const struct timeval *tp,
          const struct timeval *etp)
{
    struct timeval res;

    assert (etp && tp);

    res.tv_sec = etp->tv_sec - tp->tv_sec;
    res.tv_usec = etp->tv_usec - tp->tv_usec;
    if (res.tv_usec < 0) {
        res.tv_sec--;
        res.tv_usec += 1000000;
    }

    (void) fprintf (out, "%s: %ld sec, %ld usec\n",
        (msg ? msg : "time taken"), (long)res.tv_sec, (long)res.tv_usec);

    return (res.tv_sec * 1000000.0) + res.tv_usec;
}


static double
ctm_diff (FILE* out, const char* msg, const struct timespec *tp,
          const struct timespec *etp)
{
    struct timespec res;
    const long BLN = 1000000000;

    assert (etp && tp);

    res.tv_sec = etp->tv_sec - tp->tv_sec;
    res.tv_nsec = etp->tv_nsec - tp->tv_nsec;
    if (res.tv_nsec < 0) {
        res.tv_sec--;
        res.tv_nsec += BLN;
    }

    (void) fprintf (out, "%s: %ld sec, %ld nsec\n",
        (msg ? msg : "time taken"), (long)res.tv_sec, (long)res.tv_nsec);

    return (res.tv_sec * (double)BLN) + res.tv_nsec;
}



int main (int argc, char* const argv[])
{
    struct tms  t1, t2;
    clock_t     clk[3] = {0,0,0};
    long niter = -1, i = 0;
    int rc = 0;
    enum { m_err = -1, m_gtm = 0, m_clk = 1, m_mnt = 2, m_time = 3} mode = m_err;
    struct timeval stv, tv;
    struct timespec tsp, ssp;

    if (argc < 3) {
        (void) fprintf (stderr, "Usage: %s clk|gtm|mnt|time niter\n", argv[0]);
        return EINVAL;
    }

    if (0 == strcasecmp ("gtm", argv[1])) {
        mode = m_gtm;
    } else if (0 == strcasecmp ("clk", argv[1])) {
        mode = m_clk;
    } else if (0 == strcasecmp ("mnt", argv[1])) {
        mode = m_mnt;
    } else if (0 == strcasecmp ("time", argv[1])) {
        mode = m_time;
    }
    else {
        (void) fprintf (stderr, "Invalid mode: %s\n", argv[1]);
        return EINVAL;
    }

    if (0 >= (niter = atol (argv[2]))) {
        (void) fprintf (stderr, "Invalid niter: [%s] (must be > 0)\n",
            argv[1]);
        return EINVAL;
    }

    if (0 != (rc = clk_diff (NULL, "", 0, 0, NULL, NULL))) {
        (void) fprintf (stderr, "sysconf (_SC_CLK_TCK) error [%d]: %s\n",
            rc, strerror(rc));
        return rc;
    }

    (void) printf ("%s: %ld iterations\n", argv[1], niter);

    (void) gettimeofday (&stv, NULL);
    (void) clock_gettime (CLOCK_MONOTONIC, &ssp);

    if ((-1 == (clk[0] = clock())) || (-1 == times (&t1))) {
        perror ("times 0"); return 1;
    }

    for (i = 0; i < niter; ++i) {
        switch (mode) {
            case m_gtm:
                (void) gettimeofday (&tv, NULL); break;
            case m_clk:
                clk[1] = clock (); break;
            case m_mnt:
                (void) clock_gettime (CLOCK_MONOTONIC, &tsp);
                break;
            case m_time:
                (void) time (NULL);
                break;
            default:
                assert (0);
        }
    }

    if ((-1 == (clk[2] = clock())) || (-1 == times (&t2))) {
        perror ("times 1"); return 1;
    }
    switch (mode) {
        case m_gtm: (void) gtm_diff (stdout, "gtm time diff", &stv, &tv);
                    break;
        case m_mnt: (void) ctm_diff (stdout, "ctm time", &ssp, &tsp);
                    break;
        default:    break;
    }

    rc = clk_diff (stdout, "clock calls", clk[0], clk[2], &t1, &t2);

    (void) printf ("%s: exiting with rc=%d\n", argv[0], rc);
    return rc;
}


/* __EOF__ */

