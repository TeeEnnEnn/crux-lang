#ifndef CRUX_H
#define CRUX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Public API for the Crux language.
 *
 * This is heavily inspired by https://Crux.io/
 * Thanks Bob Nystrom and friends!
 */


#ifdef __cplusplus
extern "C" {
#endif

// The Crux semantic version number components.
#define Crux_VERSION_MAJOR 0
#define Crux_VERSION_MINOR 22
#define Crux_VERSION_PATCH 0

// A human-friendly string representation of the version.
#define Crux_VERSION_STRING "0.22.0"

// A monotonically increasing numeric representation of the version number. Use
// this if you want to do range checks over versions.
#define Crux_VERSION_NUMBER (Crux_VERSION_MAJOR * 1000000 +                    \
                             Crux_VERSION_MINOR * 1000 +                       \
                             Crux_VERSION_PATCH)


#ifndef CRUX_API
  #if defined(_MSC_VER) && defined(CRUX_API_DLLEXPORT)
    #define CRUX_API __declspec( dllexport )
  #else
    #define CRUX_API
  #endif
#endif //CRUX_API



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
    CRUX_INTERPRET_OK = 0, /** success */
    CRUX_INTERPRET_COMPILE_PANIC = 1, /** comptime panic */
    CRUX_INTERPRET_RUNTIME_PANIC = 2, /** runtime panic */
    CRUX_INTERPRET_EXIT = 3 /** exit() */
} CruxInterpretResult;

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






// A generic allocation function that handles all explicit memory management
// used by Crux. It's used like so:
//
// - To allocate new memory, [memory] is NULL and [newSize] is the desired
//   size. It should return the allocated memory or NULL on failure.
//
// - To attempt to grow an existing allocation, [memory] is the memory, and
//   [newSize] is the desired size. It should return [memory] if it was able to
//   grow it in place, or a new pointer if it had to move it.
//
// - To shrink memory, [memory] and [newSize] are the same as above but it will
//   always return [memory].
//
// - To free memory, [memory] will be the memory to free and [newSize] will be
//   zero. It should return NULL.
typedef void* (*CruxReallocateFn)(void* memory, size_t newSize, void* userData);



// Displays a string of text to the user.
typedef void (*CruxPrintFn)(CruxVM* vm, const char* text);


typedef enum
{
  // A syntax or resolution error detected at compile time.
  Crux_ERROR_COMPILE,

  // The error message for a runtime error.
  Crux_ERROR_RUNTIME,

  // One entry of a runtime error's stack trace.
  Crux_ERROR_STACK_TRACE
} CruxErrorType;

// Reports an error to the user.
//
// An error detected during compile time is reported by calling this once with
// [type] `Crux_ERROR_COMPILE`, the resolved name of the [module] and [line]
// where the error occurs, and the compiler's error [message].
//
// A runtime error is reported by calling this once with [type]
// `Crux_ERROR_RUNTIME`, no [module] or [line], and the runtime error's
// [message]. After that, a series of [type] `Crux_ERROR_STACK_TRACE` calls are
// made for each line in the stack trace. Each of those has the resolved
// [module] and [line] where the method or function is defined and [message] is
// the name of the method or function.
typedef void (*CruxErrorFn)(
    CruxVM* vm, CruxErrorType type, const char* module, int line,
    const char* message);


// A function callable from Crux code, but implemented in C.
typedef void (*CruxForeignMethodFn)(CruxVM* vm);

// A finalizer function for freeing resources owned by an instance of a foreign
// class. Unlike most foreign methods, finalizers do not have access to the VM
// and should not interact with it since it's in the middle of a garbage
// collection.
typedef void (*CruxFinalizerFn)(void* data);

// Gives the host a chance to canonicalize the imported module name,
// potentially taking into account the (previously resolved) name of the module
// that contains the import. Typically, this is used to implement relative
// imports.
typedef const char* (*CruxResolveModuleFn)(CruxVM* vm,
    const char* importer, const char* name);

// Forward declare
struct CruxLoadModuleResult;

// Called after loadModuleFn is called for module [name]. The original returned result
// is handed back to you in this callback, so that you can free memory if appropriate.
typedef void (*CruxLoadModuleCompleteFn)(CruxVM* vm, const char* name, struct CruxLoadModuleResult result);

// The result of a loadModuleFn call.
// [source] is the source code for the module, or NULL if the module is not found.
// [onComplete] an optional callback that will be called once Crux is done with the result.
typedef struct CruxLoadModuleResult
{
  const char* source;
  CruxLoadModuleCompleteFn onComplete;
  void* userData;
} CruxLoadModuleResult;

// Loads and returns the source code for the module [name].
typedef CruxLoadModuleResult (*CruxLoadModuleFn)(CruxVM* vm, const char* name);

// Returns a pointer to a foreign method on [className] in [module] with
// [signature].
typedef CruxForeignMethodFn (*CruxBindForeignMethodFn)(CruxVM* vm,
    const char* module, const char* className, bool isStatic,
    const char* signature);

typedef struct
{
  // The callback invoked when the foreign object is created.
  //
  // This must be provided. Inside the body of this, it must call
  // [CruxSetSlotNewForeign()] exactly once.
  CruxForeignMethodFn allocate;

  // The callback invoked when the garbage collector is about to collect a
  // foreign object's memory.
  //
  // This may be `NULL` if the foreign class does not need to finalize.
  CruxFinalizerFn finalize;
} CruxForeignClassMethods;

// Returns a pair of pointers to the foreign methods used to allocate and
// finalize the data for instances of [className] in resolved [module].
typedef CruxForeignClassMethods (*CruxBindForeignClassFn)(
    CruxVM* vm, const char* module, const char* className);


