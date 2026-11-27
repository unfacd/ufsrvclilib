if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(ufsrvclilib LANGUAGES C)
endif()
include(GNUInstallDirs)

# The family's shared build options live in cmake-modules/, vendored here so a
# standalone checkout configures exactly as it does inside a host: same clang
# requirement, same warning policy, same sanitizer plumbing behind the presets.
list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(UFSRV_C_WARNING_FLAGS
    -Wall -Wimplicit-fallthrough -Wno-unused-label -Wno-unused-variable
    -Wno-comment -Wno-nullability-completeness -Wno-unknown-attributes)
set(UFSRV_C_DEBUG_FLAGS_DEFAULT -ggdb3)

# Reported by ufsrv_build_options in its one-line configure banner.
if(BUILD_SHARED_LIBS)
    set(_UFSRV_BUILD_FLAVOUR "SHARED+STATIC")
else()
    set(_UFSRV_BUILD_FLAVOUR "STATIC")
endif()
include(ufsrv_build_options)

option(_PACKAGE_TESTS "Enable test targets" OFF)

# ufsrvclilib — reusable, class-agnostic client-side library for ufsrv
# class servers.
#
# Vendored dependency of this package (see packages/protobuf-c/VENDORED.md).
# It is added here, not by the host, so the package stays self-contained.
add_subdirectory(packages/protobuf-c)

if(BUILD_SHARED_LIBS AND TARGET protobuf-c)
    set_target_properties(protobuf-c PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

find_package(uflib REQUIRED)

find_package(ufsrvrxlib REQUIRED)

# ── Debug verbosity ─────────────────────────────────────────────────────────
# L3 implies L2, and either implies the base debug build switch.
option(UFSRVCLILIB_DEBUG_BUILD_L2 "Verbose level-2 debug logging" OFF)
option(UFSRVCLILIB_DEBUG_BUILD_L3 "Very verbose level-3 debug logging (per-object traces)" OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(UFSRVCLILIB_DEBUG_BUILD 1)
else()
    set(UFSRVCLILIB_DEBUG_BUILD 0)
endif()
if(UFSRVCLILIB_DEBUG_BUILD_L3)
    set(UFSRVCLILIB_DEBUG_L3 1)
    set(UFSRVCLILIB_DEBUG_L2 1)
else()
    set(UFSRVCLILIB_DEBUG_L3 0)
    if(UFSRVCLILIB_DEBUG_BUILD_L2)
        set(UFSRVCLILIB_DEBUG_L2 1)
    else()
        set(UFSRVCLILIB_DEBUG_L2 0)
    endif()
endif()
if(UFSRVCLILIB_DEBUG_L2 OR UFSRVCLILIB_DEBUG_L3)
    set(UFSRVCLILIB_DEBUG_BUILD 1)
endif()
add_compile_definitions(UFSRVCLILIB_DEBUG_BUILD=${UFSRVCLILIB_DEBUG_BUILD}
                        UFSRVCLILIB_DEBUG_BUILD_L2=${UFSRVCLILIB_DEBUG_L2}
                        UFSRVCLILIB_DEBUG_BUILD_L3=${UFSRVCLILIB_DEBUG_L3})

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# ── protobuf-c: the vendored runtime ────────────────────
# The CLI generates its own protobuf-c sources, so it links one runtime: the
# vendored packages/protobuf-c (1.4.1). It deliberately does not fall back to
# uflib's vendored copy or to a system libprotobuf-c — a second implementation
# in the same binary is unsafe, because the descriptors are laid out by the
# generated headers, not by whichever archive the linker resolves first.

if(NOT TARGET protobuf-c::protobuf-c)
    message(FATAL_ERROR
        "ufsrvclilib: the vendored protobuf-c runtime is missing. "
        "packages/protobuf-c ships with this repo; check the add_subdirectory at the top of CMakeLists.txt.")
endif()

set(UFSRVCLI_PROTOBUF_C_TARGET protobuf-c::protobuf-c)


if(TARGET uflib::uflib_protobuf_c)
    message(WARNING
        "protobuf-c: uflib also carries protobuf-c (UFLIB_CAPABILITY_PROTOBUF_C); "
        "two runtimes in one binary is unsafe. Rebuild uflib with that capability OFF.")
endif()

if(TARGET protoc-gen-c)
    message(STATUS "protobuf-c: vendored runtime + generator (protoc-gen-c)")
else()
    message(STATUS "protobuf-c: vendored runtime; no C generator — configure with "
                   "-DUFSRVCLI_PROTOBUF_C_GENERATOR=ON (needs libprotoc-dev) to generate")
endif()

set(UFSRVCLILIB_SOURCES
    src/registration/uf_registration.c
    src/ufsrvcli_conn.c
    src/ufsrvcli_http.c
    # generated from protobuf/*.proto (maintainer task; see
    # cmake/protobuf_c.cmake). Committed, so the ordinary build never regenerates.
    src/websocket/WebSocketMessage.pb-c.c
    src/unfacd_service/UnfacdService.pb-c.c
    src/websocket/websocket_commands.c
    src/ufsrvcli_context.c
    src/unfacd_service/group/unfacd_group.c
    src/unfacd_service/state_sync/state_sync.c
    src/unfacd_service/state_sync/state_sync_message.c
    src/unfacd_service/command/unfacd_comand.c
    src/unfacd_service/command/unfacd_command_utils.c
    src/unfacd_service/message/unfacd_message.c
    src/unfacd_service/rest/rest_api_fence.c
    src/unfacd_service/unfacd_rest.c
)

# Sources compile once, into an OBJECT library, so the static and (optional)
# shared flavour share them. PIC is on unconditionally: harmless for the static
# archive, required for the shared one.
add_library(ufsrvclilib_objects OBJECT ${UFSRVCLILIB_SOURCES})
set_target_properties(ufsrvclilib_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)
# OBJECT libraries do not link; these carry the compile-time usage requirements
# (include dirs/macros) the sources need from their dependencies.
target_link_libraries(ufsrvclilib_objects PRIVATE
    protobuf-c::protobuf-c uflib::uflib CURL::libcurl OpenSSL::SSL)
target_include_directories(ufsrvclilib_objects
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

# ── version + build configuration ─────────────────────────────────────────
# Version macros live in the public header; version.c is regenerated on every
# build so the compiled timestamp is fresh. The configuration header is
# generated from config_ufsrvclilib.h.in and installed with the headers.
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/include/ufsrvclilib/version.h _ufsrvclilib_vh)
foreach(_part MAJOR MINOR PATCH)
    string(REGEX MATCH "#define[ \t]+UFSRVCLILIB_${_part}[ \t]+([0-9]+)" _vm "${_ufsrvclilib_vh}")
    set(_UFSRVCLILIB_V${_part} "${CMAKE_MATCH_1}")
endforeach()
set(UFSRVCLILIB_VERSION "${_UFSRVCLILIB_VMAJOR}.${_UFSRVCLILIB_VMINOR}.${_UFSRVCLILIB_VPATCH}")

if(BUILD_SHARED_LIBS)
    set(UFSRVCLILIB_BUILT_SHARED 1)
else()
    set(UFSRVCLILIB_BUILT_SHARED 0)
endif()
set(UFSRVCLILIB_WITH_PROTOBUF_C 1)
set(UFSRVCLILIB_WITH_CURL 1)
set(UFSRVCLILIB_WITH_OPENSSL 1)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config_ufsrvclilib.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/ufsrvclilib/config_ufsrvclilib.h @ONLY)

add_custom_target(ufsrvclilib_version
    COMMAND chmod 755 ${CMAKE_CURRENT_SOURCE_DIR}/src/version.c.sh
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/src/version.c.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/version.c
    COMMENT "Generating ufsrvclilib version.c with a fresh compilation timestamp"
    VERBATIM
)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/version.c PROPERTIES GENERATED TRUE)
target_sources(ufsrvclilib_objects PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/version.c)
add_dependencies(ufsrvclilib_objects ufsrvclilib_version)


