// TODO: The code shared between Dict_EnumerateKeys and Dict_EnumerateValues should be commonized somehow
// TODO: Cleanup Dict_EnumerateKeys/Dict_EnumerateValues, the logic is confusing
// TODO: The Grow function shouldn't require a lookup when enumerating keys, provide another API to return the index
// TODO: Provide hash functions for all builtin types (float, pointers, char*, ints)
// TODO: Maybe Dict_Clear should reset the dict's allocation? Would be more in line with vector
// TODO: Deepcopy function

/* --------- PUBLIC API ---------- */

/* Include Parameters:
The type of the key for the dictionary:
    #define Dict_KeyType        Tkey
    #define Dict_KeyType_Alias Dict_KeyType

The type of the value stored in the dictionary:
    #define Dict_ValueType               Tval
    #define Dict_ValueType_Alias Dict_ValueType

A comparison function for the key type which returns true if the keys match and false otherwise
    #define Dict_CompareKey(key1, key2)  (key1 == key2)

A hash function for the key type which results in a size_t, a default hash function is provided for most integral types
    #define Dict_HashKey(key)            [See 'Provided hash functions']

The maximum load before the table is resized (out of 1024):
Default: 512/1024 (50%)
    #define Dict_MaxLoad 512

An allocator function (obeying ISO C's malloc/calloc semantics):
Default: malloc
    #define Dict_Malloc(bytes)           malloc(bytes)

A reallocator function (obeying ISO C's realloc semantics):
Default: realloc
    #define Dict_Realloc(ptr, bytes)     realloc(ptr, bytes)

A free function (obeying ISO C's free semantics):
Default: free
    #define Dict_Free(ptr)               free(ptr)

Provided hash functions:
    float, double   ->
    int, long, etc  -> xor, multiply, shift
    void*           ->
    char*           ->
*/
/* --------- END PUBLIC API ---------- */

#include <assert.h>
#include <immintrin.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if !defined(CTL_DICT_INCLUDED)
#    define CTL_DICT_INCLUDED
#    if !defined(CTL_CONCAT3_) && !defined(CTL_CONCAT3)
#        define CTL_CONCAT3_(a, b, c) a##_##b##_##c
#        define CTL_CONCAT3(a, b, c)  CTL_CONCAT3_(a, b, c)
#    endif

#    if !defined(CTL_OVERLOADABLE)
#        define CTL_OVERLOADABLE __attribute__((overloadable))
#    endif

#    if !defined(CTL_NEXT_POW2)
#        define CTL_NEXT_POW2(x)                                       \
            ({                                                         \
                typeof(x) _x = x;                                      \
                _x--;                                                  \
                _x |= _x >> 1;                                         \
                _x |= _x >> 2;                                         \
                _x |= sizeof(_x) >= sizeof(uint8_t) ? (_x >> 4) : 0;   \
                _x |= sizeof(_x) >= sizeof(uint16_t) ? (_x >> 8) : 0;  \
                _x |= sizeof(_x) >= sizeof(uint32_t) ? (_x >> 16) : 0; \
                _x |= sizeof(_x) >= sizeof(uint64_t) ? (_x >> 32) : 0; \
                _x++;                                                  \
                _x += (_x == 0);                                       \
                _x;                                                    \
            })
#    endif

#    define Dict(Tkey, Tval)            CTL_CONCAT3(Dict, Tkey, Tval)
#    define Dict_KeyGroup(Tkey, Tval)   CTL_CONCAT3(DictKeyGroup, Tkey, Tval)
#    define Dict_ValueGroup(Tkey, Tval) CTL_CONCAT3(DictValueGroup, Tkey, Tval)

#    define Dict_New(Tkey, Tval) CTL_CONCAT3(Dict_New, Tkey, Tval)
#endif

#if !defined(Dict_KeyType) || !defined(Dict_ValueType)
#    error "Dict template requires key and value types to be defined"
#endif

#if !defined(Dict_HashKey)
#    define Dict_HashKey(key) ((size_t)(key))
#endif

#if !defined(Dict_CompareKey)
#    define Dict_CompareKey(key1, key2) ((bool)(key1 == key2))
#endif

