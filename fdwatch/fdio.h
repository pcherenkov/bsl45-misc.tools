/* @(#) basic fd-based I/O */

#ifndef FDIO_03022012
#define FDIO_03022012

#ifdef __cplusplus
extern "C" {
#endif

enum {
    FDIO_QUIET_BLK  = 1,
    FDIO_QUIET_PIPE = (1 << 1)
};


ssize_t
nwrite_(int fd, const char* buf, size_t count, int* error, u_int32_t flags);


inline static ssize_t
nwrite(int fd, const char* buf, size_t count, int* error)
{
    return nwrite_(fd, buf, count, error, 0);
}


inline static ssize_t
nwrite_sock(int fd, const char* buf, size_t count, int* error)
{
    return nwrite_(fd, buf, count, error, FDIO_QUIET_BLK | FDIO_QUIET_PIPE);
}


ssize_t
nwritev_(int fd, struct iovec *iov, int iovcnt, int* error, u_int32_t flags);


inline static ssize_t
nwritev_sock(int fd, struct iovec *iov, int iovcnt, int* error)
{
    return nwritev_(fd, iov, iovcnt, error, FDIO_QUIET_BLK | FDIO_QUIET_PIPE);
}


inline static ssize_t
nwritev(int fd, struct iovec *iov, int iovcnt, int* error)
{
    return nwritev_(fd, iov, iovcnt, error, 0);
}


ssize_t
nread_(int fd, char* buf, size_t count, int* error, u_int32_t flags);


inline static ssize_t
nread_sock(int fd, char* buf, size_t count, int* error)
{
    return nread_(fd, buf, count, error, FDIO_QUIET_BLK);
}


#ifdef __cplusplus
}
#endif

#endif /* FDIO_03022012 */

/* __EOF__ */

