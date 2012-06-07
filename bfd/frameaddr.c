/* @(#) print stack-frame address of a C function */

#include <stdio.h>
#include <stdlib.h>

struct frame {
    struct frame *next;
    void *ret;
};


#if defined(USE_RBP)
#define GET_CURRENT_FRAME(f)                \
    do {                                    \
        register void *__f asm("rbp");      \
        f = __f;                            \
    } while(0)
#elif defined(__GNUC__)
#define GET_CURRENT_FRAME(f)                \
    do {                                    \
        f = __builtin_frame_address(0);     \
    } while(0)
#endif


static void
print_frameaddr()
{
    void *frame_addr = NULL;

    GET_CURRENT_FRAME(frame_addr);
    (void) fprintf(stderr, "%s frame = %p", __func__, frame_addr);
    if (frame_addr) {
        fprintf(stderr, " return_addr=%p", ((struct frame*)frame_addr)->ret);
    }
    (void) fputc('\n', stderr);
}


int main()
{
    print_frameaddr();
    return 0;
}


