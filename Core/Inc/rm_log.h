/**
 ******************************************************************************
 * @file    rm_log.h
 * @brief   Minimal-intrusion real-time log ring buffer for OpenOCD-based tuning.
 *
 *          One header, one RMLOG() macro.  No separate entries file needed.
 *
 *          Usage:
 *            1. Copy this folder to your firmware project's lib/rm_log/.
 *            2. Add lib/rm_log/rm_log.c to your firmware build.
 *            3. In any file: #include "lib/rm_log/rm_log.h"
 *            4. RMLOG(name, value) or RM_LOG(variable) — that's it.
 *
 *          Design:
 *            - RMLOG expands to an inline function call with the #name string.
 *            - At runtime, linear-search the entry table by name → write value.
 *            - Each entry owns an independent tiny ring buffer with per-entry seq.
 *            - Backend reads "rm_log_area" symbol via ELF → header + entry meta.
 ******************************************************************************
 */

#ifndef RM_LOG_H
#define RM_LOG_H

#include <stdint.h>
#ifdef __cplusplus
#include <type_traits>
#endif

/* ===================================================================
 * Configuration (override with #define before #include, or edit here)
 * =================================================================== */

#ifndef RM_LOG_MAX_ENTRIES
#define RM_LOG_MAX_ENTRIES  12
#endif

#ifndef RM_LOG_FRAME_COUNT
#define RM_LOG_FRAME_COUNT  10
#endif

#ifndef RM_LOG_MAX_NAME_LEN
#define RM_LOG_MAX_NAME_LEN 24
#endif

#define RM_LOG_MAGIC    0x524D4C47UL   /* 'RMLG' */

/* Library release version: for humans and documentation. */
#define RM_LOG_LIB_VERSION_MAJOR 0
#define RM_LOG_LIB_VERSION_MINOR 2
#define RM_LOG_LIB_VERSION_PATCH 0
#define RM_LOG_LIB_VERSION_STR   "0.2.0"

/* Protocol/layout version: Scope checks this against rm_log_area.header.version. */
#define RM_LOG_VERSION  1UL

/* Compiler barrier: prevents reordering of stores across this point.
   On ARM, also flushes the write buffer so SWD/Host sees completed stores.
   Falls back to compiler-only barrier if CMSIS is not available. */
#ifndef __DSB
#define _RM_BARRIER() __asm__ __volatile__("" ::: "memory")
#else
#define _RM_BARRIER() __DSB()
#endif

#define RM_LOG_TYPE_FLOAT32  0
#define RM_LOG_TYPE_UINT32   1
#define RM_LOG_TYPE_INT32    2
#define RM_LOG_TYPE_UINT16   3
#define RM_LOG_TYPE_INT16    4
#define RM_LOG_TYPE_UINT8    5
#define RM_LOG_TYPE_INT8     6

/* ===================================================================
 * Public types
 * =================================================================== */

typedef struct {
    char     name[RM_LOG_MAX_NAME_LEN];  /* null-terminated entry name */
    uint32_t type;                       /* RMLogType (RM_LOG_TYPE_FLOAT32 etc) */
    uint32_t write_seq;                  /* incremented on each write */
    /* Ring buffer stores raw 32-bit words.  Host decodes via *type* field. */
    uint32_t values[RM_LOG_FRAME_COUNT];
} RMLogEntry;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t entry_count;
    uint32_t frame_count;
    uint32_t max_name_len;
} RMLogHeader;

typedef struct {
    RMLogHeader header;
    RMLogEntry  entries[RM_LOG_MAX_ENTRIES];
} RMLogArea;

/* Defined in rm_log.c */
#ifdef __cplusplus
extern "C" {
#endif
extern RMLogArea rm_log_area;
#ifdef __cplusplus
}
#endif

/* ===================================================================
 * Internal: name-based lookup + write
 * =================================================================== */

/* Copy string into dst (max RM_LOG_MAX_NAME_LEN-1 chars + null). */
static inline void _rm_strcpy(char* dst, const char* src) {
    uint32_t j;
    for (j = 0; src[j] && j < (uint32_t)(RM_LOG_MAX_NAME_LEN - 1); ++j)
        dst[j] = src[j];
    dst[j] = '\0';
}

/* Name comparison. */
static inline int _rm_str_eq(const char* a, const char* b) {
    while (*a && (*a == *b)) { ++a; ++b; }
    return *a == *b;  /* both hit '\0' simultaneously */
}

static inline uint32_t _rm_log_raw_float32(float val) {
    union {
        float f;
        uint32_t u;
    } cvt;
    cvt.f = val;
    return cvt.u;
}

static inline uint32_t _rm_log_raw_uint32(uint32_t val) {
    return val;
}

static inline uint32_t _rm_log_raw_int32(int32_t val) {
    return (uint32_t)val;
}

static inline uint32_t _rm_log_raw_uint16(uint16_t val) {
    return (uint32_t)val;
}

static inline uint32_t _rm_log_raw_int16(int16_t val) {
    return (uint32_t)(int32_t)val;
}

static inline uint32_t _rm_log_raw_uint8(uint8_t val) {
    return (uint32_t)val;
}

static inline uint32_t _rm_log_raw_int8(int8_t val) {
    return (uint32_t)(int32_t)val;
}

