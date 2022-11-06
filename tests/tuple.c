#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Tuple_Types uint8_t, float, size_t, uint32_t
#include "../ctl/containers/tuple.h"

#define Tuple_Types         char*, double
#define Tuple_Types_Aliases str, double
#include "../ctl/containers/tuple.h"

#define type_to_name(type) \
    _Generic((type), uint8_t : "uint8_t", float : "float", size_t : "size_t", uint32_t : "uint32_t", char*: "char*", double: "double")

int main(void) {
    Tuple(uint8_t, float, size_t, uint32_t) tuple;

    printf("typeof(tuple.e1) = %s\n", type_to_name(tuple.e1));
    printf("typeof(tuple.e2) = %s\n", type_to_name(tuple.e2));
    printf("typeof(tuple.e3) = %s\n", type_to_name(tuple.e3));
    printf("typeof(tuple.u4) = %s\n", type_to_name(tuple.e4));

    Tuple(str, double) tuple2;

    printf("typeof(tuple2.e1) = %s\n", type_to_name(tuple2.e1));
    printf("typeof(tuple2.e2) = %s\n", type_to_name(tuple2.e2));

    return 0;
}