# ── static and (optional) shared flavours ─────────────────────────────────
# Sources compile once, in the OBJECT target above; the archives are thin
# wrappers over it, so BUILD_SHARED_LIBS=ON does not compile anything twice.
# Consumers link ufsrvclilib::ufsrvclilib, which resolves to the shared flavour
# when one is built, so no consumer encodes the linkage choice.
#
# uflib's static archive references utf8proc (utils_str.c) but does not
# propagate that dependency through its exported target; resolve it here so
# consumers link it transitively. (OPT packages live under /opt/lib.)
find_library(UTF8PROC_LIBRARY NAMES utf8proc PATHS /opt/lib)
if(NOT UTF8PROC_LIBRARY)
    message(FATAL_ERROR "utf8proc not found under /opt/lib (required by uflib)")
endif()

# Link order is load-bearing: these are static archives, and the linker takes
# protobuf_c_* definitions from the first archive that defines them. The
# vendored runtime precedes uflib so libufsrvclilib_protobuf_c.a wins over
# uflib's own copy.
set(UFSRVCLILIB_LINK_LIBS
    protobuf-c::protobuf-c ufsrvrxlib::ufsrvrxlib uflib::uflib
    CURL::libcurl ${UTF8PROC_LIBRARY} OpenSSL::SSL)

