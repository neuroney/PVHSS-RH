add_executable(PiHSSRLWE
    src/HSSRLWE.cpp
    src/tester.cpp
    test/main.cpp
)

target_include_directories(PiHSSRLWE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PiHSSRLWE
    PRIVATE
        common
)
