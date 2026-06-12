add_library(pvhss_rlwe_vhss_backend INTERFACE)
target_include_directories(pvhss_rlwe_vhss_backend INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(pvhss_rlwe_vhss_backend INTERFACE
    common
    pvhss_programs
)
