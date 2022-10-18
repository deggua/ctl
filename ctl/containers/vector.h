// TODO: Add an overload for passing single elements by value
// TODO: Small vector optimization
// TODO: Deepcopy function

/* --------- PUBLIC API ---------- */

// Include Parameters:
//  The type of the elements the vector should contain:
//      #define Vector_Type         T
//
//  The name to use to reference the type specific macros:
//      Default: Vector_Type
//      #define Vector_Type_Alias   CustomName
//
//  The growth function the vector should use when it needs to expand:
//      Default: (3 * old_size + 1) / 2
//      #define Vector_Grow(old_size)         (3 * old_size + 1) / 2
//
//  An allocator function (obeying ISO C's malloc/calloc semantics):
//      Default: malloc
//      #define Vector_Malloc(bytes)          malloc(bytes)
//
//  A reallocator function (obeying ISO C's realloc semantics):
//      Default: realloc
//      #define Vector_Realloc(ptr, bytes)    realloc(ptr, bytes)
//
//  A free function (obeying ISO C's free semantics):
//      Default: free
//      #define Vector_Free(ptr)              free(ptr)

#include <stdbool.h>
#include <stddef.h>

#if !defined(CTL_VECTOR_INCLUDED)
#    define CTL_VECTOR_INCLUDED
#    if !defined(CTL_CONCAT2)
#        define CTL_CONCAT2_(a, b) a##_##b
#        define CTL_CONCAT2(a, b)  CTL_CONCAT2_(a, b)
#    endif

#    if !defined(CTL_OVERLOADABLE)
#        define CTL_OVERLOADABLE __attribute__((overloadable))
#    endif

#    define Vector(T)     CTL_CONCAT2(Vector, T)
#    define Vector_New(T) CTL_CONCAT2(Vector_New, T)

/* --------- END PUBLIC API ---------- */
#endif

#if !defined(Vector_Type)
#    error "Vector requires a type specialization"
#endif

#if !defined(Vector_Type_Alias)
#    define Vector_Type_Alias Vector_Type
#endif

#if !defined(Vector_Init_Capacity)
#    define Vector_Init_Capacity 16
#endif

#if !defined(Vector_Grow)
#    define Vector_Grow(old_size) ((3 * old_size + 1) / 2)
#endif

#if !defined(Vector_Malloc)
#    if !defined(CTL_DEFAULT_ALLOCATOR)
#        define CTL_DEFAULT_ALLOCATOR
#    endif

#    define Vector_Malloc(bytes) calloc(1, bytes)
#endif

#if !defined(Vector_Realloc)
#    if !defined(CTL_DEFAULT_ALLOCATOR)
#        warning "Non-default malloc used with default realloc"
#    endif

#    define Vector_Realloc realloc
#endif

#if !defined(Vector_Free)
#    if !defined(CTL_DEFAULT_ALLOCATOR)
#        warning "Non-default malloc used with default free"
#    endif

#    define Vector_Free free
#endif

#if defined(CTL_DEFAULT_ALLOCATOR)
#    include <stdlib.h>
#endif

#define T                Vector_Type
#define T_               Vector_Type_Alias
#define Vector_Max(a, b) ((a) > (b) ? (a) : (b))

typedef struct Vector(T_) {
    T*     at;
    size_t length;
    size_t capacity;
}
Vector(T_);

/**
 * @brief Initialize a vector for use
 * @param vec The vector to initialize
 * @param capacity The initial capacity for the vector
 * @return True if the initialization succeeded, false otherwise
 */
static inline bool Vector_Init(Vector(T_) * vec, size_t capacity) {
    T* buffer = Vector_Malloc(sizeof(T) * capacity);

    if (buffer == NULL) {
        return false;
    }

    vec->length   = 0;
    vec->capacity = capacity;
    vec->at       = buffer;

    return true;
}

/**
 * @brief Allocate a new vector and initialize it
 * @param capacity The initial capacity of the vector
 * @return A pointer to the vector
 */
static inline Vector(T_) * Vector_New(T_)(size_t capacity) {
    Vector(T_)* vec = Vector_Malloc(sizeof(Vector(T_)));
    if (vec == NULL) {
        return NULL;
    }

    if (!Vector_Init(vec, capacity)) {
        Vector_Free(vec);
        return NULL;
    }

    return vec;
}

/**
 * @brief Uninitialize a vector
 * @param vec The vector to uninitialize
 * @warning This should only be used in conjunction with @ref Vector_Init
 */
CTL_OVERLOADABLE
static inline void Vector_Uninit(Vector(T_) * vec) {
    Vector_Free(vec->at);
}

/**
 * @brief Deletes a vector
 * @param vec The vector to delete
 * @warning This should only be used in conjunction with @ref Vector_New
 */
