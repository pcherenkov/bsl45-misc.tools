/* @(#) write to N files from a given source */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <signal.h>

#include <assert.h>

#include "fdio.h"

static size_t MTU_DEFAULT = 1500;
static volatile sig_atomic_t g_quit = 0;

static int
rollover(const char* fpath, int* fd, size_t counter)
{
    int n = 0;

    static char newpath[PATH_MAX] = "\0";

    assert(fpath && fd && counter);

    n = snprintf(newpath, sizeof(newpath)-1, "%s.%06lu", fpath, counter);
    assert ((size_t)n < sizeof(newpath));
    newpath[sizeof(newpath)-1] = '\0';

    if (-1 == rename(fpath, newpath)) {
        perror("rename");
        return -1;
    }

    if (-1 == close(*fd)) {
        perror("close");
        return -1;
    }

    if (-1 == unlink(newpath)) {
        perror("unlink");
        return -1;
    }

    *fd = open(fpath, O_WRONLY|O_CREAT|O_TRUNC);
    if (-1 == (*fd)) {
        perror("open [roll]");
        return -1;
    }

    return 0;
}


static int
cyclic_file_relay(const char* src_path, const char* dst_path, long ms_delay)
{
    int in_fd = -1, out_fd = -1, err = 0;
    u_int32_t flags = 0;
    ssize_t ntotal = 0, nsent = 0;
    off_t start = 0;
    size_t chunk_len = MTU_DEFAULT, nrolls = 0;
    struct timespec tms;

    static const size_t MAX_DST_LENGTH = (1024 * 1024) * 20;
    static const long NSEC_IN_MS = 1000000;

    tms.tv_sec  = 0;
    tms.tv_nsec = ms_delay * NSEC_IN_MS;

    in_fd = open(src_path, O_RDONLY);
    if (-1 == in_fd) {
        perror("open [src]");
        return -1;
    }

    out_fd = open(dst_path, O_WRONLY|O_CREAT|O_TRUNC);
    if (-1 == out_fd) {
        perror("open [dst]");
        return -1;
    }

    while (!g_quit) {
        nsent = transfer(out_fd, in_fd, NULL, chunk_len, flags);
        if (errno) {
            err = errno;
            perror("transfer");
            break;
        }

        if (0 == nsent) { /* EOF */
            start = lseek(in_fd, 0, SEEK_SET);
            if (-1 == start) {
                err = errno;
                perror("lseek [start]");
                break;
            }
            (void) fprintf(stderr, "%s: reading [%s] from the start\n",
                __func__, src_path);
        }

        ntotal += nsent;

        if ((ntotal + chunk_len) >= MAX_DST_LENGTH) {
            if (-1 == rollover(dst_path, &out_fd, ++nrolls))
                break;
        }

        if (-1 == nanosleep(&tms, NULL))
            perror("nanosleep");
    }

    close(out_fd);
    close(in_fd);

    return err ? -1 : 0;
}



/* __EOF__ */

