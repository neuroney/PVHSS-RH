add_library(PiHSSRLWE_core STATIC
    src/HSSRLWE.cpp
)

add_executable(PiHSSRLWE
    src/tester.cpp
    test/main.cpp
)

target_include_directories(PiHSSRLWE_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(PiHSSRLWE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PiHSSRLWE_core
    PUBLIC
        common
)

target_link_libraries(PiHSSRLWE
    PRIVATE
        PiHSSRLWE_core
)
