/* @(#) shorter version of hello world */
#include <unistd.h>

int main()
{
    static char HI[] = "Hi!\n";
    (void) write(1, HI, sizeof(HI)-1);
    return 0;
}

/* __EOF__ */

