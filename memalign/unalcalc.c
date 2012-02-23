/* @(#) unaligned memory access test */
/* gcc -W -Wall --pedantic -O3 -o unal.tt ./unalign.c */

#include <sys/mman.h>
#include <sys/types.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>

#if !defined(MAP_ANONYMOUS)
    #ifdef MAP_ANON
        #define MAP_ANONYMOUS MAP_ANON
    #else
        #error "MAP_ANON[YMOUS] is amiss"
    #endif
#endif


static u_long
align128bit_offset(void* p, void** ap)
{
    u_long  msk_right4bits0 = (u_long)-1 << 4,
            lp = (u_long)p, aligned_lp = lp;

    if (lp & msk_right4bits0)
        aligned_lp = ((lp >> 4) + 1) << 4;

    if (ap) *ap = (void*)aligned_lp;

    return aligned_lp - lp;
}


int
main(int argc, char* const argv[])
{
    void *mblock = NULL, *ap = NULL;
    long **vp = NULL;
    ssize_t mb_len = 32768, offset = 0;
    int rc = 0, should_align = 0;
    u_long al_offset = 0;

    (void) argc; (void) argv;

    if (argc < 3) {
        (void) fprintf(stderr, "Usage: %s size offset [align=Y|N]\n", argv[0]);
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

    if (argc > 3) {
        should_align = !strcasecmp("y", argv[3]) || !strcasecmp("yes", argv[0]);
    }

    mblock = mmap(NULL, (size_t)mb_len, PROT_READ|PROT_WRITE,
                MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if ((void*)-1 == mblock) {
        perror("mmap");
        return 1;
    }

    vp = (long**) ((char*)mblock + offset);
    al_offset = align128bit_offset(vp, &ap);
    if (al_offset) {
        (void) fprintf(stderr, "vp=%p is (128-bit) misaligned, suggesting "
            "vp=%p, offset=%lu\n", (void*)vp, ap, al_offset);
        if (should_align) {
            vp = (long**)((char*)vp + al_offset);
            (void) printf("vp adjusted to be 128-bit-aligned\n");
        }
        else {
            (void) printf("WARNING: vp remains *NOT* 128-bit-aligned\n");
        }
    }

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

