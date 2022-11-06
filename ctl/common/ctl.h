#pragma once

#include "pp_magic.h"

#define CTL_OVERLOADABLE __attribute__((overloadable))

#define CTL_NEXT_POW2(x)                                       \
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

#define CTL_MAX(x, y)      \
    ({                     \
        typeof(x) _x = x;  \
        typeof(y) _y = y;  \
        _x > _y ? _x : _y; \
    })

#define CTL_MIN(x, y)      \
    ({                     \
        typeof(x) _x = x;  \
        typeof(y) _y = y;  \
        _x < _y ? _x : _y; \
    })
