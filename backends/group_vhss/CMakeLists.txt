# Group VHSS backend — header-only (wraps existing common)
add_library(pvhss_group_vhss_backend INTERFACE)
target_include_directories(pvhss_group_vhss_backend INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(pvhss_group_vhss_backend INTERFACE
    common
    pvhss_programs
)
