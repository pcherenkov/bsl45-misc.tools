/* @(#) extensible vector-based data structures */

#ifndef XVDATH_20120218
#define XVDATH_20120218

#include <sys/types.h>

/* extensible C array */
struct xvec {
    void    *data;
    size_t  len,
            max_len;
};


/* xvec-based list */
struct xvlst {
    void    *data;
    size_t  len,
            max_len;

    char    *taken;
    size_t  border; /* index of last element ever used */
    ssize_t avail;  /* index of min available element */
};


#ifdef __cplusplus
extern "C" {
#endif


int
xvec_reserve(struct xvec *v, size_t new_max, size_t blksz, size_t growblk);

int
xvec_expand(struct xvec *v, size_t plus, size_t blksz, size_t growblk);

int
xvlst_reserve(struct xvlst *l, size_t new_max, size_t blksz, size_t growblk);

int
xvlst_expand(struct xvlst *l, size_t plus, size_t blksz, size_t growblk);

ssize_t
xvlst_add(struct xvlst *l, size_t blksz, size_t growblk);

int
xvlst_del(struct xvlst *l, size_t index);


#ifdef __cplusplus
}
#endif

#endif /* XVDATH_20120218 */

/* __EOF__ */