CTL_OVERLOADABLE
static inline void Vector_Delete(Vector(T_) * vec) {
    Vector_Uninit(vec);
    Vector_Free(vec);
}

CTL_OVERLOADABLE
static inline bool Vector_GrowTo(Vector(T_) * vec, size_t length) {
    T* newBuffer = realloc(vec->at, sizeof(T) * length);

    if (newBuffer == NULL) {
        return false;
    }

    vec->at       = newBuffer;
    vec->capacity = length;

    return true;
}

/**
 * @brief Reserve enough memory for @param length number of elements, if the vector can already hold @param length
 * number of elements do nothing
 * @param vec The vector to reserve space for
 * @param length The length of the vector to reserve memory for
 * @return True if the the vector was able to reserve enough space, false otherwise
 */
CTL_OVERLOADABLE
static inline bool Vector_Reserve(Vector(T_) * vec, size_t length) {
    if (vec->capacity >= length) {
        return true;
    }

    return Vector_GrowTo(T_)(vec, length);
}

/**
 * @brief Push @param length number of elements from @param elems to the end of @param vec in array order
 * @param vec The vector to push elements to
 * @param elems The source of the elements
 * @param length The length of @param elems which get pushed to the vector
 * @return True if the operation succeeded, false otherwise
 * @warning You cannot push from a vector to itself unless you guarantee it won't need to resize
 */
CTL_OVERLOADABLE
static inline bool Vector_PushMany(Vector(T_) * vec, T elems[], size_t length) {
    // check if we need to grow the vector
    if (vec->capacity < vec->length + length) {
        size_t newCapacity;

        if (vec->capacity == 0) {
            newCapacity = Vector_Max(vec->length + length, Vector_Init_Capacity);
        } else {
            newCapacity = Vector_Max(vec->length + length, Vector_Grow(vec->capacity));
        }

        if (!Vector_GrowTo(vec, newCapacity)) {
            return false;
        }
    }

    for (size_t ii = 0; ii < length; ++ii) {
        vec->at[vec->length + ii] = elems[ii];
    }

    vec->length += length;

    return true;
}

/**
 * @brief Push a single element from @param elem to the end of @param vec
 * @param vec The vector to push an element on to
 * @param elem A pointer to the element to push to @param vec
 * @return True if the operation succeeded, false otherwise
 * @warning You cannot push from a vector to itself unless you guarantee it won't need to resize
 */
CTL_OVERLOADABLE
static inline bool Vector_Push(Vector(T_) * vec, T* elem) {
    return Vector_PushMany(vec, elem, 1);
}

/**
 * @brief Extend a vector's size by some number of elements, essentially allocating in-place
 * @param vec The vector to extend
 * @param size The size to extend the vector by (i.e vec->length += length)
 * @return The index (> 0, if the operation succeeded) of the first of the extended elements, otherwise -1
 * @note Typical usage for this would be to know the index of an element in the vector before constructing it, useful
 * for vector based data structures that use indices to refer to other nodes
 */
CTL_OVERLOADABLE
static inline ssize_t Vector_Extend(Vector(T_) * vec, size_t length) {
    if (Vector_Reserve(vec, vec->length + length)) {
        size_t oldLength = vec->length;
        vec->length += length;
        return oldLength;
    } else {
        return -1;
    }
}

/**
 * @brief Insert @param length number of elements from @param elems at @param index in @param vec in array order
 * @param vec The vector to insert elements into
 * @param index The index to insert the elements at
 * @param elems The source of elements to insert in the vector
 * @param length The number of elements in @param elems to insert
 * @return True if the operation succeeded, false otherwise
 * @warning You cannot insert from a vector to itself
 */
CTL_OVERLOADABLE
static inline bool Vector_InsertMany(Vector(T_) * vec, size_t index, T elems[], size_t length) {
    if (length == 0) {
        // nop
        return true;
    } else if (index == vec->length) {
        // equivalent to a push many
        return Vector_PushMany(vec, elems, length);
    } else if (index > vec->length) {
        // can't insert past the end of the vector
        return false;
    }

    // check if we need to grow the vector
    if (vec->capacity < vec->length + length) {
        size_t newCapacity;

        if (vec->capacity == 0) {
            newCapacity = Vector_Max(vec->length + length, Vector_Init_Capacity);
        } else {
            newCapacity = Vector_Max(vec->length + length, Vector_Grow(vec->capacity));
        }

        if (!Vector_GrowTo(vec, newCapacity)) {
            return false;
        }
    }

    // copy elements (starting at the end) to their new position at the end of the vector
    for (size_t ii = vec->length + length - 1; ii > index + length - 1; --ii) {
        vec->at[ii] = vec->at[ii - length];
    }

    // copy elements from elems to their new position starting at the end of their section in the vector
    for (size_t ii = index + length - 1; ii > index; --ii) {
        vec->at[ii] = elems[ii - index];
    }

    // copy first element in elems to index
    vec->at[index] = elems[0];

    vec->length += length;

    return true;
}

