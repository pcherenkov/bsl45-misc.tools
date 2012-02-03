/* @(#) write to N files from a given source */

#include <sys/types.h>

#include <stdio.h>
#include <errno.h>

ssize_t
nwrite (int fd, const void* buf, size_t count)
{
    int err = 0;
    ssize_t total = 0, nwr = 0;

    while (count > (size_t)total) {
        nwr = write (fd, buf + total, count - (size_t)total);
        if (nwr <= 0) {
            err = errno;
            if (EINTR == err) {
                errno = 0;
                continue;
            }
            if (err && (EWOULDBLOCK != err) && (EAGAIN != err)) {
                perror ("write");
            }
            break;
        }

        total += nwr;
    }

    return nwr < 0 ? nwr : total;
}





/* __EOF__ */