#if !defined(Dict_InitCapacity)
#    define Dict_InitCapacity 16
#endif

#if !defined(Dict_MaxLoad)
#    define Dict_MaxLoad 512
#endif

#if !defined(Dict_Malloc)
#    if !defined(Dict_DefaultAllocator)
#        define Dict_DefaultAllocator
#    endif
#    define Dict_Malloc(bytes) calloc(1, bytes)
#endif

#if !defined(Dict_Realloc)
#    if !defined(Dict_DefaultAllocator)
#        warning "Non-default malloc used with default realloc"
#    endif
#    define Dict_Realloc realloc
#endif

#if !defined(Dict_Free)
#    if !defined(Dict_DefaultAllocator)
#        warning "Non-default malloc used with default free"
#    endif
#    define Dict_Free free
#endif

#if defined(Dict_DefaultAllocator)
#    include <stdlib.h>
#endif

// useful macros internally

#define Tkey Dict_KeyType
#define Tval Dict_ValueType

#if !defined(Dict_KeyType_Alias)
#    define Tkey_ Tkey
#else
#    define Tkey_ Dict_KeyType_Alias
#endif

#if !defined(Dict_ValueType_Alias)
#    define Tval_ Tval
#else
#    define Tval_ Dict_ValueType_Alias
#endif

#if !defined(CTL_DICT_COMMON)
#    define CTL_DICT_COMMON

typedef union {
    __m128d  d128;
    __m128i  i128;
    uint64_t u64[2];
    uint32_t u32[4];
    uint16_t u16[8];
    uint8_t  u8[16];
} Dict_v128;

typedef union {
    struct {
        uint8_t hlow     : 7;
        uint8_t occupied : 1;
    };
    uint8_t u8;
} Dict_Metadata;

typedef union {
    Dict_Metadata slot[16];
    Dict_v128     v128;
} Dict_MetadataGroup;

static inline uint16_t Dict_CompareBitmask(Dict_MetadataGroup metadata, uint8_t expected) {
    __m128i  expectedVector = _mm_set1_epi8(expected);
    __m128i  comparison     = _mm_cmpeq_epi8(expectedVector, metadata.v128.i128);
    uint16_t mask           = _mm_movemask_epi8(comparison);

    return mask;
}

static inline uint16_t Dict_OccupiedBitmask(Dict_MetadataGroup metadata) {
    uint16_t mask = _mm_movemask_epi8(metadata.v128.i128);
    return mask;
}
#endif

typedef struct Dict_KeyGroup(Tkey_, Tval_) {
    Tkey key[16];
}
Dict_KeyGroup(Tkey_, Tval_);

typedef struct Dict_ValueGroup(Tkey_, Tval_) {
    Tval val[16];
}
Dict_ValueGroup(Tkey_, Tval_);

typedef struct Dict(Tkey_, Tval_) {
    size_t capacity;
    size_t size;
    Dict_KeyGroup(Tkey_, Tval_) * keyGroup;
    Dict_ValueGroup(Tkey_, Tval_) * valueGroup;
    Dict_MetadataGroup* metadataGroup;
}
Dict(Tkey_, Tval_);

CTL_OVERLOADABLE
static inline bool Dict_Grow(Dict(Tkey_, Tval_) * dict);

/**
 * @brief Initializes a dict for use
 * @param dict A pointer to the dict to initialize
 * @param capacity The initial capacity of the dict
 * @return True if the initialization succeeded, false otherwise
 * @note If capacity is not of the form 2^N * 16, it is rounded up to the next suitable form (e.g. 0 -> 16, 17 -> 32,
 * etc)
 */
CTL_OVERLOADABLE
static inline bool Dict_Init(Dict(Tkey_, Tval_) * dict, size_t capacity) {
    dict->capacity = 16 * CTL_NEXT_POW2(capacity / 16);
    dict->size     = 0;

    size_t groupCount        = dict->capacity / 16;
    size_t metadataGroupSize = groupCount * sizeof(Dict_MetadataGroup);
    size_t keyGroupSize      = groupCount * sizeof(Dict_ValueGroup(Tkey_, Tval_));
    size_t valueGroupSize    = groupCount * sizeof(Dict_KeyGroup(Tkey_, Tval_));

    void* block = Dict_Malloc(metadataGroupSize + keyGroupSize + valueGroupSize);
    if (block == NULL) {
        return false;
    }

    dict->metadataGroup = block;
    dict->keyGroup      = block + metadataGroupSize;
    dict->valueGroup    = block + metadataGroupSize + keyGroupSize;

    return true;
}

