#include <stdio.h>
#include <math.h>
#include <float.h>

int main(int argc, char** argv)
{
    #define CEILF(_X) printf("ceilf(%f) = %f\n", _X, ceilf(_X));
    #define FLOORF(_X) printf("floorf(%f) = %f\n", _X, floorf(_X));

    #define CEIL(_X) printf("ceil(%f) = %f\n", _X, ceil(_X));
    #define FLOOR(_X) printf("floor(%f) = %f\n", _X, floor(_X));

    CEILF(-0.8f);
    CEILF(-0.2f);
    CEILF( 0.2f);
    CEILF( 0.8f);

    FLOORF(-0.8f);
    FLOORF(-0.2f);
    FLOORF( 0.2f);
    FLOORF( 0.8f);

    FLOORF(-999999.5f);
    FLOORF(-9999999.5f);
    FLOORF(-99999999.5f);
    FLOORF(-999999999.5f);

    FLOOR(-999999999999999.5);
    FLOOR(-9999999999999999.5);
    FLOOR(-99999999999999999.5);

    // printf("sizeof(long long) == %zu\n", sizeof(long long));
    // printf("\n");

    // float f1 = pow(2,24);
    // float f2 = pow(2,24)+1;
    // printf("f1/f2: %f, %f\n", f1, f2);

    // float f3 = nextafterf(f1, f1-2);
    // printf("f3: %f  floor: %f\n", f3, floorf(f3));

    // long long i = pow(2,24)+1;
    // printf("i = %lld\n", i);

    // CEILF(-0.8f);
    // CEILF(-0.2f);
    // CEILF( 0.2f);
    // CEILF( 0.8f);

    // FLOORF(-0.8f);
    // FLOORF(-0.2f);
    // FLOORF( 0.2f);
    // FLOORF( 0.8f);

    return 0;
}