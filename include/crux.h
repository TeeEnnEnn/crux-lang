#ifndef CRUX_H
#define CRUX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Types --- */

/**
 * @brief Opaque handle to a Crux Virtual Machine instance.
 */
typedef struct CruxVM CruxVM;

/**
 * @brief Represents a value in the Crux language (NaN-boxed 64-bit value).
 */
typedef uint64_t CruxValue;

/**
 * @brief Results from interpreting Crux source code.
 */
typedef enum {
    CRUX_INTERPRET_OK = 0,
    CRUX_INTERPRET_COMPILE_ERROR = 1,
    CRUX_INTERPRET_RUNTIME_ERROR = 2,
    CRUX_INTERPRET_EXIT = 3
} CruxInterpretResult;

/* --- CruxVM Management --- */

/**
 * @brief Creates and initializes a new Crux Virtual Machine.
 * @param argc Number of command line arguments to pass to the CruxVM.
 * @param argv Array of command line argument strings.
 * @return A pointer to the new CruxVM instance, or NULL if initialization failed.
 */
CruxVM* crux_vm_new(int argc, const char** argv);

/**
 * @brief Frees all memory associated with a Crux Virtual Machine.
 * @param vm The CruxVM instance to destroy.
 */
void crux_vm_free(CruxVM* vm);

/* --- Execution --- */

/**
 * @brief Interprets and executes Crux source code from a string.
 * @param vm The CruxVM instance.
 * @param source The null-terminated source code string.
 * @return The result of interpretation.
 */
CruxInterpretResult crux_interpret(CruxVM* vm, const char* source);

/* --- CruxValue Utilities --- */

// Internal constants for value tags (corresponds to value.h)
#define CRUX_QNAN          ((uint64_t)0x7ffc000000000000)
#define CRUX_SIGN_BIT      ((uint64_t)0x8000000000000000)
#define CRUX_TAG_NIL       1 // 01
#define CRUX_TAG_FALSE     2 // 10
#define CRUX_TAG_TRUE      3 // 11
#define CRUX_TAG_INT32_BIT ((uint64_t)1 << 48)

#define CRUX_NIL_VAL       ((CruxValue)(uint64_t)(CRUX_QNAN | CRUX_TAG_NIL))
#define CRUX_FALSE_VAL     ((CruxValue)(uint64_t)(CRUX_QNAN | CRUX_TAG_FALSE))
#define CRUX_TRUE_VAL      ((CruxValue)(uint64_t)(CRUX_QNAN | CRUX_TAG_TRUE))

// CruxValue Creation
static inline CruxValue crux_float_val(double num) {
    union { double d; uint64_t v; } u;
    u.d = num;
    return u.v;
}

static inline CruxValue crux_int_val(int32_t i) {
    return (CruxValue)(CRUX_QNAN | CRUX_TAG_INT32_BIT | ((uint64_t)(i) & 0xFFFFFFFF));
}

static inline CruxValue crux_bool_val(bool b) {
    return b ? CRUX_TRUE_VAL : CRUX_FALSE_VAL;
}

// CruxValue Checking
static inline bool crux_is_float(CruxValue v) { return (v & CRUX_QNAN) != CRUX_QNAN; }
static inline bool crux_is_int(CruxValue v)   { return (v & (CRUX_QNAN | CRUX_SIGN_BIT | CRUX_TAG_INT32_BIT)) == (CRUX_QNAN | CRUX_TAG_INT32_BIT); }
static inline bool crux_is_bool(CruxValue v)  { return (v | 1) == CRUX_TRUE_VAL; }
static inline bool crux_is_nil(CruxValue v)   { return v == CRUX_NIL_VAL; }

// CruxValue Conversion
static inline double crux_as_float(CruxValue v) {
    union { uint64_t v; double d; } u;
    u.v = v;
    return u.d;
}

static inline int32_t crux_as_int(CruxValue v) { return (int32_t)(v & 0xFFFFFFFF); }
static inline bool    crux_as_bool(CruxValue v) { return v == CRUX_TRUE_VAL; }

#ifdef __cplusplus
}
#endif

#endif // CRUX_H
