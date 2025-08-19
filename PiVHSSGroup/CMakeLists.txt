add_executable(PiVHSSGroup
    src/tester.cpp
    test/main.cpp
)

target_include_directories(PiVHSSGroup PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiVHSSGroup
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)