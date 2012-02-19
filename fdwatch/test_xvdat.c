/* @(#) xvdat unit test */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

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
    (void) fprintf(fp, "[%p]: r=[%p] {len=%ld, max_len=%ld",
        (void*)v, (void*)v->data, (long)v->len, (long)v->max_len);

    if (v->taken) {
        (void) fprintf(fp, ", border=%ld, avail=%ld} taken=[",
                (long)v->border, (long)v->avail);
        for (i = 0; i < v->border + 1; ++i)
            (void) fputc(v->taken[i] ? '1' : '0', fp);
        (void) fputs("]\n", fp);
    }
    else {
        (void) fputs("}\n", fp);
    }
}


static void
dump_xvrec(FILE* fp, const struct xv_rec* xvr)
{
    size_t i = 0;

    assert(fp && xvr);
    if (!xvr->r)
        return;

    for (i = 0; i < (xvr->taken ? xvr->border+1 : xvr->len); ++i) {
        if (xvr->taken && !xvr->taken[i])
            continue;
        (void) fprintf(fp, "%4ld. %s, %ld\n",
            (long)i, xvr->r[i].name, xvr->r[i].index);
    }
    return;
}


static void
dump(const char* msg, FILE* fp,
    const struct xv_rec* xvr, u_int32_t flags)
{
    if (msg)
        (void) fprintf(fp, "%s:\n", msg);

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
        dump("Reserved xvec (size/2)", stdout, &xvr, 0);
        assert((0 == xvr.len) && (xvr.max_len >= capacity/2));

        for (i = 0; i < capacity/2; ++i) {
            rc = xvec_expand((struct xvec*)&xvr, 1,
                            sizeof(struct rec), 10);
            if (rc) {
                (void) fprintf(stderr, "%s: xvec_expand failed "
                    "[rc=%d]\n", __func__, rc);
                break;
            }
            snprintf(xvr.r[i].name, sizeof(xvr.r[i].name)-1, "N%ld",
                (long)i+1);
            xvr.r[i].index = i;
            xvr.len++;
        }
        if (rc) break;
        dump("filled xvec", stdout, &xvr, 1);

        for (i = xvr.len; i < capacity*2; i+=2) {
            rc = xvec_expand((struct xvec*)&xvr, 2,
                            sizeof(struct rec), 10);
            if (rc) {
                (void) fprintf(stderr, "%s: xvec_expand failed "
                    "[rc=%d]\n", __func__, rc);
                break;
            }
            snprintf(xvr.r[i].name, sizeof(xvr.r[i].name)-1, "F%ld",
                (long)i+1);
            xvr.r[i].index = i * 2;

            snprintf(xvr.r[i+1].name, sizeof(xvr.r[i+1].name)-1, "S%ld",
                (long)i+1);
            xvr.r[i+1].index = (i * 2) + 1;

            xvr.len += 2;
        }
        dump("expanded xvec", stdout, &xvr, 1);
        if (rc) break;

    } while(0);

    free(xvr.r);

    return rc;
}


static int
test_xvlst(size_t capacity)
{
    int rc = -1;
    struct xv_rec xvr;
    size_t i = 0;
    ssize_t j = -1;

    (void) memset(&xvr, 0, sizeof(xvr));
    do {
        rc = xvlst_reserve((struct xvlst*)&xvr, capacity/2,
            sizeof(struct rec), 10);
        if (0 != rc) {
            (void) fprintf(stderr, "%s: xvlst_reserve failed [rc=%d]\n",
                __func__, rc);
            break;
        }
        dump("reserved xvlst", stdout, &xvr, 0);
        assert((0 == xvr.len) && (xvr.max_len >= capacity/2));

        for (i = 0; i < capacity; ++i) {
            j = xvlst_add((struct xvlst*)&xvr, sizeof(struct rec), 10);
            if (j < 0) {
                (void) fprintf(stderr, "%s: xvlst_add failed [rc=%ld]\n",
                    __func__, (long)j);
                rc = -1;
                break;
            }
            if (j != (ssize_t)i) {
                (void) fprintf(stderr, "%s: index out of seq: expected %ld, got %ld\n",
                    __func__, (long)i, (long)j);
                rc = -1;
                break;
            }

            (void) snprintf(xvr.r[j].name, sizeof(xvr.r[j].name)-1, "NN%ld", (long)j);
            xvr.r[j].index = (size_t)j;
        }
        dump("filled xvlst", stdout, &xvr, 1);

        rc = xvlst_del((struct xvlst*)&xvr, 3);
        if (0 != rc) {
            (void) fprintf(stderr, "%s: xvlst_del (3) failed: [rc=%d]\n",
                __func__, rc);
            break;
        }

        rc = xvlst_del((struct xvlst*)&xvr, 3);
        assert(EALREADY == rc);
        rc = 0;

        rc = xvlst_del((struct xvlst*)&xvr, xvr.border + 1);
        assert(ERANGE == rc);
        rc = 0;

        j = xvlst_add((struct xvlst*)&xvr, sizeof(struct rec), 10);
        assert(3 == j);
        (void) strncpy(xvr.r[3].name, "third", sizeof(xvr.r[3].name)-1);

        dump("replaced 3rd element", stdout, &xvr, 1);

        if (xvr.len < 8) {
            (void) fprintf(stderr, "%s: need size >= 8 for more testing\n",
                __func__);
            break;
        }

        for (i = 0; i < 3; ++i) {
            rc = xvlst_del((struct xvlst*)&xvr, i);
            if (0 != rc) {
                (void) fprintf(stderr, "%s:%ld xvlst_del i=%ld [rc=%d]\n",
                    __func__, (long)__LINE__, (long)i, rc);
                break;
            }
        }
        if (rc) break;
        dump("removed first 3", stdout, &xvr, 0);

        for (i = 7; i > 4; --i) {
            rc = xvlst_del((struct xvlst*)&xvr, i);
            if (0 != rc) {
                (void) fprintf(stderr, "%s:%ld xvlst_del i=%ld [rc=%d]\n",
                    __func__, (long)__LINE__, (long)i, rc);
                break;
            }
        }
        if (rc) break;
        dump("removed 3 from 8 down", stdout, &xvr, 0);

        (void) printf ("Adding 10 more elements:\n");
        for (i = 0; i < 10; ++i) {
            j = xvlst_add((struct xvlst*)&xvr, sizeof(struct rec), 10);
            if (j < 0) {
                (void) fprintf(stderr, "%s: xvlst_add failed [rc=%ld]\n",
                    __func__, (long)j);
                rc = -1;
                break;
            }

            (void) snprintf(xvr.r[j].name, sizeof(xvr.r[j].name)-1, "DD%ld", (long)j);
            xvr.r[j].index = (size_t)j;

            (void) printf("j=%ld ", (long)j);
            dump(NULL, stdout, &xvr, 0);
        }
        if (rc) break;
        dump("Elements added", stdout, &xvr, 1);

    } while(0);

    free(xvr.r);
    free(xvr.taken);

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
    else if (0 == strcasecmp("xvlst", mode))
        rc = test_xvlst((size_t)vsize);
    else {
        (void) fprintf(stderr, "%s: %s not supported\n",
            argv[0], mode);
    }

    (void) printf("%s: exiting with rc=%d\n", argv[0], rc);
    return rc;
}

/* __EOF__ */

