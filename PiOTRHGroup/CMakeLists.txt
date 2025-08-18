add_executable(PiOTRHGroup
    src/PiOTRHGroup.cpp
    src/Ped.cpp
    src/tester.cpp
    test/main.cpp
)

target_include_directories(PiOTRHGroup PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiOTRHGroup
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)