# BUILD_INTERFACE src/ is there because generated and private headers are
# addressed as <websocket/...> and <unfacd_service/...>; only include/ ships.
set(UFSRVCLILIB_PUBLIC_INCLUDES
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(ufsrvclilib_static STATIC $<TARGET_OBJECTS:ufsrvclilib_objects>)
set_target_properties(ufsrvclilib_static PROPERTIES OUTPUT_NAME ufsrvclilib)
target_include_directories(ufsrvclilib_static PUBLIC ${UFSRVCLILIB_PUBLIC_INCLUDES})
target_link_libraries(ufsrvclilib_static PUBLIC ${UFSRVCLILIB_LINK_LIBS})

if(BUILD_SHARED_LIBS)
    # Interim switch: until every public symbol carries PUBLIC_API, exporting
    # all globals keeps the shared library usable. Set OFF to enforce the
    # annotation sweep; a gap then fails at link time.
    option(UFSRVCLILIB_SHARED_EXPORT_ALL
        "Export every global from the shared library (interim; disable once PUBLIC_API coverage is complete)" ON)

    if(UFSRVCLILIB_SHARED_EXPORT_ALL)
        set(_UFSRVCLILIB_SHARED_VISIBILITY default)
    else()
        set(_UFSRVCLILIB_SHARED_VISIBILITY hidden)
    endif()

    # The sources compile in the OBJECT target, so visibility must be set
    # there; a preset on the shared target would compile nothing.
    set_target_properties(ufsrvclilib_objects PROPERTIES
        C_VISIBILITY_PRESET ${_UFSRVCLILIB_SHARED_VISIBILITY}
        VISIBILITY_INLINES_HIDDEN ON)

    add_library(ufsrvclilib_shared SHARED $<TARGET_OBJECTS:ufsrvclilib_objects>)
    set_target_properties(ufsrvclilib_shared PROPERTIES
        OUTPUT_NAME ufsrvclilib
        VERSION   ${UFSRVCLILIB_VERSION}
        SOVERSION ${_UFSRVCLILIB_VMAJOR})
    target_include_directories(ufsrvclilib_shared PUBLIC ${UFSRVCLILIB_PUBLIC_INCLUDES})
    target_link_libraries(ufsrvclilib_shared PUBLIC ${UFSRVCLILIB_LINK_LIBS})
    add_library(ufsrvclilib::ufsrvclilib ALIAS ufsrvclilib_shared)
else()
    add_library(ufsrvclilib::ufsrvclilib ALIAS ufsrvclilib_static)
endif()

# ── Install: archive, headers, CMake package, pkg-config ──────────────────
option(_PACKAGE_INSTALL "Enable install target" ON)
if(_PACKAGE_INSTALL)
    include(CMakePackageConfigHelpers)
    set(UFSRVCLILIB_INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/ufsrvclilib)

    install(TARGETS ufsrvclilib_static
        EXPORT ufsrvclilib-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    if(BUILD_SHARED_LIBS)
        install(TARGETS ufsrvclilib_shared
            EXPORT ufsrvclilib-targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()

    # The vendored runtime is part of the static link line, so it is installed
    # and exported alongside the library; without it a consumer's only
    # find_package() would leave ufsrvclilib::protobuf-c unresolved.
    install(TARGETS protobuf-c
        EXPORT ufsrvclilib-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

    install(DIRECTORY include/ufsrvclilib DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ufsrvclilib/config_ufsrvclilib.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ufsrvclilib)

    install(EXPORT ufsrvclilib-targets
        FILE ufsrvclilibTargets.cmake
        NAMESPACE ufsrvclilib::
        DESTINATION ${UFSRVCLILIB_INSTALL_CONFIGDIR})

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ufsrvclilibConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/ufsrvclilibConfig.cmake
        INSTALL_DESTINATION ${UFSRVCLILIB_INSTALL_CONFIGDIR})

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/ufsrvclilibConfigVersion.cmake
        VERSION ${UFSRVCLILIB_VERSION}
        COMPATIBILITY AnyNewerVersion)

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ufsrvclilibConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ufsrvclilibConfigVersion.cmake
        DESTINATION ${UFSRVCLILIB_INSTALL_CONFIGDIR})

    configure_file(cmake/ufsrvclilib.pc.in ${CMAKE_CURRENT_BINARY_DIR}/ufsrvclilib.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ufsrvclilib.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

    set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)
    export(PACKAGE ufsrvclilib)

    # Informational install check (never fails the build): version drift across
    # install / git / working tree, and the last origin commit's artefact.
    add_custom_target(install_verification
        COMMAND ${CMAKE_COMMAND}
            -DINSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
            -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/cmake/install_verification.cmake
        COMMENT "Verifying ufsrvclilib version across system install, git, and local copy"
        VERBATIM
    )
endif()

ufsrv_apply_build_options(ufsrvclilib_objects)
ufsrv_apply_build_options(ufsrvclilib_static)
if(BUILD_SHARED_LIBS)
    ufsrv_apply_build_options(ufsrvclilib_shared)
endif()
