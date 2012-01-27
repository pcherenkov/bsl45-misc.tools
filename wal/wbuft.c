/* @(#) test module to alternate thru different writing techniques */
/* compile with gcc -pthread flag (*not* -lpthread, s'il vous plait) */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

/* enable O_DIRECT in fcntl.h */
#ifndef __USE_GNU
    #define __USE_GNU
#endif
#include <fcntl.h>

#include <pthread.h>

#ifdef __linux__
    #include <linux/falloc.h>
#endif

#include <assert.h>

static const size_t    MIN_BUFSZ   = 1024;
static const size_t    MAX_BUFSZ   = 1024*1024*512;
static const size_t    MIN_INCR    = 128;
static const size_t    MIN_NINCR   = 1;
static const size_t    MAX_NINCR   = 1024 * 1024;


enum { sQUIT=-1, sWAIT=0, sSYNC = 1 };
static struct sync_state {
     pthread_mutex_t    mtx;
     pthread_cond_t     cond;
     int                state;
     int                fd;
     u_int32_t          flags;
} g_st = {  PTHREAD_MUTEX_INITIALIZER,
            PTHREAD_COND_INITIALIZER,
            sWAIT, -1, 0};


enum {
    SYNC_TYPE   = 0x3,
    SYNC_ALL    = 1,
    SYNC_DATA   = 2,
    FALLOC      = (1 << 2),
    FALLOC_SZ   = (1 << 3),
    SYNC_THREAD = (1 << 4)
};

struct test_spec {
    u_int32_t   mode_flags;
    u_int32_t   open_flags;
    size_t      len;
    size_t      niter;
    size_t      incr;
    size_t      nincr;
    size_t      num_chunks;
};


typedef ssize_t (*wfunc_t) (int fd, const char* buf, size_t len,
                const struct test_spec* sp);


/**
 * functions
 */

static int
fd_sync (int fd, u_int32_t flags)
{
    switch (flags & SYNC_TYPE) {
        case 0:         return 0;
        case SYNC_ALL:  return fsync (fd);
        case SYNC_DATA: return fdatasync (fd);
        default:
            assert (0);
    }
    return EINVAL;
}


static int
toggle_sync_thread (int fd, u_int32_t flags, u_int32_t state)
{
    int rc = 0;

    rc = pthread_mutex_lock (&g_st.mtx);
    if (rc) {
        (void) fprintf (stderr, "\n%s: pthread_mutex_lock error [%d]\n",
            __func__, rc);
        return rc;
    }

        g_st.fd     = fd;
        g_st.flags  = flags;
        g_st.state  = state;

    rc = pthread_cond_signal (&g_st.cond);
    if (rc) {
        (void) fprintf (stderr, "\n%s: pthread_cond_signal error [%d]\n",
            __func__, rc);
    }

    rc = pthread_mutex_unlock (&g_st.mtx);
    if (rc) {
        (void) fprintf (stderr, "\n%s: pthread_mutex_unlock error [%d]\n",
            __func__, rc);
        return rc;
    }


    return rc;
}


static int
do_sync (int fd, u_int32_t flags)
{
    if (flags & SYNC_THREAD)
        return toggle_sync_thread (fd, flags, sSYNC);
    return fd_sync (fd, flags);
}


static void*
thread_run (void* arg)
{
    int rc = 0;
    long l = 0;
    size_t nsync = 0;

    (void) arg;

    (void) printf ("Thread %ld started\n", (long)pthread_self());
    rc = pthread_mutex_lock (&g_st.mtx);
    if (rc) {
        (void) fprintf (stderr, "\npthread_mutex_lock error [%d]\n", rc);
        return (void*)(l = rc);
    }

    do {
        while (sWAIT == g_st.state) {
            rc = pthread_cond_wait (&g_st.cond, &g_st.mtx);
        }
        if (sSYNC == g_st.state || sQUIT == g_st.state) {
            rc = fd_sync (g_st.fd, g_st.flags);
            if (rc) {
                (void) fprintf (stderr, "\n%s: fd_sync error %d\n",
                    __func__, rc);
                break;
            }
            ++nsync;
        }
        if (sQUIT == g_st.state)
            break;
        else
            g_st.state = sWAIT;
    } while (1);

    rc = pthread_mutex_unlock (&g_st.mtx);
    if (rc) {
        (void) fprintf (stderr, "\npthread_mutex_unlock error [%d]\n", rc);
        return (void*)(l = rc);
    }

    (void) printf ("%ld syncs, thread %ld is exiting\n",
                    (long)nsync, (long)pthread_self());
    return NULL;
}


