/* @(#) define pthread cleanup routine */

#include <stdio.h>
#include <pthread.h>

static unsigned int g_exit = 0;

static void do_something() { g_exit = 1; }
static void* thr_start(void *arg) { return arg; }

extern void another_ref();

int main()
{
    int rc = 0;
    pthread_t thr = (pthread_t)NULL;

    (void) pthread_atfork(NULL, NULL, do_something);
    rc = pthread_create(&thr, NULL, thr_start, NULL);
    if (rc) return rc;

    (void) pthread_join(thr, NULL);

    (void) printf("Over and out, rc=%d\n", rc);
    return rc;
}

/* __EOF__ */

