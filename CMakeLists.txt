cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_COMPILER gcc)

project(crux-lang C)

set(CMAKE_C_STANDARD 11)

option(CRUX_STACK_SAFETY "Enable stack push/pop safety checks" OFF)
option(CRUX_DEBUG_TRACE_EXECUTION "Enable execution tracing" OFF)
option(CRUX_DEBUG_PRINT_CODE "Print compiled bytecode" OFF)
option(CRUX_DEBUG_LOG_GC "Enable GC logging" OFF)
option(CRUX_DEBUG_STRESS_GC "Enable GC stress mode" OFF)
option(CRUX_TAGGED_OBJECT "Enables tagged pointers for objects. Should not be used on 32 bit platforms." ON)
set(CRUX_VERSION "dev" CACHE STRING "Crux language version")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "ASAN" AND NOT UNIX)
  message(FATAL_ERROR "ASAN builds are only allowed on UNIX-like systems")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "ASAN")
  add_compile_options(-g3 -O0 -Wall -Wextra)

  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fstack-protector-strong")

  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wformat=2 -Wformat-security")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wnull-dereference -Wstack-protector")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Warray-bounds -Wshift-overflow")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "ASAN")
  if (UNIX)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=undefined")
  endif ()
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-O3 -fno-delete-null-pointer-checks -flto -flto=auto)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Perf")
  add_compile_options(-O2 -g -fno-omit-frame-pointer -fno-delete-null-pointer-checks -flto -flto=auto)
endif()

file(GLOB SOURCES
    "src/*.c"
    "src/native/*.c"
    "src/vm/*.c"
    "src/memory/*.c"
    "src/compiler/*.c"
    "src/type_system/*.c"
)

list(REMOVE_ITEM SOURCES
    "${CMAKE_SOURCE_DIR}/src/main.c"
    "${CMAKE_SOURCE_DIR}/src/file_handler.c"
)

# Internal Headers
file(GLOB LIB_HEADERS
   "include/*.h"
   "src/internal_headers/compiler/*.h"
   "src/internal_headers/native/*.h"
   "src/internal_headers/*.h"
   "deps/utf8/utf8.h"
)

# Define the library
add_library(crux_lib STATIC ${SOURCES} ${LIB_HEADERS})
set_target_properties(crux_lib PROPERTIES OUTPUT_NAME crux)

target_include_directories(crux_lib PUBLIC include)
target_include_directories(crux_lib PRIVATE src/internal_headers deps/utf8 deps/json)

target_compile_definitions(crux_lib PRIVATE
        $<$<BOOL:${CRUX_STACK_SAFETY}>:STACK_SAFETY>
        $<$<BOOL:${CRUX_DEBUG_TRACE_EXECUTION}>:DEBUG_TRACE_EXECUTION>
        $<$<BOOL:${CRUX_DEBUG_PRINT_CODE}>:DEBUG_PRINT_CODE>
        $<$<BOOL:${CRUX_DEBUG_LOG_GC}>:DEBUG_LOG_GC>
        $<$<BOOL:${CRUX_DEBUG_STRESS_GC}>:DEBUG_STRESS_GC>
        $<$<BOOL:${CRUX_TAGGED_OBJECT}>:CRUX_TAGGED_OBJECT>
)

if (UNIX)
  target_link_libraries(crux_lib PUBLIC m)
endif ()

# Define the CLI executable
set(CLI_SOURCES 
    "cli/main.c" 
    "cli/cli_commands.c" 
    "cli/cli_utils.c"
    "src/file_handler.c" 
    "deps/json/cJSON.c"
)
if (NOT WIN32)
  list(APPEND CLI_SOURCES "deps/linenoise/linenoise.c")
endif()

add_executable(crux ${CLI_SOURCES})
target_link_libraries(crux PRIVATE crux_lib)
target_include_directories(crux PRIVATE cli src/internal_headers deps/json deps/utf8)

if (NOT WIN32)
  target_include_directories(crux PRIVATE deps/linenoise)
endif()

target_compile_definitions(crux PRIVATE CRUX_VERSION="${CRUX_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Print enabled compile flags
message(STATUS "Crux-lang build configuration:")
message(STATUS "  CMake Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Version: ${CRUX_VERSION}")
message(STATUS "  Enabled options:")
if(CRUX_STACK_SAFETY)
  message(STATUS "    - CRUX_STACK_SAFETY: ON")
endif()
if(CRUX_DEBUG_TRACE_EXECUTION)
  message(STATUS "    - CRUX_DEBUG_TRACE_EXECUTION: ON")
endif()
if(CRUX_DEBUG_PRINT_CODE)
  message(STATUS "    - CRUX_DEBUG_PRINT_CODE: ON")
endif()
if(CRUX_DEBUG_LOG_GC)
  message(STATUS "    - CRUX_DEBUG_LOG_GC: ON")
endif()
if(CRUX_DEBUG_STRESS_GC)
  message(STATUS "    - CRUX_DEBUG_STRESS_GC: ON")
endif()
if(CRUX_TAGGED_OBJECT)
  message(STATUS "    - CRUX_TAGGED_OBJECT: ON")
endif()
if(NOT CRUX_STACK_SAFETY AND NOT CRUX_DEBUG_TRACE_EXECUTION AND NOT CRUX_DEBUG_PRINT_CODE AND NOT CRUX_DEBUG_LOG_GC AND NOT CRUX_DEBUG_STRESS_GC AND NOT CRUX_TAGGED_OBJECT)
  message(STATUS "    (none)")
endif()
message(STATUS "")
