/* @(#) memcached testing utility */
/* gcc -W -Wall --pedantic  -Wl,--no-as-needed -lmemcached -g -o mmcu  mmcu.c */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>

#include <errno.h>
#include <assert.h>

#include <libmemcached/memcached.h>


static const char *g_appname = NULL;


static void
usage_exit(int exit_code, const char *fmt, ...)
{
    va_list ap;

    if (fmt) {
        va_start(ap, fmt);
            vfprintf(stderr, fmt, ap);
        va_end(ap);
    }

    assert(g_appname);
    (void) fprintf(stderr, "Usage: %s host port name string size\n",
                g_appname);
    exit(exit_code);
}



int
main(int argc, char const *argv[])
{
    int rc = 0;
    memcached_return mrc = 0;
    memcached_st *st = NULL;
    in_port_t port = 0;
    const char *host = NULL, *key = NULL, *val = NULL;
    size_t size = 0;
    long lval = -1;
    char *buf = NULL, *p = NULL;
    size_t len = 0, left = 0, n = 0;
    u_int32_t flags = 0;

    g_appname = argv[0];

    if (argc < 2)
        usage_exit(1, NULL);
    else if (argc < 5)
        usage_exit(1, "insufficient number of parameters: %d\n", argc);

    host = argv[1];

    port = (in_port_t)atoi(argv[2]);
    if (port <= 0)
        usage_exit(1, "invalid port value: %hd\n", (short)port);

    key = argv[3];
    val = argv[4];

    lval = atol(argv[5]);
    if (lval <= 0)
        usage_exit(1, "invalid size value: %ld\n", (long)size);
    size = (size_t)lval;

    buf = malloc(size);
    if (!buf) {
        perror("malloc");
        return ENOMEM;
    }


    for (len = strlen(val), p = buf, left = size - 1; left > 0;) {
        n = len > left ? left : len;
        (void) memcpy(p, val, n);
        p += n; left -= n;
    }

    buf[size-1] = '\0';
    assert(strlen(buf) == (size-1));

    st = memcached_create(NULL);
    if (NULL == st) {
        (void) fprintf(stderr, "memcached_create failed\n");
        return 1;
    }

    do {
        (void) printf("Connecting to %s:%ld ... ", host, (long)port);
        mrc = memcached_server_add(st, host, port);
        if (MEMCACHED_SUCCESS != mrc) {
            (void) printf("FAILED [%ld]\n", (long)mrc);
            rc = EIO;
            break;
        }
        else
            (void) fputs("OK\n", stdout);

        (void) printf("Setting key %s [size=%ld] ... ", key, (long)size);
        mrc = memcached_set(st, key, strlen(key), buf, size, (time_t)0, 0);
        if (MEMCACHED_SUCCESS != mrc) {
            (void) printf("FAILED [%ld]\n", (long)mrc);
            rc = EIO;
            break;
        }
        else
            (void) fputs("OK\n", stdout);

        p = NULL;
        (void) printf("Retrieving key %s [size=%ld] ... ", key, (long)size);
        p = memcached_get(st, key, strlen(key), &len, &flags, &mrc);
        if (MEMCACHED_SUCCESS != mrc || !p) {
            (void) printf("FAILED [%ld]\n", (long)mrc);
            rc = EIO;
            break;
        }
        else
            (void) fputs("OK\n", stdout);

        (void) printf("Comparing original vs cached values ... ");
        if (0 != strcmp(buf, p))
            (void) fputs("FAILED\n", stdout);
        else
            (void) fputs("OK\n", stdout);

    } while(0);


    memcached_free(st);
    st = NULL;

    if (buf)
        free(buf);

    (void) printf("%s: exiting with rc=%d\n", g_appname, rc);
    return rc;
}


/* __EOF__ */

