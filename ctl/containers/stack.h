#include <stddef.h>
#include <stdbool.h>

#if !defined(_STACK_INCLUDED)
#   define _STACK_INCLUDED
#   if !defined(_CONCAT2)
#       define _CONCAT2_(a, b) a ## _ ## b
#       define _CONCAT2(a, b) _CONCAT2_(a, b)
#   endif

/* --------- PUBLIC API ---------- */

// Include Parameters:
//  The type of the elements the stack should contain:
//      #define REQ_PARAM_type                 T
//
//  The initial length the stack should allocate to:
//      Default: 16 elements
//      #define OPT_PARAM_init_capacity         16
//
//  The growth function the stack should use when it needs to expand:
//      Default: 2 * old_size
//      #define OPT_PARAM_grow(old_size)        (2 * old_size)
//
//  An allocator function (obeying ISO C's malloc/calloc semantics):
//      Default: malloc
//      #define OPT_PARAM_malloc(bytes)        malloc(bytes)
//
//  A reallocator function (obeying ISO C's realloc semantics):
//      Default: realloc
//      #define OPT_PARAM_realloc(ptr, bytes)  realloc(ptr, bytes)
//
//  A free function (obeying ISO C's free semantics):
//      Default: free
//      #define OPT_PARAM_free(ptr)            free(ptr)

// Functions:
//  The stack type
#define Stack(T)                _CONCAT2(_Stack, T)

// Create a new stack
//  Stack(T)* Stack_New(T)()
#define Stack_New(T)            _CONCAT2(_Stack_New, T)

// Delete a stack
//  void Stack_Delete(T)(Stack(T)* stack)
#define Stack_Delete(T)         _CONCAT2(_Stack_Delete, T)

// If the stack has less space allocated than required for the count specified,
// allocate the amount of space. Otherwise it's a NOP.
//  bool Stack_Reserve(T)(Stack(T)* stack, size_t count)
#define Stack_Reserve(T)        _CONCAT2(_Stack_Reserve, T)

// Push a copy of the element onto the stack
//  bool Stack_Push(T)(Stack(T)* stack, const T* elem)
#define Stack_Push(T)           _CONCAT2(_Stack_Push, T)

// Push a copy of multiple elements onto the stack
//  bool Stack_PushMany(T)(Stack(T)* stack, const T elems[], size_t length)
#define Stack_PushMany(T)       _CONCAT2(_Stack_PushMany, T)

// Pop a copy of the element off the stack
//  bool Stack_Pop(T)(Stack(T)* stack, T* dest)
#define Stack_Pop(T)            _CONCAT2(_Stack_Pop, T)

// Pop multiple elements off the stack to a T[], where the first element is the top of the stack
// and the last element is the last element pop'd
//  size_t Stack_PopMany(T)(Stack(T)* stack, T dest[], size_t length)
#define Stack_PopMany(T)        _CONCAT2(_Stack_PopMany, T)

// Peek the top element off the stack
//  bool Stack_Peek(T)(Stack(T)* stack, T* dest)
#define Stack_Peek(T)           _CONCAT2(_Stack_Peek, T)

// Peek multiple elements off the stack
//  size_t Stack_PeekMany(T)(Stack(T)* stack, T dest[], size_t length)
#define Stack_PeekMany(T)       _CONCAT2(_Stack_PeekMany, T)

// Clear the stack, freeing the underlying buffer
//  void Stack_Clear(T)(Stack(T)* stack)
#define Stack_Clear(T)          _CONCAT2(_Stack_Clear, T)

// Shrink the underlying buffer to fit the current size of the stack
//  bool Stack_Shrink(T)(Stack(T)* stack)
#define Stack_Shrink(T)         _CONCAT2(_Stack_Shrink, T)

/* --------- END PUBLIC API ---------- */
#endif

#if !defined (REQ_PARAM_type)
#   error "Stack requires a type specialization"
#endif

#if !defined (OPT_PARAM_init_capacity)
#   define OPT_PARAM_init_capacity 16
#endif

#if !defined (OPT_PARAM_grow)
#   define OPT_PARAM_grow(old_size) (2 * old_size)
#endif

#if !defined (OPT_PARAM_malloc)
#   if !defined (_DEFAULT_ALLOCATOR)
#       define _DEFAULT_ALLOCATOR
#   endif

#   define OPT_PARAM_malloc(bytes) calloc(1, bytes)
#endif

#if !defined (OPT_PARAM_realloc)
#   if !defined (_DEFAULT_ALLOCATOR)
#       warning "Non-default malloc used with default realloc"
#   endif

#   define OPT_PARAM_realloc realloc
#endif

#if !defined (OPT_PARAM_free)
#   if !defined (_DEFAULT_ALLOCATOR)
#       warning "Non-default malloc used with default free"
#   endif

