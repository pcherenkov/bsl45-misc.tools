/* @(#) another module, using pthread cleanup */

#include <pthread.h>

static unsigned int m_exit = 0;
static void do_something_else() { m_exit = 1; }

void
another_ref()
{
    (void) pthread_atfork(NULL, NULL, do_something_else);
}


/* __EOF__ */

