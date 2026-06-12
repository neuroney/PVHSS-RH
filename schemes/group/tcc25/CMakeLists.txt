add_executable(PiTCC25Group
    src/Tcc25Group.cpp
    src/tester.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/hss/src/HSSElg.cpp
    test/main.cpp
)

target_include_directories(PiTCC25Group PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/schemes/group/hss/include
)

target_link_libraries(PiTCC25Group
    PRIVATE
        common
)
