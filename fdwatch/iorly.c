/* @(#) iorly: streamable I/O relay POC */

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <assert.h>


static int g_kq;

enum {
    INFO_EOF = 1,        /* eof src position */
    INFO_DST = (1 << 1), /* is a dst record */
};

struct fdrec {
    int         fd;
    u_int32_t   fdio_flags;
    off_t       src_offset;
    u_int32_t   info;
};


struct ev_list {
    struct kevent   *ke;
    int             len;
    size_t          capacity;
};


/* TODO: make it fd-keyed hash table */
struct fdr_list {
    ssize_t         first_empty, max_taken;  /* index of the first empty record */
    struct fdrec    *rec;
    size_t          count,
                    capacity;
};


struct server_ctx {
    int             src_fd;     /* one fd to read from */
    ssize_t         idle_tmout; /* no-events timeout   */

    struct ev_list  events,     /* kqueue event list   */
                    changes;    /* kqueue change list  */

    int             in_dst;     /* dst/client fd inbox */
    int             in_sig;     /* signal notification inbox */

    struct fdr_list dst;        /* dst/client records */
};



static int
evl_ensure_avail(struct ev_list *evl, size_t n)
{
    size_t nblocks = 0, new_capacity = 0;
    int err = 0;

    enum {
        EVL_MIN_EXT = 100,
        EVL_MAX_LEN = 10000
    };

    assert(evl && (n > 0));

    if ((evl->len + n) < evl->capacity)
        return 0;

    nblocks = (n / EVL_MIN_EXT) ? (n / EVL_MIN_EXT) : 1;
    new_capacity = evl->capacity + nblocks;
    if (new_capacity > EVL_MAX_LEN)
        return EOVERFLOW;

    /* TODO: revise - use static or pre-allocated memory */
    p = realloc(evl->ke, new_capacity*sizeof(evl->ke[0]));
    if (!p) {
        err = errno;
        perror("realloc");
        if (evl->ke)
            free(evl->ke);
        return err;
    }

    ev->ke = (struct )p;
    ev->capacity = new_capacity;

    return 0;
}


static int
get_stat_mode(int fd, mode_t *st_mode)
{
    struct stat sb;

    assert((fd >= 0) && st_mode);

    if (0 != fstat(fd, &sb)) {
        perror("fstat");
        return -1;
    }

    *st_mode = sb.st_mode;

    return 0;
}


static int
kq_reader_add(struct ev_list *events, int fd, void *udata)
{
    size_t i = 0;
    mode_t mode = (mode_t)0;

    assert((kq > 0) && events && (fd >= 0));

    if (0 != get_stat_mode(fd, &mode))
        return -1;

    if (0 != evl_ensure_avail(events, S_ISREG(mode) ? 2 : 1))
        return -1;

    i = events->len;

    if (S_ISREG(mode)) {
        EV_SET(&events->ke[i], fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
            NOTE_DELETE|NOTE_RENAME, 0, udata);

        events->len++;
        ++i;
    }

    EV_SET(&events->ke[i], fd, EVFILT_READ, EV_ADD, 0, 0, udata);
    events->len++;

    return 0;
}



enum {
    FDRL_INITIAL_LEN = 100,
    FDRL_EXT = 50
};

/* TODO: put fdrl into a separate MODULE */
static struct fdr*
fdrl_add(struct fdr_list *fdrl, int fd)
{
    struct fdr *r = NULL;
    size_t i = 0, old_max = 0;

    assert(fdrl && (fd >= 0));

    if (NULL == fdrl->rec) {
        if (0 != fdrl_init(fdrl, FDRL_INITIAL_LEN)) /* TODO: */
            return NULL;
    }

    if (fdrl->max_taken >= fdrl->capacity) {
        old_max = fdrl->max_taken;

        if (0 != fdrl_expand(fdrl, FDRL_EXT)) /* TODO: */
            return NULL;

        fdrl->first_empty = old_max + 1;
    }

    assert((fdrl->first_empty >= 0) &&
            (fdrl->first_empty <= (fdrl->max_taken + 1)) &&
            (fdrl->first_empty < fdrl->capacity));

    r = &fdrl->rec[fdrl->first_empty];
    r->fd            = fd;
    r->fdio_flags    = 0; /* TODO: mind fd type and if sendfile is avail */
    r->src_offset    = 0;
    r->info          = 0;

    if (fdrl->first_empty > fdrl->max_taken)
        fdrl->max_taken = fdrl->first_empty;

    /* TODO: this is O(N) so make it *HASH* once out of POC phase */

    /* now that we've taken the first_empty spot, find next */
    for (i = fdrl->first_empty + 1, fdrl->first_empty = -1;
            (i <= (fdrl->max_taken + 1)) && (i < fdrl->capacity); ++i) {
        if (-1 == fdrl->rec[i].fd) {
            fdrl->first_empty = i;
            break;
        }
    }
    assert((fdrl->first_empty >= 0) &&
            (fdrl->first_empty <= (fdrl->max_taken + 1)) &&
            (fdrl->first_empty < fdrl->capacity));

    fdrl->count++;
    return r;
}


inline static int
add_dst(struct server_ctx *ctx, int fd)
{
    struct fdr *r = NULL;
    assert(ctx && (fd >= 0));

    if (NULL == (r = fdrl_add(&ctx->dst, fd)))
        return -1;
    r->info | = INFO_DST;

    return kq_reader_add(&ctx.changes, fd, r);
}


static int
server_loop(int *fds, size_t len)
{
    int rc = 0;
    struct timespec tmout = {0, 0}, *ptmout = NULL;
    struct kevent *ev = NULL, *chg = NULL;
    int    ev_count = 0, chg_count = 0;

    assert (0 == g_kq);

    g_kq = kqueue();
    if (-1 == g_kq) {
        perror("kqueue");
        return -1;
    }

    rc = kq_reader_add(&g_srv.changes, g_srv.src_fd, NULL);
    if (rc) return -1;

    if (g_srv.in_dst > 0) {
        rc = kq_reader_add(&g_srv.changes, g_srv.in_dst, NULL);
        if (rc) return -1;
    }
    else
        (void)fprintf(stderr, "%s: no client inbox\n", __func__);


    if (g_srv.in_sig) {
        rc = kq_reader_add(&g_srv.changes, g_srv.in_sig, NULL);
        if (rc) return -1;
    }
    else
        (void)fprintf(stderr, "%s: no signal inbox\n", __func__);

    if (fds && len) {
        for(i = 0; i < len; ++i) {
            if (0 != add_dst(&g_srv, fds[i]))
                return -1;
        }
    }

    if (g_srv.idle_tmout > 0) {
        tmout.tv_sec = (time_t)g_srv.idle_tmout;
        ptmout = &tmout;
    }


    (void) fputs("Entering server loop\n", stdout);
    while (1) {
        /* TODO: */
    }
    (void) fputs("Exited server loop\n", stdout);

}


/* __EOF__ */

