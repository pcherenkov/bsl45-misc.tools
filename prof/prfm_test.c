/* @(#) prfm POC/test module */

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

#include "prfm.h"

static const char *g_app = NULL;


static void
usage(int err, const char *fmt, ...)
{
    va_list ap;

    if (fmt) {
	    va_start(ap, fmt);
            vfprintf(stderr, fmt, ap);
	    va_end(ap);
    }

    (void) fprintf(stderr, "Usage: %s test_name num_iter [bufsz]\n", g_app);
    exit(err);
}


static void
fatal(const char *msg)
{
	int err = errno;
    perror(msg);

    (void) printf("%s ABORTING [errno=%d]\n", g_app, err);
    abort();
}


static int
test_write(size_t n, size_t len)
{
    char *buf = NULL;
    int fd = -1;
    char tmpfile[] = "/tmp/prfm.test.XXXXXX";
    
    if (NULL == (buf = malloc(len)))
    	fatal("malloc");

    (void) memset(buf, 0, len);

    fd = mkstemp(tmpfile);
    if (-1 == fd)
    	fatal("open");

    if (-1 == unlink(tmpfile))
    	perror("unlink");

    while (--n) {
        if (0 >= write(fd, buf, len)) {
        	perror("write");
        	break;
        }
    }
    
    (void) close(fd);
    free(buf);

    return 0;
}


int main(int argc, char *const argv[])
{
    int rc = 0;
    const char *test_name = NULL;
    long niter = 0, bufsz = 0;

    if (1 == argc)
    	usage(0, NULL);

    if (argc < 3)
    	usage(EINVAL, "Insufficient number of parameters\n");

    test_name = argv[1];

    niter = atol(argv[2]);
    if (niter <= 0)
    	usage(EINVAL, "Invalid number of iterations: %s\n", argv[2]);

    if (0 == strcmp("write", test_name)) {
    	if (argc < 4)
    		usage(EINVAL, "write test requires more parameters\n");

    	bufsz = atol(argv[3]);
    	if (bufsz <= 0)
    		usage(EINVAL, "Invalid buffer size: %s\n", argv[3]);

    	rc = test_write((size_t)niter, (size_t)bufsz);
    }
    else {
        (void) fprintf(stderr, "%s: test [%s] is not (yet) supported\n",
            g_app, test_name);
        return EINVAL;
    }

    (void) printf("%s finished, rc=%d\n", g_app, rc);
    return rc;
}


/* __EOF__ */