typedef struct
{
  // The callback Crux will use to allocate, reallocate, and deallocate memory.
  //
  // If `NULL`, defaults to a built-in function that uses `realloc` and `free`.
  CruxReallocateFn reallocateFn;

  // The callback Crux uses to resolve a module name.
  //
  // Some host applications may wish to support "relative" imports, where the
  // meaning of an import string depends on the module that contains it. To
  // support that without baking any policy into Crux itself, the VM gives the
  // host a chance to resolve an import string.
  //
  // Before an import is loaded, it calls this, passing in the name of the
  // module that contains the import and the import string. The host app can
  // look at both of those and produce a new "canonical" string that uniquely
  // identifies the module. This string is then used as the name of the module
  // going forward. It is what is passed to [loadModuleFn], how duplicate
  // imports of the same module are detected, and how the module is reported in
  // stack traces.
  //
  // If you leave this function NULL, then the original import string is
  // treated as the resolved string.
  //
  // If an import cannot be resolved by the embedder, it should return NULL and
  // Crux will report that as a runtime error.
  //
  // Crux will take ownership of the string you return and free it for you, so
  // it should be allocated using the same allocation function you provide
  // above.
  CruxResolveModuleFn resolveModuleFn;

  // The callback Crux uses to load a module.
  //
  // Since Crux does not talk directly to the file system, it relies on the
  // embedder to physically locate and read the source code for a module. The
  // first time an import appears, Crux will call this and pass in the name of
  // the module being imported. The method will return a result, which contains
  // the source code for that module. Memory for the source is owned by the
  // host application, and can be freed using the onComplete callback.
  //
  // This will only be called once for any given module name. Crux caches the
  // result internally so subsequent imports of the same module will use the
  // previous source and not call this.
  //
  // If a module with the given name could not be found by the embedder, it
  // should return NULL and Crux will report that as a runtime error.
  CruxLoadModuleFn loadModuleFn;

  // The callback Crux uses to find a foreign method and bind it to a class.
  //
  // When a foreign method is declared in a class, this will be called with the
  // foreign method's module, class, and signature when the class body is
  // executed. It should return a pointer to the foreign function that will be
  // bound to that method.
  //
  // If the foreign function could not be found, this should return NULL and
  // Crux will report it as runtime error.
  CruxBindForeignMethodFn bindForeignMethodFn;

  // The callback Crux uses to find a foreign class and get its foreign methods.
  //
  // When a foreign class is declared, this will be called with the class's
  // module and name when the class body is executed. It should return the
  // foreign functions uses to allocate and (optionally) finalize the bytes
  // stored in the foreign object when an instance is created.
  CruxBindForeignClassFn bindForeignClassFn;

  // The callback Crux uses to display text when `System.print()` or the other
  // related functions are called.
  //
  // If this is `NULL`, Crux discards any printed text.
  CruxPrintFn writeFn;

  // The callback Crux uses to report errors.
  //
  // When an error occurs, this will be called with the module name, line
  // number, and an error message. If this is `NULL`, Crux doesn't report any
  // errors.
  CruxErrorFn errorFn;

  // The number of bytes Crux will allocate before triggering the first garbage
  // collection.
  //
  // If zero, defaults to 10MB.
  size_t initialHeapSize;

  // After a collection occurs, the threshold for the next collection is
  // determined based on the number of bytes remaining in use. This allows Crux
  // to shrink its memory usage automatically after reclaiming a large amount
  // of memory.
  //
  // This can be used to ensure that the heap does not get too small, which can
  // in turn lead to a large number of collections afterwards as the heap grows
  // back to a usable size.
  //
  // If zero, defaults to 1MB.
  size_t minHeapSize;

  // Crux will resize the heap automatically as the number of bytes
  // remaining in use after a collection changes. This number determines the
  // amount of additional memory Crux will use after a collection, as a
  // percentage of the current heap size.
  //
  // For example, say that this is 50. After a garbage collection, when there
  // are 400 bytes of memory still in use, the next collection will be triggered
  // after a total of 600 bytes are allocated (including the 400 already in
  // use.)
  //
  // Setting this to a smaller number wastes less memory, but triggers more
  // frequent garbage collections.
  //
  // If zero, defaults to 50.
  int heapGrowthPercent;

  // The path to the main script being executed.
  const char* scriptPath;

  // User-defined data associated with the VM.
  void* userData;

} CruxConfiguration;

/* --- CruxVM Management --- */

/**
 * @brief Initializes [configuration] with all of its default values.
 *
 * Call this before setting the particular fields you care about.
 */
CRUX_API void init_crux_configuration(CruxConfiguration* configuration);

/**
 * @brief Creates and initializes a new Crux Virtual Machine.
 * @param configuration The configuration for the VM. If NULL, default configuration is used.
 * @return A pointer to the new CruxVM instance, or NULL if initialization failed.
 */
CRUX_API CruxVM* crux_vm_new(CruxConfiguration* configuration);

/**
 * @brief Frees all memory associated with a Crux Virtual Machine.
 * @param vm The CruxVM instance to destroy.
 */
CRUX_API void crux_vm_free(CruxVM* vm);

/**
 * @brief Gets the exit code set by the script.
 */
CRUX_API int crux_vm_get_exit_code(CruxVM* vm);

/**
 * @brief Immediately run the garbage collector to free unused memory.
 */
CRUX_API void crux_collect_garbage(CruxVM* vm);

/* --- Execution --- */

/**
 * @brief Interprets and executes Crux source code from a string.
 * @param vm The CruxVM instance.
 * @param module The name of the module (for error reporting and imports).
 * @param source The null-terminated source code string.
 * @return The result of interpretation.
 */
CRUX_API CruxInterpretResult crux_interpret(CruxVM* vm, const char* module, const char* source);



#ifdef __cplusplus
}
#endif

#endif // CRUX_H
