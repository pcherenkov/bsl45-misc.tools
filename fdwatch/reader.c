/* @(#) read file contents as the file grows */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/event.h>
#include <sys/time.h>

#include <unistd.h>

#define TMOUT_SEC   5
#define TMOUT_NSEC  0

static ssize_t
nread (int fd, void* buf, size_t count)
{
    ssize_t total = 0;

    while (count > (size_t)total) {
        ssize_t nrd = read (fd, buf + total, count - (size_t)total);
        if (-1 == nrd) {
            if (EINTR == errno) {
                errno = 0;
                continue;
            } else {
                say_syserror ("read");
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
follow (int fd)
{
    int rc = 0, kq = -1, n = -1;
    struct timespec tmout = { TMOUT_SEC, TMOUT_NSEC };
    struct kevent ke, ev[2];

    kq = kqueue();
    if (-1 == kq) {
        perror("kqueue");
        return -1;
    }

    EV_SET(&ke, fd, EVFILT_VNODE, EV_ADD,
        NOTE_DELETE|NOTE_EXTEND|NOTE_RENAME|
        NOTE_REVOKE, 0, NULL);

    if (-1 == kevent(kq, &ke, 1, NULL, 0, NULL)) {
        perror("kevent EVFILT_VNODE");
        return -1;
    }

    EV_SET(&ev, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

    if (-1 == kevent(kq, &ke, 1, NULL, 0, NULL)) {
        perror("kevent EVFILT_READ");
        return -1;
    }

    while (1) {
        (void) memset(&chg[0], 0, sizeof(chg));

        n = kevent(kq, NULL, 0, &ev[0], sizeof(ev)/sizeof(ev[0]), &tmout);
        if (n < 0) {
            perror ("kevent (loop)");
            break;
        }
        if (0 == n) {
            (void) fputs ("\nkevent timed out\n", stderr);
            break;
        }

        /* TODO: event processing */
    }


    return rc;
}


/* __EOF__ */

