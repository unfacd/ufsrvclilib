# packages/protobuf-c — vendored protobuf-c 1.4.1 (BSD-2-Clause).
#
# The upstream sources are built here rather than through build-cmake/: that
# build assumes it is the top-level CMake project and writes its generated
# protobuf-c.pb.cc relative to CMAKE_BINARY_DIR, which does not hold when the
# package is a subdirectory. Nothing under protobuf-c/ or protoc-c/ is edited.
#
# Two artefacts:
#
#   protobuf-c::protobuf-c   the runtime the generated .pb-c.c sources link against
#   protoc-gen-c             the protoc plugin that generates them — only with
#                            UFSRVCLI_PROTOBUF_C_GENERATOR=ON
#
# Generating .pb-c sources is a maintainer task, not part of the ordinary build,
# so the generator is behind an explicit flag and repeatable on demand:
#
#   cmake -B <dir> -DUFSRVCLI_PROTOBUF_C_GENERATOR=ON
#
# It needs the protoc plugin API, which the system provides only through
# libprotoc-dev (google/protobuf/compiler/*.h and the unversioned libprotoc.so).
# With the flag ON and that package absent, configuration fails and says so.
#
# Provenance and the update procedure are in VENDORED.md.

option(UFSRVCLI_PROTOBUF_C_GENERATOR
    "Build protoc-gen-c, the .pb-c source generator (maintainer-only; needs libprotoc-dev)" OFF)

set(_pc_dir ${CMAKE_CURRENT_SOURCE_DIR})
set(_pc_bin ${CMAKE_CURRENT_BINARY_DIR})

add_library(protobuf-c STATIC ${_pc_dir}/protobuf-c/protobuf-c.c)
# Installed under a namespaced name: a vendored copy must never be mistaken for
# (or shadow) the system libprotobuf-c.
set_target_properties(protobuf-c PROPERTIES OUTPUT_NAME ufsrvclilib_protobuf_c)
# Two roots on purpose: generated code does #include <protobuf-c/protobuf-c.h>,
# while the vendored generator sources do #include "protobuf-c.h".
target_include_directories(protobuf-c PUBLIC
    $<BUILD_INTERFACE:${_pc_dir}>
    $<BUILD_INTERFACE:${_pc_dir}/protobuf-c>)
add_library(protobuf-c::protobuf-c ALIAS protobuf-c)

if(UFSRVCLI_PROTOBUF_C_GENERATOR)
    find_package(Protobuf REQUIRED)

    if(NOT Protobuf_PROTOC_LIBRARY OR
       NOT EXISTS "${Protobuf_INCLUDE_DIR}/google/protobuf/compiler/code_generator.h")
        message(FATAL_ERROR
            "protobuf-c: UFSRVCLI_PROTOBUF_C_GENERATOR=ON requires libprotoc-dev "
            "(the protoc plugin API: google/protobuf/compiler/*.h and libprotoc).\n"
            "Install it (apt install libprotoc-dev) to generate .pb-c sources, or "
            "leave the option OFF and use prebuilt sources.")
    endif()

    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Baseline version of the vendored sources (VENDORED.md); upstream's build
    # files are not vendored, so the string lives here.
    set(_pc_version 1.4.1)

    set(_pc_gen ${_pc_bin}/generated)
    file(MAKE_DIRECTORY "${_pc_gen}/protobuf-c")

    add_custom_command(
        OUTPUT "${_pc_gen}/protobuf-c/protobuf-c.pb.cc" "${_pc_gen}/protobuf-c/protobuf-c.pb.h"
        COMMAND "${Protobuf_PROTOC_EXECUTABLE}" "--cpp_out=${_pc_gen}" "-I${_pc_dir}"
                "${_pc_dir}/protobuf-c/protobuf-c.proto"
        DEPENDS "${_pc_dir}/protobuf-c/protobuf-c.proto"
        COMMENT "protoc: protobuf-c.proto -> C++ plugin API"
        VERBATIM)

    file(GLOB _protoc_gen_src CONFIGURE_DEPENDS "${_pc_dir}/protoc-c/*.cc")

    add_executable(protoc-gen-c
        ${_protoc_gen_src}
        "${_pc_gen}/protobuf-c/protobuf-c.pb.cc")
    target_include_directories(protoc-gen-c PRIVATE
        ${_pc_dir} ${_pc_dir}/protobuf-c ${_pc_dir}/protoc-c ${_pc_gen} ${Protobuf_INCLUDE_DIR})
    target_compile_definitions(protoc-gen-c PRIVATE
        HAVE_PROTO3
        PACKAGE_VERSION="${_pc_version}"
        PACKAGE_STRING="protobuf-c ${_pc_version}")
    target_link_libraries(protoc-gen-c PRIVATE protobuf::libprotobuf protobuf::libprotoc)

    message(STATUS "protobuf-c: building protoc-gen-c (maintainer task)")
else()
    message(STATUS "protobuf-c: generator build off (UFSRVCLI_PROTOBUF_C_GENERATOR=OFF)")
endif()
