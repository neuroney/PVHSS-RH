add_library(common STATIC
    src/VHSSElg.cpp
    src/elgamal.cpp
    src/helper.cpp
    src/relic_cp_decped.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(common
    PUBLIC
        pvhss_external_deps
)
