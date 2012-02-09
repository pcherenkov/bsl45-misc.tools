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
    (void) fprintf(stderr, "Moved %s to %s\n", fpath, newpath);

    if (-1 == fsync(*fd)) {
        perror("fsync");
        return -1;
    }
    if (-1 == close(*fd)) {
        perror("close");
        return -1;
    }

    *fd = -1;

    /*
    if (-1 == unlink(newpath)) {
        perror("unlink");
        return -1;
    }
    */

    *fd = open(fpath, O_WRONLY|O_CREAT|O_TRUNC,
        S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (-1 == (*fd)) {
        perror("open [roll]");
        return -1;
    }

    return 0;
}


static int
cyclic_file_relay(const char* src_path, const char* dst_path,
    long ms_delay, size_t max_dst_len)
{
    int in_fd = -1, out_fd = -1, err = 0;
    u_int32_t flags = 0;
    ssize_t ntotal = 0, nsent = 0;
    off_t start = 0;
    size_t chunk_len = MTU_DEFAULT * 10, nrolls = 0;
    struct timespec tms;

    static const long NSEC_IN_MS = 1000000;

    tms.tv_sec  = ms_delay / 1000;
    tms.tv_nsec = (ms_delay % 1000) * NSEC_IN_MS;

    do {
        in_fd = open(src_path, O_RDONLY);
        if (-1 == in_fd) {
            err = errno;
            perror("open [src]");
            break;
        }

        out_fd = open(dst_path, O_WRONLY|O_CREAT|O_TRUNC,
            S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (-1 == out_fd) {
            err = errno;
            perror("open [dst]");
            break;
        }
    } while(0);

    (void) fputs("Beginning data transfer:\n", stdout);
    ntotal = 0;
    while ((in_fd >= 0) && (out_fd >=0) && !g_quit) {
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
            (void) fprintf(stderr, "\n%s: reading [%s] from the start\n",
                __func__, src_path);
        }
        else
            (void) fputc('.', stdout);

        ntotal += nsent;

        if ((ntotal + (size_t)nsent) >= max_dst_len) {
            (void) fputc('\n', stdout);
            if (-1 == rollover(dst_path, &out_fd, ++nrolls))
                break;
            (void) printf("%ld bytes relayed, rolled over\n", (long)ntotal);
            ntotal = 0;
        }

        if (-1 == nanosleep(&tms, NULL))
            perror("nanosleep");
    }

    if (out_fd >= 0)
        close(out_fd);
    if (in_fd >= 0)
        close(in_fd);

    return err ? -1 : 0;
}


int main(int argc, char* const argv[])
{
    int rc = 0;
    long delay_ms = -1;
    ssize_t max_dst_len = 1024 * 1024 * 10;
    static const ssize_t MIN_DST_SZ = 1024 * 10;

    if (argc < 4) {
        (void) fprintf(stderr, "Usage: %s srcfile dstfile delay_ms "
                    "[max_dst_size]\n", argv[0]);
        return 1;
    }

    assert(argc > 3);
    delay_ms = atol(argv[3]);
    if ((delay_ms <= 0) || (delay_ms > 3000)) {
        (void) fprintf(stderr, "%s: invalid delay_ms value: %s\n",
            argv[0], argv[3]);
        return 1;
    }

    if (argc > 4) {
        max_dst_len = (ssize_t)atol(argv[4]);
        if (max_dst_len <= MIN_DST_SZ) {
            (void) fprintf(stderr, "%s: invalid max_dst_size value: %s\n",
                argv[0], argv[4]);
            return 1;
        }
    }

    setvbuf(stdout, NULL, _IONBF, 0);
    rc = cyclic_file_relay(argv[1], argv[2], delay_ms, max_dst_len);

    (void) printf("%s: exiting with rc=%d\n", argv[0], rc);
    return rc;
}


/* __EOF__ */

