/* --------- PUBLIC API ---------- */
// Include Parameters:
//  The type of the elements the stack should contain:
//      #define Stack_Type                 T
//
//  The growth function the stack should use when it needs to expand:
//      Default: 2 * old_size
//      #define Stack_Grow(old_size)        (2 * old_size)
//
//  An allocator function (obeying ISO C's malloc/calloc semantics):
//      Default: malloc
//      #define Stack_Malloc(bytes)        malloc(bytes)
//
//  A reallocator function (obeying ISO C's realloc semantics):
//      Default: realloc
//      #define Stack_Realloc(ptr, bytes)  realloc(ptr, bytes)
//
//  A free function (obeying ISO C's free semantics):
//      Default: free
//      #define Stack_Free(ptr)            free(ptr)
/* --------- END PUBLIC API ---------- */

#include <stdbool.h>
#include <stddef.h>

#if !defined(CTL_STACK_INCLUDED)
#    define CTL_STACK_INCLUDED
#    if !defined(CTL_CONCAT2_) && !defined(CTL_CONCAT2)
#        define CTL_CONCAT2_(a, b) a##_##b
#        define CTL_CONCAT2(a, b)  CTL_CONCAT2_(a, b)
#    endif

#    if !defined(CTL_OVERLOADABLE)
#        define CTL_OVERLOADABLE __attribute__((overloadable))
#    endif

#    define Stack(T)     CTL_CONCAT2(_Stack, T)
#    define Stack_New(T) CTL_CONCAT2(_Stack_New, T)

#endif

#if !defined(Stack_Type)
#    error "Stack requires a type specialization"
#endif

#if !defined(Stack_InitCapacity)
#    define Stack_InitCapacity 16
#endif

#if !defined(Stack_Grow)
#    define Stack_Grow(old_size) (2 * old_size)
#endif

#if !defined(Stack_Malloc)
#    if !defined(Stack_DefaultAllocator)
#        define Stack_DefaultAllocator
#    endif

#    define Stack_Malloc(bytes) calloc(1, bytes)
#endif

#if !defined(Stack_Realloc)
#    if !defined(Stack_DefaultAllocator)
#        warning "Non-default malloc used with default realloc"
#    endif

#    define Stack_Realloc realloc
#endif

#if !defined(Stack_Free)
#    if !defined(Stack_DefaultAllocator)
#        warning "Non-default malloc used with default free"
#    endif

#    define Stack_Free free
#endif

#if defined(Stack_DefaultAllocator)
#    include <stdlib.h>
#endif

// useful macros internally

#define T Stack_Type

#if !defined(Stack_Type_Alias)
#    define T_ T
#else
#    define T_ Stack_Type_Alias
#endif

#define Stack_Max(a, b)     \
    ({                      \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        _a > _b ? _a : _b;  \
    })

#define Stack_Min(a, b)     \
    ({                      \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        _a < _b ? _a : _b;  \
    })

// data types

typedef struct Stack(T_) {
    T*     base;
    T*     head;
    size_t count;
    size_t capacity;
}
Stack(T_);

// functions

/**
 * @brief Initializes a stack
 * @param stack A pointer to the stack to initialize
 * @param capacity The initial capacity of the stack
 * @return True if the initialization succeeded, false otherwise
 */
CTL_OVERLOADABLE
static inline bool Stack_Init(Stack(T_) * stack, size_t capacity) {
    T* buffer = Stack_Malloc(sizeof(T) * Stack_InitCapacity);
    if (stack == NULL) {
        return false;
    }

    stack->base     = buffer;
    stack->head     = buffer;
    stack->count    = 0;
    stack->capacity = capacity;

    return true;
}

/**
 * @brief Allocates and intiailizes a dict on the heap
 * @param capacity The initial capacity of the stack
 * @return A pointer to the stack on the heap, or NULL if the allocation failed
 */
static inline Stack(T_) * Stack_New(T_)(size_t capacity) {
    Stack(T_)* stack = Stack_Malloc(sizeof(Stack(T_)));
    if (stack == NULL) {
        return NULL;
    }

    if (!Stack_Init(stack, capacity)) {
        Stack_Free(stack);
        return NULL;
    }

    return stack;
}

/**
 * @brief Uninitializes a stack, which then allows it to be discarded without leaking memory
 * @param stack The stack to uninitialize
 * @warning This function should only be used in conjunction with @ref Stack_Init
 */
CTL_OVERLOADABLE
static inline void Stack_Uninit(Stack(T_) * stack) {
    Stack_Free(stack->base);
}

/**
 * @brief Deletes a stack that was allocated on the heap
 * @param stack The stack to delete
 * @warning This function should only be used in conjunction with @ref Stack_New
 */
CTL_OVERLOADABLE
static inline void Stack_Delete(Stack(T_) * stack) {
    Stack_Uninit(stack);
    Stack_Free(stack);
}

/**
 * @brief Reserves more space for the stack if @param capacity is > the stack's capacity, does nothing otherwise
 * @param stack The stack to reserve space for
 * @param capacity The desired capacity of the stack
 * @return True if space was reserved or if no allocation was required, returns False otherwise
 */
CTL_OVERLOADABLE
static inline bool Stack_Reserve(Stack(T_) * stack, size_t capacity) {
    if (stack->capacity >= count) {
        return true;
    }

    T* newBuffer = Stack_Realloc(stack->base, sizeof(T) * count);
    if (newBuffer == NULL) {
        return false;
    }

    stack->base     = newBuffer;
    stack->head     = &newBuffer[count];
    stack->capacity = count;

    return true;
}

