/* @(#) convert double to timespec */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int main(int argc, char* const argv[])
{
    double dval = 0.0;
    struct timespec tms;

    if (argc < 2) {
        (void) fprintf(stderr, "Usage: %s double\n", argv[0]);
        return 1;
    }

    dval = atof(argv[1]);
    if (dval <= 0.0) {
        (void) fprintf(stderr, "Invalid parameter value: %s\n", argv[0]);
        return 1;
    }

    (void) printf("dval = %.09f\n", dval);

    tms.tv_sec = floor(dval);
    tms.tv_nsec = (dval - floor(dval)) * 1000000000.0;

    (void) printf("tv_sec=%ld, tv_nsec=%ld\n", (long)tms.tv_sec, (long)tms.tv_nsec);
    return 0;
}


/* __EOF__ */

