# ctl
C Template Library

A collection of templated containers for C. Currently on supports clang with extensions used for overloading
* This will be optional, but reduces verbosity significantly, and is preferred if you can use them

Other useful things will be added in the future, probably as I need them (SIMD wrappers, algorithms, etc.)

Usage:
```C
#define Vector_Type int
#include "ctl/containers/vector.h"

int main(void) {
    Vector(int)* vec = Vector_New(int)(16);

    for (int ii = 0; ii < 20; ii++) {
        Vector_Push(vec, &ii);
    }

    for (size_t ii = 0; ii < vec->length; ii++) {
    	printf("vec[%zu] = %d\n", vec->at[ii]);
    }

    return 0;
}
```

