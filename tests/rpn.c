#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#define Stack_Type float
#include "../ctl/containers/stack.h"

bool IsNumeric(const char* str) {
    size_t len = strlen(str);

    for (size_t ii = 0; ii < len; ii++) {
        if ('0' <= str[ii] && str[ii] <= '9') {
            continue;
        } else {
            return false;
        }
    }

    return true;
}

float EvalExpr(float lhs, char op, float rhs) {
    switch (op) {
        case '+': return lhs + rhs;
        case '-': return lhs - rhs;
        case '*': return lhs * rhs;
        case '/': return lhs / rhs;
        case '^': return powf(lhs, rhs);
        case '%': return remainderf(lhs, rhs);
        default: return NAN;
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Expected one argument, received %d\n", argc - 1);
        return EXIT_FAILURE;
    }

    Stack(float)* stack = Stack_New(float)();

    for (char* token = strtok(argv[1], " "); token != NULL; token = strtok(NULL, " ")) {
        if (IsNumeric(token)) {
            float number = atof(token);
            Stack_Push(stack, &number);
        } else {
            float lhs, rhs;
            if (!Stack_Pop(stack, &rhs) || !Stack_Pop(stack, &lhs)) {
                printf("Unbalanced expression\n");
                return EXIT_FAILURE;
            }

            float result = EvalExpr(lhs, token[0], rhs);
            Stack_Push(stack, &result);
        }
    }

    if (stack->count != 1) {
        printf("Unbalanced expression\n");
        return EXIT_FAILURE;
    }

    float result;
    Stack_Pop(stack, &result);
    printf(" = %f\n", result);

    Stack_Delete(stack);
    return EXIT_SUCCESS;
}
