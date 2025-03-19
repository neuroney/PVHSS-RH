file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(GroupPi2
    ${SOURCES}
    test/main.cpp
)

target_include_directories(GroupPi2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(GroupPi2
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)