/**
 * @brief Allocates and initializes a dict on the heap
 * @param capacity The initial capacity of the dict
 * @return A pointer to the dict on the heap, or NULL if the allocation failed
 * @note If capacity is not of the form 2^N * 16, it is rounded up to the next suitable form (e.g. 0 -> 16, 17 -> 32,
 * etc)
 */
static inline Dict(Tkey_, Tval_) * Dict_New(Tkey_, Tval_)(size_t capacity) {
    Dict(Tkey_, Tval_)* dict = Dict_Malloc(sizeof(*dict));
    if (!Dict_Init(dict, capacity)) {
        Dict_Free(dict);
        return NULL;
    }

    return dict;
}

/**
 * @brief Uninitializes a dict, which then allows it to be discarded without leaking memory
 * @param dict The dict to uninitialize
 * @warning This function should only be used in conjunction with @ref Dict_Init
 */
CTL_OVERLOADABLE
static inline void Dict_Uninit(Dict(Tkey_, Tval_) * dict) {
    Dict_Free(dict->metadataGroup);
}

/**
 * @brief Deletes a dict that was allocated on the heap
 * @param dict The dict to delete
 * @warning This function should only be used in conjunction with @ref Dict_New
 */
CTL_OVERLOADABLE
static inline void Dict_Delete(Dict(Tkey_, Tval_) * dict) {
    Dict_Uninit(dict);
    Dict_Free(dict);
}

CTL_OVERLOADABLE
static inline bool
Dict_Find(Dict(Tkey_, Tval_) * dict, Tkey key, size_t hash, size_t* groupIndexOut, size_t* slotIndexOut) {
    size_t        groupIndex       = hash & (dict->capacity / 16 - 1);
    Dict_Metadata expectedMetadata = {.hlow = hash, .occupied = true};

    // look through the dict for a match
    // NOTE: we don't bother to provide a termination condition because the table should always have an empty entry
    while (true) {
        // compare a group at a time via SIMD (16 in one go)
        uint16_t mask = Dict_CompareBitmask(dict->metadataGroup[groupIndex], expectedMetadata.u8);

        // if bits are set in the mask, check each of the set bit positions (corresponding to positions in the group)
        for (int imask = mask, bitpos = 0; imask != 0; imask &= ~(1 << bitpos)) {
            bitpos = ffs(mask) - 1;
            if (Dict_CompareKey(key, dict->keyGroup[groupIndex].key[bitpos])) {
                *groupIndexOut = groupIndex;
                *slotIndexOut  = bitpos;
                return true;
            }
        }

        // if any were unoccupied, the entry must not be present in the dictionary
        uint16_t occupiedMask = Dict_OccupiedBitmask(dict->metadataGroup[groupIndex]);
        if (occupiedMask != 0xFFFF) {
            *groupIndexOut = groupIndex;
            *slotIndexOut  = occupiedMask;
            return false;
        }

        // go to next group
        // TODO: quadratic probing? might be better
        groupIndex = (groupIndex + 1) & (dict->capacity / 16 - 1);
    }
}

/**
 * @brief Looks up a value given a key, returns true if the key was found, false otherwise
 * @param dict The dictionary to search for the key
 * @param key The key to look for
 * @param outVal A pointer to where to write the value found at @param key, if found
 * @return True if @param key was found and the value was written to @param outVal, false otherwise
 */
CTL_OVERLOADABLE
static inline bool Dict_Get(Dict(Tkey_, Tval_) * dict, Tkey key, Tval* outVal) {
    size_t hash = Dict_HashKey(key);
    size_t groupIndex, slotIndex;
    if (Dict_Find(dict, key, hash, &groupIndex, &slotIndex)) {
        *outVal = dict->valueGroup[groupIndex].val[slotIndex];
        return true;
    }

    return false;
}

