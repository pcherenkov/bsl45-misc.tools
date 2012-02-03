/* @(#) basic I/O routines */

#include <sys/types.h>
#include <sys/uio.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "fdio.h"

ssize_t
nwrite_(int fd, const char* buf, size_t count, int* error, u_int32_t flags)
{
    int err = 0;
    ssize_t total = 0, nwr = 0;

    while (count > (size_t)total) {
        nwr = write (fd, buf + total, count - (size_t)total);
        if (nwr <= 0) {
            err = errno;
            if (EINTR == err) {
                if (0 == (flags & FDIO_QUIET_EINTR)) {
                    perror("writev");
                }
                errno = err = 0;
                continue;
            }
            if ((flags & FDIO_QUIET_EAGAIN) &&
                ((EWOULDBLOCK == err) || (EAGAIN == err)))
                break;
            else if ((flags & FDIO_QUIET_EPIPE) && (EPIPE == err))
                break;

            perror("writev");
            break;
        }

        total += nwr;
    }
    if (error)
        *error = err;

    return nwr < 0 ? nwr : total;
}



ssize_t
nwritev_(int fd, struct iovec *iov, int iovcnt, int* error, u_int32_t flags)
{
    int err = 0;
    ssize_t nbytes = 0, total = 0;
    size_t underwr = 0;

    assert(iov && (iovcnt > 0));

    while (iovcnt > 0) {
        nbytes = writev(fd, iov, iovcnt);
        if (nbytes <= 0) {
            err = errno;
            if (EINTR == err) {
                if (0 == (flags & FDIO_QUIET_EINTR)) {
                    perror("writev");
                }
                errno = err = 0;
                nbytes = 0;
            }
            else {
                if ((flags & FDIO_QUIET_EAGAIN) &&
                    ((EWOULDBLOCK == err) || (EAGAIN == err)))
                    break;
                else if ((flags & FDIO_QUIET_EPIPE) && (EPIPE == err))
                    break;

                perror("writev");
                break;
            }
        }
        total += nbytes;

        for (; iovcnt > 0; ++iov, --iovcnt) {
            if (nbytes < (ssize_t)iov->iov_len) {
                iov->iov_base = (char*)iov->iov_base + nbytes;
                iov->iov_len -= nbytes;
                break;
            }
            nbytes -= iov->iov_len;
        }
    }

    if (nbytes < 0) {
        for (; iovcnt > 0; ++iov, --iovcnt)
            underwr += iov->iov_len;
        if (underwr)
            (void) fprintf(stderr, "writev: underwrote %ld bytes\n",
                (long)underwr);
    }
    if (error)
        *error = err;

    return err ? nbytes : total;
}


ssize_t
nread_(int fd, char* buf, size_t count, int* error, u_int32_t flags)
{
    ssize_t total = 0;
    int err = 0;

    while (count > (size_t)total) {
        ssize_t nrd = read (fd, buf + total, count - (size_t)total);
        if (-1 == nrd) {
            err = errno;
            if (EINTR == err) {
                if (0 == (flags & FDIO_QUIET_EINTR)) {
                    perror("writev");
                }
                err = errno = 0;
                continue;
            }
            if ((flags & FDIO_QUIET_EAGAIN) &&
                ((EWOULDBLOCK == err) || (EAGAIN == err)))
                    break;
            perror ("read");
            break;
        }
        else if (0 == nrd) {
            return total;
        }

        total += nrd;
    }

    if (error)
        *error = err;

    return total;
}



/* __EOF__ */

