/* @(#) C co-routine based condition variables */

#include "coro/coro.h"

static struct coro_context *current_ctx;


struct evc_node {
    struct coro_context *ctx;
    struct evc_node     *next;
};

struct evc_cond {
    struct evc_node     *waiting;   /* Co-routiones waiting on the variable. */
};


int evc_cond_create(evc_cond_t *cond);

int evc_cond_wait(evc_cond_t *cond, struct coro_context *ctx);

int evc_cond_signal(evc_cond_t *cond);

int evc_cond_broadcast(evc_cond_t *cond);

/* ====================================================== */


int
evc_cond_wait(evc_cond_t *cond, struct coro_context *ctx)
{
    add_node(cond->waiting, ctx);
    
}



/* __EOF__ */

