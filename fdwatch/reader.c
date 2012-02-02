/* @(#) read file contents as the file grows */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/event.h>
#include <sys/time.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#define TMOUT_SEC   5
#define TMOUT_NSEC  0


static struct cliopt {
    int     print;
    time_t  tmout_sec;
} g_opt;


static ssize_t
nread (int fd, char* buf, size_t count)
{
    ssize_t total = 0;

    while (count > (size_t)total) {
        ssize_t nrd = read (fd, buf + total, count - (size_t)total);
        if (-1 == nrd) {
            if (EINTR == errno) {
                errno = 0;
                continue;
            } else {
                perror ("read");
                return nrd;
            }
        }
        else if (0 == nrd) {
            return total;
        }

        total += nrd;
    }

    return total;
}


static int
process_event(const struct kevent *ke)
{
    off_t offset = 0;
    int fd = -1, rc = 0;
    char buf [1024 + 1] = "\0";
    size_t len = 0;
    ssize_t nrd = -1;

    if (EVFILT_READ == ke->filter) {
        offset = (off_t)ke->data;
        fd = (int)ke->ident;

        (void) printf("EVFILT_READ: fd=%d, offset=%ld\n",
                fd, (long)offset);

        if (offset > 0) {
            len = ((size_t)offset >= sizeof(buf))
                    ? sizeof(buf) - 1 : (size_t)offset;
            nrd = nread(fd, buf, len);
            (void) printf("read %ld bytes from fd=%d\n", (long)nrd, fd);
            if (nrd <= 0)
                return -1;
            if (g_opt.print) {
                buf[nrd] = '\0';
                (void) printf("=== BEGIN\n%s\n=== END\n", buf);
            }
        }
    }
    else if (EVFILT_VNODE == ke->filter) {
        (void) fputs("EVFILT_VNODE: ", stdout);
        if (ke->fflags & NOTE_DELETE) {
            (void) fputs("NOTE_DELETE ", stdout);
            rc = 1;
        }
        if (ke->fflags & NOTE_WRITE)
            (void) fputs("NOTE_WRITE ", stdout);
        if (ke->fflags & NOTE_EXTEND)
            (void) fputs("NOTE_EXTEND ", stdout);
        if (ke->fflags & NOTE_ATTRIB)
            (void) fputs("NOTE_ATTRIB ", stdout);
        if (ke->fflags & NOTE_LINK)
            (void) fputs("NOTE_LINK ", stdout);
        if (ke->fflags & NOTE_RENAME)
            (void) fputs("NOTE_RENAME ", stdout);
        if (ke->fflags & NOTE_REVOKE) {
            (void) fputs("NOTE_REVOKE ", stdout);
            rc = 1;
        }
        (void) fputc('\n', stdout);
    }

    return rc;
}


static int
follow (int fd)
{
    int rc = 0, kq = -1, n = -1, i = -1;
    struct timespec tmout = {0, 0}, *ptmout = NULL;
    struct kevent ke, ev[10];

    kq = kqueue();
    if (-1 == kq) {
        perror("kqueue");
        return -1;
    }

    /* NB: use EV_CLEAR with EV_ADD or events will loop */
    EV_SET(&ke, fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
        NOTE_DELETE|NOTE_RENAME|NOTE_EXTEND|
        NOTE_REVOKE, 0, NULL);

    if (-1 == kevent(kq, &ke, 1, NULL, 0, NULL)) {
        perror("kevent EVFILT_VNODE");
        return -1;
    }

    EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

    if (-1 == kevent(kq, &ke, 1, NULL, 0, NULL)) {
        perror("kevent EVFILT_READ");
        return -1;
    }

    if (g_opt.tmout_sec > 0) {
        tmout.tv_sec = g_opt.tmout_sec;
        ptmout = &tmout;
    }

    (void) fputs("Entering watch loop\n", stdout);
    while (1) {
        (void) memset(&ev[0], 0, sizeof(ev));

        n = kevent(kq, NULL, 0, &ev[0], sizeof(ev)/sizeof(ev[0]), ptmout);
        if (n < 0) {
            perror ("kevent (loop)");
            break;
        }
        if (0 == n) {
            (void) fputs("\nkevent timed out\n", stderr);
            break;
        }

        (void) printf("got %d events:\n", n);
        for (i = 0; i < n; ++i) {
            if (0 != (rc = process_event(&ev[i])))
                break;
        }

        if (rc) break;
    }

    (void) printf("Exiting watch loop, rc = %d\n", rc);
    return rc;
}


static void
usage_exit(const char* app, int rc)
{
    (void) fprintf(stderr, "Usage: %s [-p|--print] filename\n", app);
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

    setvbuf(stdout, NULL, _IONBF, 0);
    (void) printf("Following fd=%d [%s]\n", fd, fname);

    rc = follow(fd);

    (void) close(fd);

    return rc;
}


/* __EOF__ */

