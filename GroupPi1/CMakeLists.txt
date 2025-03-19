file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(GroupPi1
    ${SOURCES}
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