/**
 * @brief Stores a <key, value> pair to the dictionary, replacing existing instances if present
 * @param dict The dictionary to store to
 * @param key The key to act as the unique identifier for the value
 * @param val The value to store associated with key
 * @return True if the <key, value> pair was inserted successfully, false otherwise
 * @warning Current behavior is to not overwrite the key, this can have implications if two separate key allocations
 * with identical hashes are used and the dict is used as a way to track these allocations (e.g. 2 malloc'd char* =
 * "str", the 2nd insertion doesn't store the pointer to the dict, which if left un-free'd would be a leak)
 */
CTL_OVERLOADABLE
static inline bool Dict_Set(Dict(Tkey_, Tval_) * dict, Tkey key, Tval val) {
    size_t hash = Dict_HashKey(key);
    size_t groupIndex, slotIndex;
    if (Dict_Find(dict, key, hash, &groupIndex, &slotIndex)) {
        // key was found in the dict already, overwrite the value
        // TODO: should we overwrite the key? e.g. char* identical strings in different memory locations?
        dict->valueGroup[groupIndex].val[slotIndex] = val;
    } else {
        // key was not found in the dict already, we get the group index it should go in, but we need to determine the
        // next open slot from the slot mask
        size_t slotMask = slotIndex;
        int    bitpos   = ffs(~(uint16_t)slotMask) - 1;

        dict->valueGroup[groupIndex].val[bitpos] = val;
        dict->keyGroup[groupIndex].key[bitpos]   = key;

        dict->metadataGroup[groupIndex].slot[bitpos] = (Dict_Metadata){.hlow = hash, .occupied = true};
        dict->size += 1;
    }

    if (dict->size * 1024 > dict->capacity * Dict_MaxLoad) {
        if (!Dict_Grow(dict)) {
            // Growth allocation failed
            return false;
        }
    }

    return true;
}

/**
 * @brief Clears the dict of all elements
 * @param dict The dict to clear
 * @note This does not shrink the dict, it just clears all the memory and resets the size
 */
CTL_OVERLOADABLE
static inline void Dict_Clear(Dict(Tkey_, Tval_) * dict) {
    dict->size = 0;

    size_t groupCount        = dict->capacity / 16;
    size_t metadataGroupSize = groupCount * sizeof(Dict_MetadataGroup);
    size_t keyGroupSize      = groupCount * sizeof(Dict_ValueGroup(Tkey_, Tval_));
    size_t valueGroupSize    = groupCount * sizeof(Dict_KeyGroup(Tkey_, Tval_));

    memset(dict->metadataGroup, 0x00, metadataGroupSize + keyGroupSize + valueGroupSize);
}

/**
 * @brief Iteratively enumerate keys within the dict, returns a pointer to the next occupied key given an
 * existing key in the dict
 * @param dict The dictionary to enumerate
 * @param prevKey The previous key, passing NULL will give provide the first key in the dict
 * @return Returns NULL if @param prevKey was the last key in the dict, otherwise returns @param prevKey successor
 */
CTL_OVERLOADABLE
static inline Tkey* Dict_EnumerateKeys(Dict(Tkey_, Tval_) * dict, Tkey* prevKey) {
    if (prevKey == NULL) {
        if (dict->metadataGroup[0].slot[0].occupied) {
            // return the first one if no previous one was supplied and it's occupied
            return &dict->keyGroup[0].key[0];
        } else {
            // set prevKey to the first one since we know it's empty
            prevKey = &dict->keyGroup[0].key[0];
        }
    }

    // find the next occupied key
    uintptr_t firstKeyAddr = (uintptr_t)&dict->keyGroup[0].key[0];
    uintptr_t deltaBytes   = (uintptr_t)prevKey - firstKeyAddr;
    size_t    groupIndex   = deltaBytes / sizeof(Dict_KeyGroup(Tkey_, Tval_));
    size_t    slotIndex    = (deltaBytes - groupIndex * sizeof(Dict_KeyGroup(Tkey_, Tval_))) / sizeof(Tkey);

    while (true) {
        if (groupIndex == dict->capacity / 16) {
            return NULL;
        } else if (slotIndex == 16 && dict->metadataGroup[groupIndex].slot[0].occupied) {
            return &dict->keyGroup[groupIndex].key[0];
        }

        uint16_t occupiedMask     = Dict_OccupiedBitmask(dict->metadataGroup[groupIndex]);
        uint16_t clearMask        = ((1 << slotIndex) - 1) | (1 << slotIndex);
        uint16_t nextOccupiedMask = occupiedMask & ~clearMask;

        if (nextOccupiedMask) {
            int nextSlot = ffs(nextOccupiedMask) - 1;
            return &dict->keyGroup[groupIndex].key[nextSlot];
        }

        groupIndex += 1;
        slotIndex = 16;
    }
}

