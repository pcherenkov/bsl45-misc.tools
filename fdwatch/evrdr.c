/* @(#) read file contents as the file grows */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <assert.h>

#include "fdio.h"


#define TMOUT_SEC   5
#define TMOUT_NSEC  0

enum {f_REOPEN = 1};

static int g_kq;

static struct cliopt {
    int     print;
    time_t  tmout_sec;
    char    fpath[PATH_MAX + 1];
    int     fd;
} g_opt;


static int
kq_add(int kq, int fd)
{
    struct kevent ke[2];
    size_t i = 0;

    EV_SET(&ke[0], fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
        NOTE_DELETE|NOTE_RENAME, 0, NULL);

    EV_SET(&ke[1], fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

    for (i = 0; i < sizeof(ke)/sizeof(ke[0]); ++i) {
        if (-1 == kevent(kq, &ke[i], 1, NULL, 0, NULL)) {
            perror("kevent");
            return -1;
        }
    }

    return 0;
}


static int
re_open()
{
    int rc = 0;

    if (0 != (rc = close(g_opt.fd))) {
        perror("close [re-open]");
        return rc;
    }

    g_opt.fd = open(g_opt.fpath, O_CREAT|O_RDONLY|O_NONBLOCK,
        S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (-1 == g_opt.fd) {
        perror("open [re-open]");
        return -1;
    }

    rc = kq_add(g_kq, g_opt.fd);
    if (0 == rc) {
        (void) fprintf(stderr, "\nre-opened %s\n", g_opt.fpath);
    }

    return rc;
}


static int
process_event(const struct kevent *ke, u_int32_t *flags)
{
    off_t offset = 0;
    int fd = -1, rc = 0;
    char buf [1024 + 1] = "\0";
    size_t len = 0;
    ssize_t nrd = -1;

    assert(flags);

    if (EVFILT_READ == ke->filter) {
        offset = (off_t)ke->data;
        fd = (int)ke->ident;

        /*
        */

        if (offset > 0) {
            len = ((size_t)offset >= sizeof(buf))
                    ? sizeof(buf) - 1 : (size_t)offset;
            nrd = nread(fd, buf, len, 0);
            /*
            */
            if (nrd <= 0) {
                (void) fprintf(stderr, "EVFILT_READ: fd=%d, offset=%ld\n",
                    fd, (long)offset);
                (void) fprintf(stderr, "read %ld bytes from fd=%d\n", (long)nrd, fd);
                return -1;
            }
            fputc('.', stdout);
            if (g_opt.print) {
                buf[nrd] = '\0';
                (void) printf("=== BEGIN\n%s\n=== END\n", buf);
            }
        }
    }
    else if (EVFILT_VNODE == ke->filter) {
        /* (void) fputs("EVFILT_VNODE: ", stdout); */
        if (ke->fflags & NOTE_DELETE) {
            (void) fputs("NOTE_DELETE ", stdout);
            *flags |= f_REOPEN;
        }

        /*
        if (ke->fflags & NOTE_WRITE)
            (void) fputs("NOTE_WRITE ", stdout);
        if (ke->fflags & NOTE_EXTEND)
            (void) fputs("NOTE_EXTEND ", stdout);
        if (ke->fflags & NOTE_ATTRIB)
            (void) fputs("NOTE_ATTRIB ", stdout);
        if (ke->fflags & NOTE_LINK)
            (void) fputs("NOTE_LINK ", stdout);
        */

        if (ke->fflags & NOTE_RENAME) {
            (void) fputs("NOTE_RENAME ", stdout);
            *flags |= f_REOPEN;
        }

        /*
        if (ke->fflags & NOTE_REVOKE) {
            (void) fputs("NOTE_REVOKE ", stdout);
            rc = 1;
        }
        */

        (void) fputc('\n', stdout);
    }

    if (rc)
        (void) fprintf(stderr, "%s: rc=%d\n", __func__, rc);
    return rc;
}



static int
follow()
{
    int rc = 0, n = -1, i = -1;
    struct timespec tmout = {0, 0}, *ptmout = NULL;
    struct kevent ev[10];
    u_int32_t flags;

    g_kq = kqueue();
    if (-1 == g_kq) {
        perror("kqueue");
        return -1;
    }

    kq_add(g_kq, g_opt.fd);

    if (g_opt.tmout_sec > 0) {
        tmout.tv_sec = g_opt.tmout_sec;
        ptmout = &tmout;
    }

    (void) fputs("Entering watch loop\n", stdout);
    while (1) {
        (void) memset(&ev[0], 0, sizeof(ev));

        n = kevent(g_kq, NULL, 0, &ev[0], sizeof(ev)/sizeof(ev[0]), ptmout);
        if (n < 0) {
            perror ("kevent (loop)");
            break;
        }
        if (0 == n) {
            (void) fputs("\nkevent timed out\n", stderr);
            break;
        }

        /* (void) printf("got %d events:\n", n); */
        for (i = 0, flags = 0; i < n; ++i) {
            if (0 != (rc = process_event(&ev[i], &flags))) {
                (void) fprintf(stderr, "%s: i=%ld rc=%d\n",
                    __func__, (long)i,  rc);
                break;
            }
        }

        /*
        (void) fprintf(stderr, "%s: rc=%d\n", __func__, rc);
        */
        if (rc) break;

        if (flags & f_REOPEN) {
            rc = re_open();
            if (rc) break;
        }
    }

    (void) printf("Exiting watch loop, rc = %d\n", rc);
    return rc;
}


static void
usage_exit(const char* app, int rc)
{
    (void) fprintf(stderr, "Usage: %s [-p|--print] [-t timeout_sec] filename\n", app);
    (void) fprintf(stderr, "  -p|--print = print after reading\n");
    exit(rc);
}


int main(int argc, char* const argv[])
{
    int rc = 0, ch = 0, fd = -1;
    const char* fname = NULL;
    long lval = -1;

    static struct option lopts[] = {
        {"print",   no_argument,        NULL,   'p'},
        {"timeout", required_argument,  NULL,   't'},
        {NULL,      0,                  NULL,   0  }
    };

    if (argc < 2)
        usage_exit(argv[0], EINVAL);


    while (-1 != (ch = getopt_long(argc, argv, "pt:",
                    lopts, NULL))) {
        switch (ch) {
            case 'p':
                g_opt.print = 1;
                break;
            case 't':
                lval = atol(optarg);
                if (lval <= 0) {
                    (void) fprintf(stderr, "Invalid timeout value: %s\n",
                        optarg);
                    usage_exit(argv[0], EINVAL);
                }
                g_opt.tmout_sec = (time_t)lval;
                break;
            default:
                (void) fprintf(stderr, "%s: unknown option: %c\n",
                        argv[0], (char)ch);
                usage_exit(argv[0], EINVAL);
        }
    }

    if (optind >= argc)
        usage_exit(argv[0], EINVAL);

    fname = argv[optind];
    assert(fname);

    fd = open(fname, O_RDONLY|O_NONBLOCK);
    if (-1 == fd) {
        perror("open");
        return 1;
    }
    (void) strncpy(g_opt.fpath, fname, sizeof(g_opt.fpath));
    g_opt.fd = fd;

    setvbuf(stdout, NULL, _IONBF, 0);
    (void) printf("Following fd=%d [%s]\n", fd, fname);

    rc = follow();

    (void) close(g_opt.fd);

    return rc;
}


/* __EOF__ */