/**
 * @brief Pushes @param length elements from @param elems to the stack in array order
 * @param stack The stack to push elements to
 * @param elems The array of elements to push to the stack
 * @param length The length of the array, also the number of elements to push to the stack
 * @return True if all the elements were pushed to the stack successfully, False otherwise
 * @note If the function returns False, the stack is left intact and no elements were pushed to the stack
 */
CTL_OVERLOADABLE
static inline bool Stack_PushMany(Stack(T_) * stack, T* elems, size_t length) {
    // see if we need to grow the buffer
    if (stack->count + length > stack->capacity) {
        size_t newCapacity;
        if (stack->capacity == 0) {
            newCapacity = Stack_Max(stack->count + length, Stack_InitCapacity);
        } else {
            newCapacity = Stack_Max(stack->count + length, Stack_Grow(stack->capacity));
        }

        T* newBuffer = Stack_Realloc(stack->base, sizeof(T) * newCapacity);
        if (newBuffer == NULL) {
            return false;
        }

        stack->base     = newBuffer;
        stack->head     = &newBuffer[stack->count];
        stack->capacity = newCapacity;
    }

    for (size_t ii = 0; ii < length; ++ii) {
        stack->head[ii] = elems[ii];
    }

    stack->count += length;
    stack->head = &stack->base[stack->count];

    return true;
}

/**
 * @brief Pushes a single element to the stack
 * @param stack The stack to push an element to
 * @param elem The element to push to the stack
 * @return True if the element was pushed to the stack successfully, False otherwise
 */
CTL_OVERLOADABLE
static inline bool Stack_Push(Stack(T_) * stack, T* elem) {
    return Stack_PushMany(stack, elem, 1);
}

/**
 * @brief Read @param length number of elements from the top of the stack in popped order to @param dest without
 * removing them from the stack
 * @param stack The stack to peek elements from
 * @param dest The destination to peek elements to
 * @param length The length of the array, also the number of elements to peek off the stack
 * @return The number of elements peek'd from the stack,
 * @note The value returned may be smaller than @param length if the stack has fewer elements than @param length
 */
CTL_OVERLOADABLE
static inline size_t Stack_PeekMany(Stack(T_) * stack, T* dest, size_t length) {
    size_t peekCount = Stack_Min(length, stack->count);

    for (size_t ii = 0; ii < peekCount; ++ii) {
        dest[ii] = stack->base[stack->count - 1 - ii];
    }

    return peekCount;
}

/**
 * @brief Peek a single element from the top of the stack
 * @param stack The stack to peek an element from
 * @param dest The destination of the peek'd element
 * @return The number of elements peek'd
 * @note The value returned is 0 if no elements were present on the stack, 1 if an element was present
 */
CTL_OVERLOADABLE
static inline size_t Stack_Peek(Stack(T_) * stack, T* dest) {
    return Stack_PeekMany(stack, dest, 1);
}

/**
 * @brief Pops @param length elements from @param stack into @param dest in pop order
 * @param stack The stack to pop elements from
 * @param dest The destination array for the elements popped ([0] = top element, [1] = next, etc.)
 * @param length The length of the array, also the number of elements to pop from the stack
 * @return The number of elements popped from the stack
 * @note The value returned may be < @param length if fewer elements were on the stack than @param length
 */
CTL_OVERLOADABLE
static inline size_t Stack_PopMany(Stack(T_) * stack, T* dest, size_t length) {
    size_t popCount = Stack_PeekMany(stack, dest, length);

    stack->count -= popCount;
    stack->head = &stack->base[stack->count];

    return popCount;
}

/**
 * @brief Pops a single element from @param stack into @param dest
 * @param stack The stack to pop elements from
 * @param dest Where to store the popped element
 * @return The number of elements popped
 * @note The value returned is 1 if an element was present on the stack, 0 if the stack was empty
 */
CTL_OVERLOADABLE
static inline size_t Stack_Pop(Stack(T_) * stack, T* dest) {
    return Stack_PopMany(stack, dest, 1);
}

/**
 * @brief Clears the internal stack contents, freeing up memory while keeping the stack in a usable state
 * @param stack The stack to clear of elements and free internally
 */
CTL_OVERLOADABLE
static inline void Stack_Clear(Stack(T_) * stack) {
    Stack_Free(stack->base);
    stack->base = NULL;
    stack->head = NULL;

    stack->count    = 0;
    stack->capacity = 0;
}

/**
 * @brief Shrink the internal stack buffer to the minimum size capable of storing the stack's contents
 * @param stack The stack to shrink the memory of
 * @return True if the shrink succeeded, False otherwise
 */
CTL_OVERLOADABLE
static inline bool Stack_Shrink(Stack(T_) * stack) {
    if (stack->capacity == stack->count) {
        return true;
    }

    if (stack->count == 0) {
        Stack_Free(stack->base);
        stack->base = NULL;
        stack->head = NULL;
    } else {
        size_t shrunkCapacity = sizeof(T) * stack->count;
        T*     shrunkBuffer   = Stack_Realloc(stack->base, shrunkCapacity);
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
#undef T_

#undef Stack_Max
#undef Stack_Min

#undef Stack_Type
#undef Stack_InitCapacity
#undef Stack_Grow
#undef Stack_Malloc
#undef Stack_Realloc
#undef Stack_Free