/**
 * @brief Iteratively enumerate values within the dict, returns a pointer to the next occupied value given an
 * existing value in the dict
 * @param dict The dictionary to enumerate
 * @param prevValue The previous value, passing NULL will give provide the first value in the dict
 * @return Returns NULL if @param prevValue was the last value in the dict, otherwise returns @param prevValue successor
 */
CTL_OVERLOADABLE
static inline Tval* Dict_EnumerateValues(Dict(Tkey_, Tval_) * dict, Tval* prevValue) {
    if (prevValue == NULL) {
        if (dict->metadataGroup[0].slot[0].occupied) {
            // return the first one if no previous one was supplied and it's occupied
            return &dict->valueGroup[0].val[0];
        } else {
            // set prevKey to the first one since we know it's empty
            prevValue = &dict->valueGroup[0].val[0];
        }
    }

    // find the next occupied key
    uintptr_t firstValueAddr = (uintptr_t)&dict->valueGroup[0].val[0];
    uintptr_t deltaBytes     = (uintptr_t)prevValue - firstValueAddr;
    size_t    groupIndex     = deltaBytes / sizeof(Dict_ValueGroup(Tkey_, Tval_));
    size_t    slotIndex      = (deltaBytes - groupIndex * sizeof(Dict_ValueGroup(Tkey_, Tval_))) / sizeof(Tval);

    while (true) {
        if (groupIndex == dict->capacity / 16) {
            return NULL;
        } else if (slotIndex == 16 && dict->metadataGroup[groupIndex].slot[0].occupied) {
            return &dict->valueGroup[groupIndex].val[0];
        }

        uint16_t occupiedMask     = Dict_OccupiedBitmask(dict->metadataGroup[groupIndex]);
        uint16_t clearMask        = ((1 << slotIndex) - 1) | (1 << slotIndex);
        uint16_t nextOccupiedMask = occupiedMask & ~clearMask;

        if (nextOccupiedMask) {
            int nextSlot = ffs(nextOccupiedMask) - 1;
            return &dict->valueGroup[groupIndex].val[nextSlot];
        }

        groupIndex += 1;
        slotIndex = 16;
    }
}

CTL_OVERLOADABLE
static inline bool Dict_Grow(Dict(Tkey_, Tval_) * dict) {
    Dict(Tkey_, Tval_) dictNew;
    if (!Dict_Init(&dictNew, 2 * dict->capacity)) {
        return false;
    }

    // TODO: provide a better dict copy that doesn't require a lookup
    Tkey* key = NULL;
    Tval  val;
    while ((key = Dict_EnumerateKeys(dict, key))) {
        Dict_Get(dict, *key, &val);
        Dict_Set(&dictNew, *key, val);
    }

    // free the old dict, copy the new one
    Dict_Uninit(dict);
    *dict = dictNew;

    return true;
}

// cleanup macros
#undef Dict_KeyType
#undef Dict_KeyType_Alias

#undef Dict_ValueType
#undef Dict_ValueType_Alias

#undef Dict_CompareKey
#undef Dict_HashKey
#undef Dict_InitCapacity
#undef Dict_MaxLoad

#undef Dict_Malloc
#undef Dict_Realloc
#undef Dict_Free
#undef Dict_DefaultAllocator

#undef Tkey
#undef Tval

#undef Tkey_
#undef Tval_
