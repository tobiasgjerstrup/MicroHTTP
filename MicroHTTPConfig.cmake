set(MicroHTTP_VERSION "0.1.0")

# Installed in: <prefix>/lib*/cmake/MicroHTTP/MicroHTTPConfig.cmake
get_filename_component(_microhttp_cmake_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_microhttp_lib_dir "${_microhttp_cmake_dir}/../.." ABSOLUTE)
get_filename_component(_microhttp_prefix "${_microhttp_lib_dir}/.." ABSOLUTE)

set(MicroHTTP_INCLUDE_DIR "${_microhttp_prefix}/include")
set(MicroHTTP_LIBRARY "${_microhttp_lib_dir}/libmicrohttp.so")
set(MicroHTTP_STATIC_LIBRARY "${_microhttp_lib_dir}/libmicrohttp.a")

if(NOT TARGET MicroHTTP::microhttp)
    add_library(MicroHTTP::microhttp SHARED IMPORTED)
    set_target_properties(MicroHTTP::microhttp PROPERTIES
        IMPORTED_LOCATION "${MicroHTTP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MicroHTTP_INCLUDE_DIR}"
    )
endif()

if(NOT TARGET MicroHTTP::microhttp_static)
    add_library(MicroHTTP::microhttp_static STATIC IMPORTED)
    set_target_properties(MicroHTTP::microhttp_static PROPERTIES
        IMPORTED_LOCATION "${MicroHTTP_STATIC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MicroHTTP_INCLUDE_DIR}"
    )
endif()
