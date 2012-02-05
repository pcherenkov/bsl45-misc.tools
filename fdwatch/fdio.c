/* @(#) basic I/O routines */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <assert.h>

#include "fdio.h"

static char trf_buf[(1024 * 10) + 1];
static const size_t TRFBUF_LIMIT = sizeof(trf_buf) - 1;

ssize_t
nwrite(int fd, const char* buf, size_t count, u_int32_t flags)
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
    if (err)
        errno = err;

    return nwr < 0 ? nwr : total;
}


ssize_t
nwritev(int fd, struct iovec *iov, int iovcnt, u_int32_t flags)
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
    if (err)
        errno = err;

    return err ? nbytes : total;
}


ssize_t
nread(int fd, char* buf, size_t count, u_int32_t flags)
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

    if (err)
        errno = err;

    return total;
}


ssize_t
npread(int fd, char* buf, size_t count, off_t offset, u_int32_t flags)
{
    ssize_t total = 0;
    int err = 0;

    while (count > (size_t)total) {
        ssize_t nrd = pread (fd, buf + total, count - (size_t)total, offset);
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
        offset += nrd;
    }

    if (err)
        errno = err;

    return total;
}



inline static ssize_t
bsd_nsendfile(int out_fd, int in_fd, off_t *offset, size_t count, int bsd_flags,
    u_int32_t flags)
{
    off_t start_pos =0, pos = 0, *eoffset = offset, sbytes = 0;
    int rc = 0, err = 0;
    ssize_t total = 0;

    if (NULL == eoffset) {
        if (-1 == (pos = lseek(in_fd, 0, SEEK_CUR))) {
            err = errno;
            perror("lseek [current]");
            errno = err;
            return -1;
        }
        start_pos = pos;
        eoffset = &pos;
    }

    if (*eoffset < 0) {
        errno = EINVAL;
        return -1;
    }


    while (count > (size_t)total) {
        rc = sendfile(in_fd, out_fd, *eoffset, count - (size_t)total,
                        NULL, &sbytes, bsd_flags);
        if (-1 == rc) {
            err = errno;

            if (EINTR == err) {
                if (0 == (flags & FDIO_QUIET_EINTR)) {
                    perror("sendfile");
                }
                errno = err = 0;
            }
            else {
                if ((flags & FDIO_QUIET_EAGAIN) &&
                    ((EWOULDBLOCK == err) || (EAGAIN == err)))
                    break;
                else if ((flags & FDIO_QUIET_EPIPE) && (EPIPE == err))
                    break;

                perror("sendfile");
                break;
            }
        }
        total += sbytes;
        *eoffset += sbytes;
    }

    if ((NULL == offset) && (total > 0)) {
        pos = lseek(in_fd, start_pos + total, SEEK_SET);
    }
    if (-1 == pos) {
        if (!err)
            err = errno;
        perror("lseek [set]");
    }

    if (offset)
        *offset = *eoffset + sbytes;

    if (err)
        errno = err;

    return sbytes;
}


ssize_t
nsendfile(int out_fd, int in_fd, off_t *offset, size_t count, int bsd_flags,
    u_int32_t flags)
{
#if defined(__linux__)
    (void)flags;
    return sendfile(out_fd, in_fd, offset, count);
#elif defined(__FreeBSD__)
    return bsd_nsendfile(out_fd, in_fd, offset, count, bsd_flags, flags);
#else
    #error "sendfile_compat: unsupported OS/API"
#endif
}


ssize_t
transfer(int out_fd, int in_fd, off_t *offset, size_t count,
    u_int32_t flags)
{
    int bsd_flags = 0, err = 0;
    size_t nbytes = count;
    ssize_t nrd = 0, nwr = 0, ntotal = 0;

    if (flags & FDIO_SENDFILE) {
#if defined(__FreeBSD__)
        if (flags & FDIO_SENDFILE_NOWAIT)
            bsd_flags |= (SF_NODISKIO | SF_MNOWAIT);
#endif
        return nsendfile(out_fd, in_fd, offset, count, bsd_flags, flags);
    }

    if (nbytes > TRFBUF_LIMIT)
        nbytes = TRFBUF_LIMIT;

    do {
        nrd = offset
            ? npread(in_fd, &trf_buf[0], nbytes, *offset, flags)
            : nread(in_fd, &trf_buf[0], nbytes, flags);
        err = errno;

        if (err)
            break;

        nwr = nwrite(out_fd, &trf_buf[0], nrd, flags);
        if (nwr == nrd) {
            ntotal = nwr;
        }
        else if (nwr < nrd) {
            ntotal = nwr;
            (void) fprintf(stderr, "%s: fd in/out=[%d/%d] underwrite: "
                "read/write=[%ld/%ld]", __func__, in_fd, out_fd,
                (long)nrd, (long)nwr);
        }
        else {
            (void) fprintf(stderr, "%s: panic: nread < nwrite [%ld/%ld]\n",
                __func__, (long)nrd, (long)nwr);
            abort();
        }
    } while (0);


    if (err || (ntotal <= 0)) {
        (void) fprintf(stderr, "%s: total=%ld, err=[%d:%s]\n",
            __func__, (long)ntotal, err, err ? strerror(err) : "");
    }

    if (offset)
        *offset += ntotal;

    if (err)
        errno = err;

    return ntotal;
}


/* __EOF__ */

