#include "crux.h"
#include "vm.h"
#include <string.h>

CRUX_API int get_crux_version_number() {
    return Crux_VERSION_NUMBER;
}

CRUX_API void init_crux_configuration(CruxConfiguration* configuration) {
    configuration->reallocateFn = NULL;
    configuration->resolveModuleFn = NULL;
    configuration->loadModuleFn = NULL;
    configuration->bindForeignMethodFn = NULL;
    configuration->bindForeignClassFn = NULL;
    configuration->writeFn = NULL;
    configuration->errorFn = NULL;
    configuration->initialHeapSize = 1024 * 1024 * 5; // 5MB
    configuration->minHeapSize = 1024 * 1024;        // 1MB
    configuration->heapGrowthPercent = 50;
    configuration->userData = NULL;
}

CRUX_API CruxVM* crux_vm_new(CruxConfiguration* configuration) {
    return new_vm(configuration);
}

CRUX_API void crux_vm_free(CruxVM* vm) {
    free_vm(vm);
}

CRUX_API int crux_vm_get_exit_code(CruxVM* vm) {
    return vm->exit_code;
}

CRUX_API void crux_collect_garbage(CruxVM* vm) {
    // TODO: implement once internal collect_garbage is ready
}

CRUX_API CruxInterpretResult crux_interpret(CruxVM* vm, const char* module, const char* source) {
    // module is currently ignored but can be used for error reporting
    return (CruxInterpretResult)interpret(vm, (char*)source);
}
