/* @(#) unaligned memory access test */
/* gcc -W -Wall --pedantic -O3 -o unal.tt ./unalign.c */

#include <sys/mman.h>
#include <sys/types.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

int
main(int argc, char* const argv[])
{
    void *mblock = NULL; 
    long **vp = NULL;
    ssize_t mb_len = 32768, offset = 0;
    int rc = 0;

    (void) argc; (void) argv;

    if (argc < 3) {
        (void) fprintf(stderr, "Usage: %s size offset\n", argv[0]);
        return 1;
    }

    mb_len = atol(argv[1]);
    if (mb_len <= 0) {
        (void) fprintf(stderr, "Invalid size value: %s", argv[1]);
        return 1;
    }

    offset = atol(argv[2]);
    if (offset <= 0) {
        (void) fprintf(stderr, "Invalid offset value: %s", argv[1]);
        return 1;
    }

    mblock = mmap(NULL, (size_t)mb_len, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if ((void*)-1 == mblock) {
        perror("mmap");
        return 1;
    }

    vp = (long**) ((char*)mblock + offset);
    (void) fprintf(stdout, "BEGIN vp=%p\n", (void*)vp);
    for (; (char*)vp < (char*)mblock + mb_len; ++vp) {
        *vp = NULL;
    }
    (void) fprintf(stdout, "END vp=%p\n", (void*)vp);

    if (-1 == (rc = munmap(mblock, mb_len)))
        perror("munmap");

    return rc;
}



/* __EOF__ */

