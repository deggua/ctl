#include <stdint.h>
#include <stdio.h>

#define Dict_KeyType   int
#define Dict_ValueType float
#include "ctl/containers/dict.h"

int main(void) {
    Dict(int, float)* dict = Dict_New(int, float)(0);

    for (int ii = 0; ii < 64; ii++) {
        Dict_Set(dict, ii, (float)ii);
    }

    printf("Enumeration:\n");

    int* key = NULL;
    while ((key = Dict_EnumerateKeys(dict, key))) {
        float val;
        Dict_Get(dict, *key, &val);
        printf("dict[%d] = %f\n", *key, val);
    }

    printf("Lookups:\n");

    for (int ii = 0; ii < 128; ii++) {
        float val;
        if (Dict_Get(dict, ii, &val)) {
            printf("dict[%d] = %f\n", ii, val);
        } else {
            printf("dict[%d] = ?\n", ii);
        }
    }

    return 0;
}
