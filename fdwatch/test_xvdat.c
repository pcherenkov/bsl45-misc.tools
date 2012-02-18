/* @(#) xvdat unit test */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "xvdat.h"

struct rec {
    char    name[10];
    long    index;
};

struct xv_rec {
    struct rec  *r;
    size_t      len,
                max_len;
    char        *taken;
    size_t      border;
    ssize_t     avail;
};


static void
dump_xvl(FILE* fp, const struct xvlst* v)
{
    size_t i = 0;

    assert(fp && v && v->data);
    (void) fprintf(fp, "[%p]: r=[%p] {len=%ld, max_len=%ld}\n",
        (void*)v, (void*)v->data, (long)v->len, (long)v->max_len);

    if (v->taken) {
        (void) fprintf(fp, "border=%ld, avail=%ld\n",
                (long)v->border, (long)v->avail);
        for(i = 0; i < v->border; ++i)
            (void) fputc(v->taken[i] ? '1' : '.', fp);
        (void) fputc('\n', fp);
    }
}


static void
dump_xvrec(FILE* fp, const struct xv_rec* xvr)
{
    size_t i = 0;

    assert(fp && xvr);
    if (!xvr->r)
        return;

    for(i = 0; i < (xvr->taken ? xvr->border : xvr->len); ++i) {
        if (xvr->taken && !xvr->taken[i])
            continue;
        (void) fprintf(fp, "%4ld. %s, %ld\n",
            (long)i, xvr->r[i].name, xvr->r[i].index);
    }
    return;
}


static void
dump(FILE* fp, const struct xv_rec* xvr, u_int32_t flags)
{
    dump_xvl(fp, (const struct xvlst*)xvr);
    if (flags)
        dump_xvrec(fp, xvr);
}


static int
test_xvec(size_t capacity)
{
    int rc = -1;
    struct xv_rec xvr;
    size_t i = 0;

    (void) memset(&xvr, 0, sizeof(xvr));

    do {
        rc = xvec_reserve((struct xvec*)&xvr, capacity/2,
            sizeof(struct rec), 10);
        if (0 != rc) {
            (void) fprintf(stderr, "%s: xvec_reserve failed [rc=%d]\n",
                __func__, rc);
            break;
        }
        dump(stdout, &xvr, 0);
        assert((0 == xvr.len) && (xvr.max_len >= capacity/2));

        for(i = 0; i < capacity/2; ++i) {
            rc = xvec_expand((struct xvec*)&xvr, 1,
                            sizeof(struct rec), 10);
            if (rc) {
                (void) fprintf(stderr, "%s: xvec_expand failed "
                    "[rc=%d]\n", __func__, rc);
            }
            snprintf(xvr.r[i].name, sizeof(xvr.r[i].name)-1, "N%ld\n",
                (long)i+1);
            xvr.r[i].index = i;
            xvr.len++;
        }
        dump(stdout, &xvr, 1);

    } while(0);


    free(xvr.r);

    return rc;
}


int
main(int argc, char* const argv[])
{
    int rc = 0;
    const char *mode = NULL;
    long vsize = -1;

    enum { MIN_VSIZE = 4 };

    if (argc < 3) {
        (void) fprintf(stderr, "Usage: %s xvec|xvlst size...\n",
                        argv[0]);
        return 1;
    }

    mode = argv[1];
    vsize = atol(argv[2]);
    if (vsize <= MIN_VSIZE) {
        (void) fprintf(stderr, "%s: size should be no less that %ld\n",
            argv[0], (long)MIN_VSIZE);
        return 1;
    }

    if (0 == strcasecmp("xvec", mode))
        rc = test_xvec((size_t)vsize);
    else {
        (void) fprintf(stderr, "%s: %s not supported\n",
            argv[0], mode);
    }

    return rc;
}

/* __EOF__ */

