/* @(#) iorly: streamable I/O relay POC */

#include <sys/types.h>


struct node {
    struct node *src;
    int         fd;
    u_int32_t   fdio_flags;
    off_t       offset;
    int         is_eof;
};




/* __EOF__ */

