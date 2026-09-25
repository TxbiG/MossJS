cmake_minimum_required(VERSION 3.20)

project(MossJS VERSION 0.4.0 LANGUAGES C CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "MossJS requires an Emscripten toolchain.")
endif()

set(MOSS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../Moss"
    CACHE PATH "Path to the Moss source tree")

if(NOT EXISTS "${MOSS_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "Moss source tree was not found at ${MOSS_ROOT}")
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(USE_OPENGL ON CACHE BOOL "" FORCE)
set(USE_OPENGLES OFF CACHE BOOL "" FORCE)
set(USE_VULKAN OFF CACHE BOOL "" FORCE)
set(USE_METAL OFF CACHE BOOL "" FORCE)
set(USE_DIRECTX12 OFF CACHE BOOL "" FORCE)
set(USE_WAYLAND OFF CACHE BOOL "" FORCE)

option(MOSSJS_SINGLE_FILE "Embed the WebAssembly binary in the generated JavaScript" OFF)
option(MOSSJS_THREADS "Enable WebAssembly pthreads" OFF)
set(MOSSJS_PTHREAD_POOL_SIZE 4 CACHE STRING "Emscripten WebAssembly pthread pool size")
option(MOSSJS_SIMD "Enable WebAssembly SIMD" ON)
option(MOSSJS_ASSERTIONS "Enable Emscripten assertions" OFF)
option(MOSSJS_ASYNCIFY "Enable Asyncify for Moss_Delay/Moss_Yield" ON)

if(MOSSJS_SIMD)
    set(USE_WASM_SIMD ON CACHE BOOL "" FORCE)
else()
    set(USE_WASM_SIMD OFF CACHE BOOL "" FORCE)
endif()

# Apply the Web stdinc compatibility layer to Moss itself as well as MossJS.
# The overlay includes the real upstream Moss_stdinc.h and then supplies the
# browser-specific corrections, without editing the upstream source file.
add_compile_options(
    "SHELL:-include ${CMAKE_CURRENT_LIST_DIR}/include/Moss/Moss_stdinc_Web.h"
)

add_subdirectory("${MOSS_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/Moss")

add_executable(MossJS
    src/MossJS.cpp
    src/MossJS_Subsystems.cpp
)
target_compile_features(MossJS PRIVATE cxx_std_17)
target_compile_definitions(MossJS PRIVATE
    MOSS_PLATFORM_WASM=1
    MOSS_PLATFORM_WEB=1
)


target_include_directories(MossJS PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${MOSS_ROOT}/include"
)
target_link_libraries(MossJS PRIVATE Moss::Moss)

set(_MOSSJS_FLAGS
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-sEXPORT_NAME=MossJS"
    "-sENVIRONMENT=web"
    "-sNO_EXIT_RUNTIME=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sINITIAL_MEMORY=134217728"
    "-sSTACK_SIZE=1048576"
    "-sWASM_BIGINT=1"
    "-sMIN_WEBGL_VERSION=2"
    "-sMAX_WEBGL_VERSION=2"
    "-sFULL_ES3=1"
    "-sFILESYSTEM=1"
    "-sFORCE_FILESYSTEM=1"
    "-sEXPORTED_RUNTIME_METHODS=FS"
    "-lidbfs.js"
    "--bind"
)

if(MOSSJS_SINGLE_FILE)
    list(APPEND _MOSSJS_FLAGS "-sSINGLE_FILE=1")
endif()

if(MOSSJS_ASSERTIONS)
    list(APPEND _MOSSJS_FLAGS "-sASSERTIONS=1")
endif()

if(MOSSJS_ASYNCIFY)
    list(APPEND _MOSSJS_FLAGS "-sASYNCIFY=1")
endif()

if(MOSSJS_THREADS)
    target_compile_options(MossJS PRIVATE "-pthread")
    target_link_options(MossJS PRIVATE
        "-pthread"
        "-sPTHREAD_POOL_SIZE=${MOSSJS_PTHREAD_POOL_SIZE}"
        "-sPROXY_TO_PTHREAD=0"
    )
endif()

if(MOSSJS_SIMD)
    target_compile_options(MossJS PRIVATE "-msimd128")
endif()

target_link_options(MossJS PRIVATE ${_MOSSJS_FLAGS})

set_target_properties(MossJS PROPERTIES
    OUTPUT_NAME "MossJS"
    SUFFIX ".js"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/dist"
)
