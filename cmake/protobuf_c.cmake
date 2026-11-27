# protobuf_c.cmake — generate protobuf-c sources for a target.
#
#   include(cmake/protobuf_c.cmake)                    # from the package CMakeLists
#   ufsrvcli_protobuf_c(<target> <proto-folder> <out-folder> [<name>...])
#
# <proto-folder> holds the .proto inputs; <out-folder> receives the generated
# <Name>.pb-c.c / <Name>.pb-c.h pair. Both are resolved against the calling
# CMakeLists.txt's directory, so each schema family keeps its own pair of
# folders — protos can share one directory while their generated code lands
# beside its consumers:
#
#   ufsrvcli_protobuf_c(ufsrvclilib protobuf src/websocket     WebSocketMessage)
#   ufsrvcli_protobuf_c(ufsrvclilib protobuf src/signalservice SignalService)
#
# Without <name> arguments every .proto in <proto-folder> is generated.
#
# Generation is on demand: a .proto edit regenerates its pair on the next build.
# The generated pair is written into the source tree, the same layout
# ufsrvwebsock uses, so a target can be built from a clean checkout once the
# files are committed.
#
# The output folder's parent is added to the target's include path, so the
# generated header is included as <out-folder>/<Name>.pb-c.h.
#
# The caller's target is expected to link the vendored runtime already (the
# package CMakeLists does, ahead of uflib, so the link order favours it). This
# function only generates and adds sources.
#
# Requires the vendored runtime (protobuf-c::protobuf-c, selected by the package
# CMakeLists) and the vendored generator, which exists only when configured with
# -DUFSRVCLI_PROTOBUF_C_GENERATOR=ON.

function(ufsrvcli_protobuf_c target proto_folder out_folder)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "ufsrvcli_protobuf_c: target '${target}' does not exist")
    endif()
    if(NOT UFSRVCLI_PROTOBUF_C_TARGET OR NOT TARGET ${UFSRVCLI_PROTOBUF_C_TARGET})
        message(FATAL_ERROR
            "ufsrvcli_protobuf_c: the vendored protobuf-c runtime is missing; "
            "packages/protobuf-c ships with this repo; check the add_subdirectory at the top of CMakeLists.txt.")
    endif()
    if(NOT TARGET protoc-gen-c)
        message(FATAL_ERROR
            "ufsrvcli_protobuf_c: the vendored generator is not built. Configure with "
            "-DUFSRVCLI_PROTOBUF_C_GENERATOR=ON (needs libprotoc-dev).")
    endif()
    if(NOT Protobuf_PROTOC_EXECUTABLE)
        message(FATAL_ERROR "ufsrvcli_protobuf_c: protoc executable not found (find_package(Protobuf))")
    endif()
    set(generator_cmd "${Protobuf_PROTOC_EXECUTABLE}")
    set(generator_args --plugin=protoc-gen-c=$<TARGET_FILE:protoc-gen-c> "--c_out=${out_dir}")

    get_filename_component(proto_dir "${proto_folder}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(out_dir   "${out_folder}"   ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    file(MAKE_DIRECTORY "${out_dir}")

    set(names ${ARGN})
    if(names)
        foreach(name IN LISTS names)
            if(NOT EXISTS "${proto_dir}/${name}.proto")
                message(FATAL_ERROR "ufsrvcli_protobuf_c: no ${name}.proto under ${proto_dir}")
            endif()
        endforeach()
    else()
        file(GLOB schema_files CONFIGURE_DEPENDS "${proto_dir}/*.proto")
        if(NOT schema_files)
            message(FATAL_ERROR "ufsrvcli_protobuf_c: no .proto files under ${proto_dir}")
        endif()
        foreach(schema IN LISTS schema_files)
            get_filename_component(name "${schema}" NAME_WE)
            list(APPEND names "${name}")
        endforeach()
    endif()

    foreach(name IN LISTS names)
        set(schema      "${proto_dir}/${name}.proto")
        set(generated_c "${out_dir}/${name}.pb-c.c")
        set(generated_h "${out_dir}/${name}.pb-c.h")

        add_custom_command(
            OUTPUT  "${generated_c}" "${generated_h}"
            COMMAND "${generator_cmd}" ${generator_args} "-I${proto_dir}" "${schema}"
            DEPENDS "${schema}"
            COMMENT "protoc-c: ${name}.proto -> ${out_folder}/"
            VERBATIM
        )
        set_source_files_properties("${generated_c}" PROPERTIES GENERATED TRUE)
        set_source_files_properties("${generated_h}" PROPERTIES GENERATED TRUE)
        target_sources(${target} PRIVATE "${generated_c}")
        set_property(TARGET ${target} APPEND PROPERTY SOURCES "${generated_h}")
        set_property(SOURCE "${generated_h}" PROPERTY HEADER_FILE_ONLY TRUE)
    endforeach()

    get_filename_component(out_parent "${out_dir}" DIRECTORY)
    target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${out_parent}>)

    source_group("protobuf-c/${out_folder}" FILES ${generated_c} ${generated_h})
endfunction()
