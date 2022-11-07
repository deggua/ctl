#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Dict_KeyType         int
#define Dict_ValueType       char*
#define Dict_ValueType_Alias str
#include "containers/dict.h"

#define Dict_KeyType       char*
#define Dict_KeyType_Alias str
#define Dict_ValueType     int
#include "containers/dict.h"

typedef struct {
    int x, y;
} foo;

#define Dict_KeyType            foo
#define Dict_ValueType          float
#define Dict_HashKey(key)       (key.x + key.y)
#define Dict_CompareKey(k1, k2) (k1.x == k2.x && k1.y == k2.y)
#include "containers/dict.h"

bool enable_allocations = true;

void* Special_Malloc(size_t bytes) {
    if (enable_allocations) {
        return malloc(bytes);
    } else {
        return NULL;
    }
}

void* Special_Realloc(void* ptr, size_t bytes) {
    void* new_mem = Special_Malloc(bytes);
    if (new_mem) {
        free(ptr);
    }

    return new_mem;
}

void Special_Free(void* ptr) {
    free(ptr);
}

#define Dict_KeyType   int
#define Dict_ValueType float
#define Dict_Malloc    Special_Malloc
#define Dict_Realloc   Special_Realloc
#define Dict_Free      Special_Free
#include "containers/dict.h"

int main(void) {
    /* -- Test A, Basic Get/Set usage --- */
    Dict(int, str)* dict_a = Dict_New(int, str)(0);
    assert(dict_a != NULL);

    assert(Dict_Set(dict_a, 1, "abc"));
    assert(Dict_Set(dict_a, 2, "xyz"));
    assert(Dict_Set(dict_a, 3, "ijk"));

    char* out_val_a;

    assert(Dict_Get(dict_a, 1, &out_val_a));
    assert(!strcmp(out_val_a, "abc"));
    assert(Dict_Get(dict_a, 2, &out_val_a));
    assert(!strcmp(out_val_a, "xyz"));
    assert(Dict_Get(dict_a, 3, &out_val_a));
    assert(!strcmp(out_val_a, "ijk"));

    assert(!Dict_Get(dict_a, 4, &out_val_a));

    /* --- Test B, Large Get/Set usage --- */
    Dict(str, int) dict_b;
    assert(Dict_Init(&dict_b, 0));

    for (int ii = 0; ii < 4096; ii++) {
        char* tmp = malloc(32);
        snprintf(tmp, 32, "%d", ii);
        assert(Dict_Set(&dict_b, tmp, ii));
    }

    for (int ii = 0; ii < 4096; ii++) {
        char tmp[32];
        snprintf(tmp, 32, "%d", ii);
        int out_val_b;
        assert(Dict_Get(&dict_b, tmp, &out_val_b));
        assert(out_val_b == ii);
    }

    /* --- Test C, Weirder usage --- */
    Dict(str, int)* dict_c = Dict_New(str, int)(0);
    assert(dict_c != NULL);
    assert(Dict_Copy(&dict_b, dict_c));

    for (int ii = 0; ii < 4096; ii++) {
        char tmp[32];
        snprintf(tmp, 32, "%d", ii);
        int out_val_b;
        assert(Dict_Get(&dict_b, tmp, &out_val_b));
        assert(out_val_b == ii);
    }

    Dict_Uninit(dict_c);
    assert(Dict_Copy(&dict_b, dict_c));

    for (int ii = 0; ii < 4096; ii++) {
        char tmp[32];
        snprintf(tmp, 32, "%d", ii);
        int out_val_b;
        assert(Dict_Get(&dict_b, tmp, &out_val_b));
        assert(out_val_b == ii);
    }

    printf("All tests passed\n");
    return 0;
}
