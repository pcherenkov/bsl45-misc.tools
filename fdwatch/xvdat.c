/* @(#) generic data structures */

#include <sys/types.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <assert.h>

#include "xvdat.h"


int
xvec_reserve(struct xvec *v, size_t new_max, size_t blksz, size_t growblk)
{
    size_t nblk = 0;
    void *p = NULL;

    assert(v && (growblk > 0));

    if (new_max < v->max_len)
        return 0;

    nblk = (1 + (new_max-1)/growblk) * growblk;

    p = realloc(v->data, nblk * blksz);
    if (NULL == p)
        return -1;

    v->data = p;
    v->max_len = nblk;

    return 0;
}


int
xvec_expand(struct xvec *v, size_t plus, size_t blksz, size_t growblk)
{
    assert(v && plus && blksz && growblk > 0);
    return xvec_reserve(v, v->len + plus, blksz, growblk);
}


int
xvlst_reserve(struct xvlst *l, size_t new_max, size_t blksz, size_t growblk)
{
    int rc = 0;
    size_t old_max = 0;

    assert(l);
    if (l->max_len >= new_max)
        return 0;

    old_max = l->max_len;

    rc = xvec_reserve((struct xvec*)l, new_max, blksz, growblk);
    if (0 != rc)
        return rc;

    if (l->max_len <= old_max)
        return rc;

    l->taken = realloc(l->taken, l->max_len);
    if (NULL == l->taken)
        return -1;

    (void) memset(&l->taken[old_max], 0, l->max_len - old_max);

    if (l->avail < 0)
        l->avail = old_max;

     return 0;
}


int
xvlst_expand(struct xvlst *l, size_t plus, size_t blksz, size_t growblk)
{
    assert(l && plus && blksz && growblk);
    return xvlst_reserve(l, l->len + plus, blksz, growblk);
}


ssize_t
xvlst_add(struct xvlst *l, size_t blksz, size_t growblk)
{
    ssize_t i = -1;
    char *found = NULL;

    assert(l);
    assert(l->avail < (ssize_t)l->max_len);

    if (0 != xvlst_expand(l, 1, blksz, growblk))
        return -ENOMEM;

    if (l->avail < 0)
        return -1;

    i = l->avail;
    l->taken[i] = 1;

    if ((ssize_t)l->last < i)
        l->last = (ssize_t)i;

    if (i == (ssize_t)l->last) {
        l->avail = i + 1;
        if (l->avail >= (ssize_t)l->max_len)
            l->avail = -1;
        else
            assert(0 == l->taken[l->avail]);
    }
    else if (0 == l->taken[i+1])
        l->avail = i + 1;
    else {
        found = memchr(&l->taken[i+1], 0, l->max_len - (size_t)i - 1);
        l->avail = found ? (found - &l->taken[0]) : -1;
    }

    l->len++;

    return i;
}


int
xvlst_del(struct xvlst *l, size_t index)
{
    assert(l);

    if (index > l->last)
        return ERANGE;

    if (0 == l->taken[index])
        return EALREADY;

    l->taken[index] = 0;

    if ((l->avail < 0) || ((ssize_t)index < l->avail))
        l->avail = index;

    assert(l->len);
    l->len--;

    return 0;
}


/* __EOF__ */

