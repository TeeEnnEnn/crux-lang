#ifndef CRUX_H
#define CRUX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Public API for the Crux language.
 * Heavily inspired by Wren (https://wren.io/)
 * Thanks Bob Nystrom and friends!
 */

#ifdef __cplusplus
extern "C" {
#endif

// Semantic version components
#define CRUX_VERSION_MAJOR 0
#define CRUX_VERSION_MINOR 22
#define CRUX_VERSION_PATCH 0

#define CRUX_VERSION_STRING "0.22.0"

// Numeric representation for range checks
#define CRUX_VERSION_NUMBER (CRUX_VERSION_MAJOR * 1000000 + \
                             CRUX_VERSION_MINOR * 1000 +    \
                             CRUX_VERSION_PATCH)

#ifndef CRUX_API
  #if defined(_MSC_VER) && defined(CRUX_API_DLLEXPORT)
    #define CRUX_API __declspec( dllexport )
  #else
    #define CRUX_API
  #endif
#endif

/* --- Types --- */

typedef struct CruxVM CruxVM;
typedef struct CruxHandle CruxHandle;
typedef uint64_t CruxValue;

typedef enum {
    CRUX_INTERPRET_OK = 0,
    CRUX_INTERPRET_COMPILE_PANIC = 1,
    CRUX_INTERPRET_RUNTIME_PANIC = 2,
    CRUX_INTERPRET_EXIT = 3
} CruxInterpretResult;

typedef enum {
  CRUX_ERROR_COMPILE,
  CRUX_ERROR_RUNTIME,
  CRUX_ERROR_STACK_TRACE
} CruxErrorType;

typedef enum {
  CRUX_TYPE_BOOL,
  CRUX_TYPE_INT,
  CRUX_TYPE_FLOAT,
  CRUX_TYPE_STRING,
  CRUX_TYPE_ARRAY,
  CRUX_TYPE_TABLE,
  CRUX_TYPE_NULL,
  CRUX_TYPE_FOREIGN,
  CRUX_TYPE_UNKNOWN
} CruxType;

/* --- Configuration & Callbacks --- */

typedef void* (*CruxReallocateFn)(void* memory, size_t newSize, void* userData);
typedef void (*CruxPrintFn)(CruxVM* vm, const char* text);
typedef void (*CruxErrorFn)(CruxVM* vm, CruxErrorType type, const char* module, int line, const char* message);
typedef void (*CruxForeignMethodFn)(CruxVM* vm);
typedef void (*CruxFinalizerFn)(void* data);

typedef const char* (*CruxResolveModuleFn)(CruxVM* vm, const char* importer, const char* name);

struct CruxLoadModuleResult;
typedef void (*CruxLoadModuleCompleteFn)(CruxVM* vm, const char* name, struct CruxLoadModuleResult result);

typedef struct CruxLoadModuleResult {
  const char* source;
  CruxLoadModuleCompleteFn onComplete;
  void* userData;
} CruxLoadModuleResult;

typedef CruxLoadModuleResult (*CruxLoadModuleFn)(CruxVM* vm, const char* name);
typedef CruxForeignMethodFn (*CruxBindForeignMethodFn)(CruxVM* vm, const char* module, const char* className, bool isStatic, const char* signature);

typedef struct {
  CruxForeignMethodFn allocate;
  CruxFinalizerFn finalize;
} CruxForeignClassMethods;

typedef CruxForeignClassMethods (*CruxBindForeignClassFn)(CruxVM* vm, const char* module, const char* className);

typedef struct {
  CruxReallocateFn reallocateFn;
  CruxResolveModuleFn resolveModuleFn;
  CruxLoadModuleFn loadModuleFn;
  CruxBindForeignMethodFn bindForeignMethodFn;
  CruxBindForeignClassFn bindForeignClassFn;
  CruxPrintFn writeFn;
  CruxErrorFn errorFn;

  size_t initialHeapSize;
  size_t minHeapSize;
  int heapGrowthPercent;

  const char* scriptPath;
  void* userData;
} CruxConfiguration;

/* --- CruxVM Management --- */

CRUX_API int crux_get_version_number(void);
CRUX_API void init_crux_configuration(CruxConfiguration* configuration);
CRUX_API CruxVM* crux_vm_new(CruxConfiguration* configuration);
CRUX_API void crux_vm_free(CruxVM* vm);
CRUX_API int crux_vm_get_exit_code(CruxVM* vm);
CRUX_API void crux_collect_garbage(CruxVM* vm);

/* --- Execution --- */

CRUX_API CruxInterpretResult crux_interpret(CruxVM* vm, const char* module, const char* source);

/* --- Slot API --- */
CRUX_API void crux_ensure_slots(CruxVM* vm, int numSlots);
CRUX_API int crux_get_slot_count(CruxVM* vm);
CRUX_API CruxType crux_get_slot_type(CruxVM* vm, int slot);

/**
 * @brief Looks up a variable in the specified module and stores it in [slot].
 * If module is NULL, searches in the main module.
 */
CRUX_API void crux_get_variable(CruxVM* vm, const char* module, const char* name, int slot);

/**
 * @brief Calls the function stored in slot 0 with [argCount] arguments.
 * Arguments are expected to be in slots 1 to [argCount].
 * The result is stored in slot 0.
 */
CRUX_API CruxInterpretResult crux_call(CruxVM* vm, int argCount);

// Getters
CRUX_API bool crux_get_slot_bool(CruxVM* vm, int slot);

CRUX_API int32_t crux_get_slot_int(CruxVM* vm, int slot);
CRUX_API double crux_get_slot_double(CruxVM* vm, int slot);
CRUX_API const char* crux_get_slot_string(CruxVM* vm, int slot);

CRUX_API void crux_set_slot_bool(CruxVM* vm, int slot, bool value);
CRUX_API void crux_set_slot_int(CruxVM* vm, int slot, int32_t value);
CRUX_API void crux_set_slot_double(CruxVM* vm, int slot, double value);
CRUX_API void crux_set_slot_string(CruxVM* vm, int slot, const char* text);
CRUX_API void crux_set_slot_nil(CruxVM* vm, int slot);
CRUX_API void crux_set_slot_handle(CruxVM* vm, int slot, CruxHandle* handle);

/* --- Handle API --- */

CRUX_API CruxHandle* crux_get_slot_handle(CruxVM* vm, int slot);
CRUX_API void crux_release_handle(CruxVM* vm, CruxHandle* handle);

/* --- Inline Value Utilities --- */

#define CRUX_QNAN          ((uint64_t)0x7ffc000000000000)
#define CRUX_SIGN_BIT      ((uint64_t)0x8000000000000000)
#define CRUX_TAG_NIL       1
#define CRUX_TAG_FALSE     2
#define CRUX_TAG_TRUE      3
#define CRUX_TAG_INT32_BIT ((uint64_t)1 << 48)

#define CRUX_NIL_VAL       ((CruxValue)(uint64_t)(CRUX_QNAN | CRUX_TAG_NIL))
#define CRUX_FALSE_VAL     ((CruxValue)(uint64_t)(CRUX_QNAN | CRUX_TAG_FALSE))
#define CRUX_TRUE_VAL      ((CruxValue)(uint64_t)(CRUX_QNAN | CRUX_TAG_TRUE))

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

static inline bool crux_is_float(CruxValue v) { return (v & CRUX_QNAN) != CRUX_QNAN; }
static inline bool crux_is_int(CruxValue v)   { return (v & (CRUX_QNAN | CRUX_SIGN_BIT | CRUX_TAG_INT32_BIT)) == (CRUX_QNAN | CRUX_TAG_INT32_BIT); }
static inline bool crux_is_bool(CruxValue v)  { return (v | 1) == CRUX_TRUE_VAL; }
static inline bool crux_is_nil(CruxValue v)   { return v == CRUX_NIL_VAL; }

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