static int
start_sync_thread (pthread_t* ptid)
{
    int rc = 0;

    assert (ptid);
    rc = pthread_create (ptid, NULL, &thread_run, NULL);
    if (rc) {
        (void) fprintf (stderr, "%s: error [%d] starting sync thread\n",
                        __func__, rc);
    }

    return rc;
}

static int
stop_sync_thread (pthread_t* ptid)
{
    int rc;
    void *retval = NULL;

    assert (ptid && *ptid);

    rc = toggle_sync_thread (-1, 0, sQUIT);
    if (rc) return rc;

    rc = pthread_join (*ptid, &retval);
    if (rc) {
        (void) fprintf (stderr, "%s: pthread_join error [%d]\n",
            __func__, rc);
        return rc;
    }

    (void) printf ("Thread %ld has exited with rc=%ld\n",
                    (long)*ptid, (long)retval);
    return rc;
}


static ssize_t
writev_nbuf (int fd, const char* buf, size_t len, const struct test_spec* sp)
{
    size_t i = 0, j = 0;
    ssize_t chunk_len = 0, nwr = -1, ntotal = 0, nleft = (ssize_t)len;
    int err = 0;
    struct iovec* iov = NULL;
    const char *p = buf;

    assert (buf && sp);
    assert ((sp->num_chunks > 1) && (sp->num_chunks < len));

    chunk_len = (ssize_t)(len / sp->num_chunks);
    assert (chunk_len >= 1);

    if (-1 == lseek (fd, 0, SEEK_SET)) {
        err = errno;
        perror ("lseek");
        return -err;
    }

    iov = calloc (sp->num_chunks, sizeof(iov[0]));
    if (!iov) {
        (void) fputs ("calloc for iov failed\n", stderr);
        return -1;
    }

    for (i = 0; (i < sp->num_chunks) && (nleft > 0); ++i) {
        iov[i].iov_base = (char*)p;
        iov[i].iov_len = ((i + 1) < sp->num_chunks)
                        ? (size_t)chunk_len : (size_t)nleft;

        p += iov[i].iov_len;
        nleft -= iov[i].iov_len;
    }
    assert (0 == nleft);

    for (j = 0; j < sp->niter; ++j) {
        nwr = writev (fd, iov, sp->num_chunks);
        if (nwr != (ssize_t)len) {
            err = errno;
            (void) fprintf (stderr, "%s: wrote %ld bytes out of %ld: err=%d [%s]\n",
                __func__, (long)nwr, len, err, (err ? strerror(err) : "none"));
            break;
        }

        if (sp->mode_flags & SYNC_TYPE) {
            do_sync (fd, sp->mode_flags);
        }
        ntotal += nwr;
    }

    free (iov);
    return !err ? ntotal : -err;
}


static ssize_t
write_nbuf (int fd, const char* buf, size_t len, const struct test_spec* sp)
{
    size_t i;
    ssize_t nwr = -1, ntotal = 0;
    int err = 0;

    assert (buf && sp);

    if (-1 == lseek (fd, 0, SEEK_SET)) {
        err = errno;
        perror ("lseek");
        return -err;
    }
    for (i = 0; i < sp->niter; ++i) {
        nwr = write (fd, buf, len);
        if (nwr != (ssize_t)len) {
            err = errno;
            (void) fprintf (stderr, "%s: wrote %ld bytes out of %ld: err=%d [%s]\n",
                __func__, (long)nwr, len, err, (err ? strerror(err) : "none"));
            break;
        }

        if (sp->mode_flags & SYNC_TYPE) {
            do_sync (fd, sp->mode_flags);
        }
        ntotal += nwr;
    }

    return !err ? ntotal : -err;
}


inline static double
get_seconds (const struct timeval* s, const struct timeval* e)
{
    return (e->tv_sec - s->tv_sec) + (e->tv_usec - s->tv_usec)*0.000001;
}


