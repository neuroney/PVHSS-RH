file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(GroupHSS
    ${SOURCES}
    test/main.cpp
)

target_include_directories(GroupHSS PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(GroupHSS
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)