add_executable(PiHSSGroup
    src/HSSElg.cpp
    src/tester.cpp
    test/main.cpp
)

target_include_directories(PiHSSGroup PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiHSSGroup
    PRIVATE
        common
)
