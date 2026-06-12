add_library(pvhss_cz_backend INTERFACE)
target_include_directories(pvhss_cz_backend INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(pvhss_cz_backend INTERFACE
    common
    pvhss_programs
)