/*
    size_t blen, size_t niter, size_t blen_incr, size_t nincr)
*/
static int
run_odsync (const char* fname, const struct test_spec* sp)
{
    int fd = -1, err = 0, rc = 0;
    size_t len = 0, ni = 0, nfalloc = 0, j = 0;
    double sec = 0.0;
    int oflags = O_CREAT | O_RDWR;
    char *buf = NULL;
    struct timeval tstart, tend;
    ssize_t n = -1;
    pthread_t sync_thr = (pthread_t)0;
    wfunc_t write_data = NULL;

    assert (fname && sp);
    srand (time(NULL));

    write_data = (sp->num_chunks > 0) ? writev_nbuf : write_nbuf;

    oflags |= sp->open_flags;
    fd = open (fname, oflags, S_IRUSR|S_IWUSR);
    if (fd < 0) {
        err = errno;
        fprintf (stderr, "open [%s]: %s\n", fname, strerror(err));
        return err;
    }
    (void) printf ("BEGIN test: buf[%ld], niter=%ld, incr=%ld, nincr=%ld, "
            "num_chunks=[%ld] mode=0x%x\n",
            (long)sp->len, (long)sp->niter, (long)sp->incr, (long)sp->nincr,
            (long)sp->num_chunks, sp->mode_flags);
    (void) printf ("opened %s with flags=0x%04x\n", fname, oflags);

    if (sp->mode_flags & FALLOC) {
        nfalloc = (sp->len * sp->niter) + (sp->incr * (sp->nincr-1));
#ifdef __linux__
        rc = fallocate (fd, sp->mode_flags & FALLOC_SZ ? FALLOC_FL_KEEP_SIZE : 0, 0, nfalloc);
#else
        rc = posix_fallocate (fd, 0, nfalloc);
#endif
        if (rc) {
            err = errno;
            (void) fprintf (stderr, "fallocate [%ld bytes] for %s failed: %s\n",
                (long)nfalloc, fname, strerror (err));
        }
        else {
            (void) printf ("fallocate [%ld] for [%s]\n", (long)nfalloc, fname);
        }
    }

    if ((sp->mode_flags & SYNC_THREAD) && (sp->mode_flags & SYNC_TYPE)) {
        rc = start_sync_thread (&sync_thr);
    }

    for (len = sp->len, ni = 0; !rc && (len < MAX_BUFSZ) && (ni < sp->nincr);
            len += sp->incr, ++ni) {
        if (NULL == (buf = malloc (len))) {
            rc = ENOMEM;
            break;
        }
        for (j = 0; j < len; buf[j++] = rand () & 0xFF);

        (void) printf (" => buf[%ld bytes]: ", (long)len);
        gettimeofday (&tstart, 0);
            n = write_data (fd, buf, len, sp);
            if (n <= 0) {
                rc = (int)n;
                break;
            }
        gettimeofday (&tend, 0);
        sec = get_seconds (&tstart, &tend);
        (void) printf ("%ld/%ld bytes/writes in %.6f sec, %.6f w/sec, %.6f Kb/sec\n", (long)n,
                    (long)sp->niter, sec, (double)sp->niter/sec, ((double)n/sec)/1024);

        free (buf); buf = NULL;
    }

    if (buf) free (buf);
    (void) close (fd);

    if (sync_thr) {
        stop_sync_thread (&sync_thr);
    }

    (void) printf ("\nEND test, rc = %d\n", rc);
    return rc;
}


static void
usage_exit (const char* appname)
{
    (void) fprintf (stderr, "Usage: %s [options] filename buf_size num_iter \n",
                    appname);
    (void) fprintf (stderr, " Mode options: \n");
    (void) fprintf (stderr, "  -F fallocate() disk space\n");
    (void) fprintf (stderr, "  -k fallocate(FALLOC_FL_KEEP_SIZE)\n");
    (void) fprintf (stderr, " Increment options: \n");
    (void) fprintf (stderr, "  -I size =  increment buffer by <size>\n");
    (void) fprintf (stderr, "  -N count = conduct <count> increments\n");
    (void) fprintf (stderr, "  -U count = split each buffer into <count> chunks to writev(2)>\n");
    (void) fprintf (stderr, " open/write(2) options: \n");
    (void) fprintf (stderr, "  -i = O_DIRECT, -f = O_FSYNC, -d = O_DSYNC "
            "-s = O_SYNC -a = O_APPEND -t = O_TRUNC\n");
    (void) fprintf (stderr, "  -D = fdatasync() after each write\n");
    (void) fprintf (stderr, "  -S = fsync() after each write\n");
    (void) fprintf (stderr, "  -T = sync [-D|-S] in a thread\n");

    (void) fputc ('\n', stdout);
    exit (1);
}