#   define OPT_PARAM_free free
#endif

#if defined(_DEFAULT_ALLOCATOR)
#   include <stdlib.h>
#endif

// useful macros internally

#define T REQ_PARAM_type
#define stack_init_capacity OPT_PARAM_init_capacity
#define stack_grow OPT_PARAM_grow
#define malloc OPT_PARAM_malloc
#define realloc OPT_PARAM_realloc
#define free OPT_PARAM_free

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))


// data types

typedef struct Stack(T) {
    T* base;
    T* head;
    size_t count;
    size_t capacity;
} Stack(T);

// functions

Stack(T)* Stack_New(T)()
{
    Stack(T)* stack = malloc(sizeof(Stack(T)));
    if (stack == NULL) {
        return NULL;
    }

    T* buffer = malloc(sizeof(T) * stack_init_capacity);
    if (stack == NULL) {
        return NULL;
    }

    stack->base = buffer;
    stack->head = buffer;
    stack->count = 0;
    stack->capacity = stack_init_capacity;

    return stack;
}

void Stack_Delete(T)(Stack(T)* stack)
{
    free(stack->base);
    free(stack);
}

bool Stack_Reserve(T)(Stack(T)* stack, size_t count)
{
    if (stack->capacity >= count) {
        return true;
    }

    T* newBuffer = realloc(stack->base, sizeof(T) * count);
    if (newBuffer == NULL) {
        return false;
    }

    stack->base = newBuffer;
    stack->head = &newBuffer[count];
    stack->capacity = count;

    return true;
}

bool Stack_PushMany(T)(Stack(T)* stack, const T elems[], size_t length)
{
    // see if we need to grow the buffer
    if (stack->count + length > stack->capacity) {
        size_t newCapacity;
        if (stack->capacity == 0) {
            newCapacity = max(stack->count + length, stack_init_capacity);
        } else {
            newCapacity = max(stack->count + length, stack_grow(stack->capacity));
        }

        T* newBuffer = realloc(stack->base, sizeof(T) * newCapacity);
        if (newBuffer == NULL) {
            return false;
        }

        stack->base = newBuffer;
        stack->head = &newBuffer[stack->count];
        stack->capacity = newCapacity;
    }

    for (size_t ii = 0; ii < length; ++ii) {
        stack->head[ii] = elems[ii];
    }

    stack->count += length;
    stack->head = &stack->base[stack->count];

    return true;
}

bool Stack_Push(T)(Stack(T)* stack, const T* elem)
{
    return Stack_PushMany(T)(stack, elem, 1);
}

size_t Stack_PeekMany(T)(Stack(T)* stack, T dest[], size_t length)
{
    size_t peekCount = min(length, stack->count);

    for (size_t ii = 0; ii < peekCount; ++ii) {
        dest[ii] = stack->base[stack->count - 1 - ii];
    }

    return peekCount;
}

size_t Stack_Peek(T)(Stack(T)* stack, T* dest)
{
    return Stack_PeekMany(T)(stack, dest, 1);
}

size_t Stack_PopMany(T)(Stack(T)* stack, T dest[], size_t length)
{
    size_t popCount = Stack_PeekMany(T)(stack, dest, length);

    stack->count -= popCount;
    stack->head = &stack->head[stack->count];

    return popCount;
}

size_t Stack_Pop(T)(Stack(T)* stack, T* dest)
{
    return Stack_PopMany(T)(stack, dest, 1);
}

void Stack_Clear(T)(Stack(T)* stack)
{
    free(stack->base);
    stack->base = NULL;
    stack->head = NULL;

    stack->count = 0;
    stack->capacity = 0;
}

bool Stack_Shrink(T)(Stack(T)* stack)
{
    if (stack->capacity == stack->count) {
        return true;
    }

    if (stack->count == 0) {
        free(stack->base);
        stack->base = NULL;
        stack->head = NULL;
    } else {
        size_t shrunkCapacity = sizeof(T) * stack->count;
        T* shrunkBuffer = realloc(stack->base, shrunkCapacity);
        if (shrunkBuffer == NULL) {
            return false;
        }
        stack->base = shrunkBuffer;
        stack->head = &shrunkBuffer[stack->count];
    }

    stack->capacity = stack->count;

    return true;
}

// cleanup macros
#undef T
#undef stack_init_capacity
#undef stack_grow
#undef malloc
#undef realloc
#undef free
#undef max
#undef min

#undef _DEFAULT_ALLOCATOR

#undef REQ_PARAM_type
#undef OPT_PARAM_init_capacity
#undef OPT_PARAM_grow
#undef OPT_PARAM_malloc
#undef OPT_PARAM_realloc
#undef OPT_PARAM_free
