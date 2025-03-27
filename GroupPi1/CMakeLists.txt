add_executable(GroupPi1
    src/GroupPi1.cpp
    src/Ped.cpp
    src/tester.cpp
    test/main.cpp
)

target_include_directories(GroupPi1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(GroupPi1
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)