static int
read_opt (int argc, char* const argv[], struct test_spec* sp)
{
    int opt = 0;
    long lval = -1;

    assert (argv && sp);

    (void) fputs ("Options: ", stdout);
    while (-1 != (opt = getopt (argc, argv, "ktfaidDSI:N:FTU:"))) {
        switch (opt) {
            case 'T':
                sp->mode_flags |= SYNC_THREAD;
                (void) fputs ("THREAD ", stdout);
                break;
            case 'F':
                sp->mode_flags |= FALLOC;
                (void) fputs ("fallocate() ", stdout);
                break;
            case 'k':
                sp->mode_flags |= FALLOC_SZ;
                (void) fputs ("fallocate(FALLOC_FL_KEEP_SIZE) ", stdout);
                break;
            case 't':
                sp->open_flags |= O_TRUNC;
                (void) fputs ("O_TRUNC ", stdout);
                break;
            case 'a':
                sp->open_flags |= O_APPEND;
                (void) fputs ("O_APPEND ", stdout);
                break;
            case 'i':
                sp->open_flags |= O_DIRECT;
                (void) fputs ("O_DIRECT ", stdout);
                break;
            case 's':
                sp->open_flags |= O_SYNC;
                (void) fputs ("O_SYNC ", stdout);
                break;
            case 'f':
                sp->open_flags |= O_FSYNC;
                (void) fputs ("O_FSYNC ", stdout);
                break;
            case 'd':
                sp->open_flags |= O_DSYNC;
                (void) fputs ("O_DSYNC ", stdout);
                break;
            case 'D':
                sp->mode_flags |= SYNC_DATA;
                (void) fputs ("fdatasync() ", stdout);
                break;
            case 'S':
                sp->mode_flags |= SYNC_DATA;
                (void) fputs ("fsync() ", stdout);
                break;
            case 'I':
                lval = atol (optarg);
                if (lval < (long)MIN_INCR || lval > (long)(MIN_INCR + MAX_NINCR * MIN_BUFSZ)) {
                    (void) fprintf (stderr, "\n%s: invalid incr value: %ld\n", argv[0], lval);
                    return EINVAL;
                }
                sp->incr = lval;
                (void) fprintf (stdout, "incr=%ld ", lval);
                break;
            case 'N':
                lval = atol (optarg);
                if (lval < (long)MIN_NINCR || lval > (long)MAX_NINCR) {
                    (void) fprintf (stderr, "\n%s: invalid nincr value: %ld\n", argv[0], lval);
                    return EINVAL;
                }
                sp->nincr = lval;
                (void) fprintf (stdout, "nincr=%ld ", lval);
                break;
            case 'U':
                lval = atol (optarg);
                if (lval < 2) {
                    (void) fprintf (stderr, "\n%s: invalid chunk count value: %ld\n", argv[0], lval);
                    return EINVAL;
                }
                sp->num_chunks = lval;
                (void) fprintf (stdout, "num_chunks=%ld ", lval);
                break;
            default:
                (void) fprintf (stderr, "\n%s: Invalid option: %c\n",
                                argv[0], (char)opt);
                return -1;
        }
    } /* while getopt */

    (void) fputc ('\n', stdout);
    return 0;
}



int main (int argc, char* const argv[])
{
    ssize_t bufsz = 0, niter = 0;
    const char *fname = NULL;
    int rc = 0;
    struct test_spec sp;

    static const int MIN_ARGC = 3;

    if (argc < MIN_ARGC) {
        usage_exit (argv[0]);
    }

    (void) memset (&sp, 0, sizeof(sp));
    rc = read_opt (argc, argv, &sp);
    if (rc) return EINVAL;
    /* uses getopt, exposes optind*/

    if (argc < (optind +  2)) { /* expecting at least 2 more arguments */
        (void) fprintf (stderr, "%s: expecting 2 mandatory parameters "
            "after option flags\n", argv[0]);
        usage_exit (argv[0]);
    }

    fname = argv[optind];
    ++optind;

    bufsz = (ssize_t) atol (argv[optind]);
    if ((bufsz < (ssize_t)MIN_BUFSZ) || (bufsz > (ssize_t)MAX_BUFSZ)) {
        (void) fprintf (stderr, "%s: buffer size [%s] must be within [%ld .. %ld] range\n",
            argv[0], argv[optind], (long)MIN_BUFSZ, (long)MAX_BUFSZ);
        return EINVAL;
    }
    sp.len = bufsz;
    ++optind;

    if (0 >= (niter = (ssize_t) atol (argv[optind]))) {
        (void) fprintf (stderr, "%s: invalid value for niter: %s\n",
            argv[0], argv[optind]);
        return EINVAL;
    }
    sp.niter = niter;
    ++optind;


    if (!sp.len)   sp.len     = MIN_BUFSZ;
    if (!sp.incr)  sp.incr    = MIN_INCR;
    if (!sp.nincr) sp.nincr   = MIN_NINCR;

    if (sp.num_chunks >= sp.len) {
        (void) fprintf (stderr, "%s: number of chunks (-U option) [%ld] "
            "should not exceed buffer size [%ld]\n", argv[0],
            (long)sp.num_chunks, (long)sp.len);
        return EINVAL;
    }

    setbuf (stdout, NULL);
    rc = run_odsync (fname, &sp);

    return rc;
}



/* __EOF__ */

