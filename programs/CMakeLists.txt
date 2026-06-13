add_library(pvhss_programs STATIC
    src/rms_program.cpp
    src/mpe_program.cpp
    src/plaintext_mpe.cpp
)

target_include_directories(pvhss_programs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pvhss_programs
    PUBLIC
        pvhss_external_deps
)