/**
 * @brief Insert @param elem into @param vec at @param index
 * @param vec The vector to insert elements into
 * @param index The index to insert the elements at
 * @param elem The element to insert
 * @return True if the operation succeeded, false otherwise
 * @warning You cannot insert from a vector to itself
 */
CTL_OVERLOADABLE
static inline bool Vector_Insert(Vector(T_) * vec, size_t index, T* elem) {
    return Vector_InsertMany(vec, index, elem, 1);
}

/**
 * @brief Shrink a vector's buffer to fit its current size, reducing memory usage
 * @param vec The vector to shrink
 * @return True if the operation succeeded, false otherwise
 */
CTL_OVERLOADABLE
static inline bool Vector_Shrink(Vector(T_) * vec) {
    // nop if it's already shrunk to size
    if (vec->capacity == vec->length) {
        return true;
    }

    // needs special handling for resizes to 0, realloc basically acts as a free in that case
    // which implies that the old memory is invalidated, the problem is realloc returns NULL,
    // so we can't distinguish whether we did a realloc(ptr, 0) or realloc failed unless we check
    // before we call realloc
    if (vec->length == 0) {
        Vector_Free(vec->at);
        vec->at = NULL;
    } else {
        size_t shrunkCapacity = sizeof(T) * vec->length;
        T*     shrunkBuffer   = Vector_Realloc(vec->at, shrunkCapacity);

        if (shrunkBuffer == NULL) {
            return false;
        }

        vec->at = shrunkBuffer;
    }

    vec->capacity = vec->length;

    return true;
}

/**
 * @brief Clear a vector of all elements, freeing its buffer and reducing its memory consumption
 * @param vec The vector to clear
 * @return True if the operation succeeded, false otherwise
 */
CTL_OVERLOADABLE
static inline bool Vector_Clear(Vector(T_) * vec) {
    Vector_Free(vec->at);
    vec->at = NULL;

    vec->length   = 0;
    vec->capacity = 0;

    return true;
}

/**
 * @brief Remove a slice of elements from the vector given an index range of the form [start, stop)
 * @param vec The vector to remove elements from
 * @param start The first element index of the range which will be removed
 * @param stop The last element index of the range which will NOT be removed
 */
CTL_OVERLOADABLE
static inline void Vector_RemoveRange(Vector(T_) * vec, size_t start, size_t stop) {
    if (start > vec->length - 1 || stop > vec->length || stop - start == 0) {
        // nop
        return;
    }

    for (size_t old_ii = stop, new_ii = start; old_ii < vec->length; ++old_ii, ++new_ii) {
        vec->at[new_ii] = vec->at[old_ii];
    }

    vec->length -= stop - start;
}

/**
 * @brief Remove a single element at a specified index from the vector
 * @param vec The vector to remove elements from
 * @param index The index of the element to remove from the vector
 */
CTL_OVERLOADABLE
static inline void Vector_Remove(Vector(T_) * vec, size_t index) {
    Vector_RemoveRange(vec, index, index + 1);
}

/**
 * @brief Pop @param len number of elements from the end of @param vec to @param dest in popped order
 * @param vec The vector to pop from
 * @param dest The destination array of the popped elements
 * @param len The number of elements to pop from the vector
 * @return True if the operation succeeded, false otherwise
 * @note Popped order implies that for a vector {e0, e1, ..., eN} that dest will recieve the elements as {eN, eN-1, ...,
 * e0} (for len = N)
 */
CTL_OVERLOADABLE
static inline bool Vector_PopMany(Vector(T_) * vec, T dest[], size_t len) {
    if (len > vec->length) {
        return false;
    }

    for (size_t ii = 0; ii < len; ii++) {
        dest[ii] = vec->at[vec->length - 1 - ii];
    }

    vec->length -= len;
    return true;
}

/**
 * @brief Pop a single element from @param vec to @param dest
 * @param vec The vector to pop from
 * @param dest The destination of the element
 * @return True if the operation succeeded, false otherwise
 */
CTL_OVERLOADABLE
static inline bool Vector_Pop(Vector(T_) * vec, T* dest) {
    return Vector_PopMany(vec, dest, 1);
}

// cleanup macros
#undef T
#undef T_
#undef Vector_Init_Capacity
#undef Vector_Grow
#undef Vector_Malloc
#undef Vector_Realloc
#undef Vector_Free
#undef Vector_Max

#undef CTL_DEFAULT_ALLOCATOR

#undef Vector_Type
#undef Vector_Type_Alias
#undef Vector_Init_Capacity
#undef Vector_Grow
#undef Vector_Malloc
#undef Vector_Realloc
#undef Vector_Free
