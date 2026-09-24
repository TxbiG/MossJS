cmake_minimum_required(VERSION 3.20)

project(JSMossWeb VERSION 0.1.0 LANGUAGES C CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "JSMoss-Web must be configured with Emscripten.")
endif()

set(MOSS_FRAMEWORK_DIR ""
    CACHE PATH "Path to the Moss framework source checkout")

if(NOT MOSS_FRAMEWORK_DIR)
    set(MOSS_FRAMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Moss")
endif()

get_filename_component(MOSS_FRAMEWORK_DIR
    "${MOSS_FRAMEWORK_DIR}" ABSOLUTE)

if(NOT EXISTS "${MOSS_FRAMEWORK_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Moss framework not found: ${MOSS_FRAMEWORK_DIR}")
endif()

add_subdirectory(
    "${MOSS_FRAMEWORK_DIR}"
    "${CMAKE_BINARY_DIR}/moss"
    EXCLUDE_FROM_ALL
)

if(NOT TARGET Moss::Moss)
    if(TARGET Moss)
        add_library(Moss::Moss ALIAS Moss)
    else()
        message(FATAL_ERROR "Moss::Moss target was not found.")
    endif()
endif()

add_executable(JSMoss
    src/moss_web.cpp
    src/moss_web_bindings.cpp
)

target_compile_features(JSMoss PRIVATE cxx_std_17)

target_include_directories(JSMoss PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(JSMoss PRIVATE Moss::Moss)

set_target_properties(JSMoss PROPERTIES
    SUFFIX ".js"
)

target_link_options(JSMoss PRIVATE
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-sENVIRONMENT=web"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sFILESYSTEM=1"
    "-sASSERTIONS=1"
    "-sWASM_BIGINT=1"
    "-sUSE_WEBGL2=1"
    "-sFULL_ES3=1"
    "--bind"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(JSMoss PRIVATE MOSS_DEBUG)
else()
    target_link_options(JSMoss PRIVATE
        "-O3"
        "-sASSERTIONS=0"
    )
endif()
