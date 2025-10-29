add_executable(PiOTGroup
    src/PiOTGroup.cpp
    src/Ped.cpp
    src/tester.cpp
    test/main.cpp
)

target_include_directories(PiOTGroup PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiOTGroup
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)