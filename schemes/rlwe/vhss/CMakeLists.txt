add_executable(PiVHSSRLWE
    src/VHSSRLWE.cpp
    src/tester.cpp
    test/main.cpp
)

target_include_directories(PiVHSSRLWE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiVHSSRLWE
    PRIVATE
        common
)
