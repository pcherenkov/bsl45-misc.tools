/* @(#) write to N files from a given source */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include "fdio.h"


enum {
    trf_SOCK_DST =  1,
    trf_LOOP_SRC =  (1 << 1)
};

static char trf_buf[1024 * 4 + 1];
static const size_t TRFBUF_LIMIT = sizeof(trf_buf) - 1;


/* implementation
 */

/* TODO: move to fdio.c */

ssize_t
transfer(int in_fd, int out_fd, off_t *in_offset, size_t nbytes,
     int* error, u_int32_t *flags)
{
    int rc = 0, err = 0;
    ssize_t nrd = -1, nwr = -1, ntotal = -1;
    off_t offset = 0;
            /* TODO: move to nsendfile */
    /*
    static const int SF_FLAGS = SF_NODISKIO | SF_MNOWAIT;
    */
    u_int32_t nwr_flags = FDIO_QUIET_EAGAIN;

    assert(flags);
    assert(!in_offset || (*in_offset >= 0));

    do {
        if (*flags & trf_SOCK_DST) {
            /* TODO: implement */
            ntotal = nsendfile(out_fd, in_fd, in_offset, nbytes, &err);
            if (0 == err)
                break;
            else {
                perror ("sendfile");
                rc = -1;
                if (ntotal || !can_rdwr(err, in_offset))
                    break;
                (void) fprintf(stderr, "%s: trying read(2)/write(2)\n",
                    __func__);
            }
        }

        if (nbytes > TRFBUF_LIMIT)
            nbytes = TRFBUF_LIMIT;

            /* TODO: implement */
        nrd = nread_at(in_fd, in_offset, &tr_buf[0], nbytes, &err);

        if ((0 == nrd) && !err && (*flags & trf_LOOP_SRC)) {
            *in_offset = 0;
            /* TODO: implement */
            nrd = nread_at(in_fd, in_offset, &tr_buf[0], nbytes, &err);
        }

        if((nrd <= 0) || err)
            break;

        if (*flags & trf_SOCK_DST)
            nwr_flags |= FDIO_QUIET_EPIPE;

        nwr = nwrite_(out_fd, &tr_buf[0], nrd, &err, nwr_flags);
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

        if (*flags & trf_SOCK_DST) {
            *flags &= ~trf_SOCK_DST;
            (void) fprintf(stderr, "%s: socket mode dropped for out_fd=%d\n",
                __func__, out_fd);
        }
    } while (0);


    if (err || (ntotal <= 0)) {
        (void) fprintf(stderr, "%s: total=%ld, err=[%d:%s]\n",
            __func__, (long)ntotal, err, err ? strerror(err) : "");
    }

    if (offset)
        *offset += ntotal;

    if (error)
        *error = err;

    return ntotal;
}


/* __EOF__ */