static inline void _rm_log_write_raw(const char* name, uint32_t type, uint32_t raw) {
    const uint32_t n = rm_log_area.header.entry_count;
    uint32_t i;

    /* 1. Search existing entries */
    for (i = 0; i < n; ++i) {
        if (_rm_str_eq(name, rm_log_area.entries[i].name)) {
            const uint32_t pos = rm_log_area.entries[i].write_seq
                                 % (uint32_t)RM_LOG_FRAME_COUNT;
            /* Write value BEFORE incrementing seq — Host checks seq to
               determine if new data is available.  Barrier ensures the
               value store completes before seq becomes visible to SWD. */
            rm_log_area.entries[i].values[pos] = raw;
            _RM_BARRIER();
            rm_log_area.entries[i].write_seq++;
            return;
        }
    }

    /* 2. Auto-register new entry */
    if (i < (uint32_t)RM_LOG_MAX_ENTRIES) {
        _rm_strcpy(rm_log_area.entries[i].name, name);
        rm_log_area.entries[i].type = type;
        rm_log_area.entries[i].write_seq = 1;
        rm_log_area.entries[i].values[0] = raw;
        /* Barrier ensures all entry fields are visible via SWD before
           entry_count is incremented — prevents Host from seeing a
           partially initialised entry. */
        _RM_BARRIER();
        rm_log_area.header.entry_count = i + 1;
    }
}

static inline void _rm_log_write_float32(const char* name, float val) {
    _rm_log_write_raw(name, RM_LOG_TYPE_FLOAT32, _rm_log_raw_float32(val));
}

static inline void _rm_log_write_uint32(const char* name, uint32_t val) {
    _rm_log_write_raw(name, RM_LOG_TYPE_UINT32, _rm_log_raw_uint32(val));
}

static inline void _rm_log_write_int32(const char* name, int32_t val) {
    _rm_log_write_raw(name, RM_LOG_TYPE_INT32, _rm_log_raw_int32(val));
}

static inline void _rm_log_write_uint16(const char* name, uint16_t val) {
    _rm_log_write_raw(name, RM_LOG_TYPE_UINT16, _rm_log_raw_uint16(val));
}

static inline void _rm_log_write_int16(const char* name, int16_t val) {
    _rm_log_write_raw(name, RM_LOG_TYPE_INT16, _rm_log_raw_int16(val));
}

static inline void _rm_log_write_uint8(const char* name, uint8_t val) {
    _rm_log_write_raw(name, RM_LOG_TYPE_UINT8, _rm_log_raw_uint8(val));
}

static inline void _rm_log_write_int8(const char* name, int8_t val) {
    _rm_log_write_raw(name, RM_LOG_TYPE_INT8, _rm_log_raw_int8(val));
}

#ifdef __cplusplus
template <typename T>
static inline typename std::enable_if<std::is_floating_point<T>::value, void>::type
_rm_log_write_auto(const char* name, T val) {
    _rm_log_write_float32(name, (float)val);
}

template <typename T>
static inline typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, void>::type
_rm_log_write_auto(const char* name, T val) {
    if (sizeof(T) <= 1) {
        _rm_log_write_int8(name, (int8_t)val);
    } else if (sizeof(T) <= 2) {
        _rm_log_write_int16(name, (int16_t)val);
    } else {
        _rm_log_write_int32(name, (int32_t)val);
    }
}

template <typename T>
static inline typename std::enable_if<std::is_integral<T>::value && !std::is_signed<T>::value, void>::type
_rm_log_write_auto(const char* name, T val) {
    if (sizeof(T) <= 1) {
        _rm_log_write_uint8(name, (uint8_t)val);
    } else if (sizeof(T) <= 2) {
        _rm_log_write_uint16(name, (uint16_t)val);
    } else {
        _rm_log_write_uint32(name, (uint32_t)val);
    }
}
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define _rm_log_write_auto(name, val) \
    _Generic((val), \
        float: _rm_log_write_float32, \
        double: _rm_log_write_float32, \
        uint8_t: _rm_log_write_uint8, \
        int8_t: _rm_log_write_int8, \
        uint16_t: _rm_log_write_uint16, \
        int16_t: _rm_log_write_int16, \
        uint32_t: _rm_log_write_uint32, \
        int32_t: _rm_log_write_int32, \
        default: _rm_log_write_float32 \
    )(name, val)
#else
#define _rm_log_write_auto(name, val) _rm_log_write_float32((name), (float)(val))
#endif

/* ===================================================================
 * Public macro
 * =================================================================== */

#define RMLOG(name, val)  _rm_log_write_auto(#name, (val))
#define RM_LOG(val)       _rm_log_write_auto(#val, (val))

/* Belt-and-suspenders init (all data is statically initialized in rm_log.c).
   Call once at startup if you want, but not required. */
#ifdef __cplusplus
extern "C" {
#endif
void rm_log_init(void);
#ifdef __cplusplus
}
#endif

/* ===================================================================
 * Backend protocol (reference)
 * ===================================================================
 *
 * Connect:
 *   1. ELF resolve "rm_log_area" → base address A
 *   2. mdw A (5 words) → magic, version, entry_count, frame_count, max_name_len
 *   3. Verify magic == 0x524D4C47
 *   4. For i in 0..entry_count-1:
 *        Read RMLogEntry at A + 20 + i * sizeof(RMLogEntry)
 *        → name (max_name_len bytes), write_seq (4B), values (4B * frame_count)
 *   5. Store entry_names[i], last_seq[i] = write_seq
 *
 * Poll:
 *   1. For each entry i:
 *        seq = mdw(address_of(entries[i].write_seq))
 *        if seq > last_seq[i]:
 *            pos = (seq - 1) % frame_count
 *            val = mdw(address_of(entries[i].values[pos]))
 *            emit { name, value }
 *            last_seq[i] = seq
 *
 * Memory layout (contiguous, one ELF symbol):
 *   [RMLogHeader: 5 × uint32 = 20B]
 *   [RMLogEntry[0]: max_name_len + 4 + 4*frame_count]
 *   [RMLogEntry[1]: ...]
 *   ...
 */

#endif /* RM_LOG_H */
