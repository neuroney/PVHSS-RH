# Group HSS backend — header-only (wraps existing common + schemes/group/hss)
add_library(pvhss_group_hss_backend INTERFACE)
target_include_directories(pvhss_group_hss_backend INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(pvhss_group_hss_backend INTERFACE
    common
    pvhss_programs
)
