/* @(#) basic fd-based I/O */

#ifndef FDIO_03022012
#define FDIO_03022012

#ifdef __cplusplus
extern "C" {
#endif

enum {
    FDIO_QUIET_EAGAIN   = 1,
    FDIO_QUIET_EPIPE    = (1 << 1),
    FDIO_QUIET_EINTR    = (1 << 2),
    FDIO_SENDFILE       = (1 << 3),
    FDIO_SENDFILE_NOWAIT= (1 << 4),
    FDIO_SRC_LOOP       = (1 << 5)
};


ssize_t
nwrite(int fd, const char* buf, size_t count, u_int32_t flags);


ssize_t
nwritev(int fd, struct iovec *iov, int iovcnt, u_int32_t flags);


ssize_t
nread(int fd, char* buf, size_t count, u_int32_t flags);


ssize_t
npread(int fd, char* buf, size_t count, off_t offset, u_int32_t flags);


ssize_t
nsendfile(int out_fd, int in_fd, off_t *offset, size_t count,
            int bsd_flags, u_int32_t flags);


ssize_t
transfer(int out_fd, int in_fd, off_t *offset, size_t count,
    u_int32_t flags);


#ifdef __cplusplus
}
#endif

#endif /* FDIO_03022012 */

/* __EOF__ */

