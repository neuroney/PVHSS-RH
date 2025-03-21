file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(RLWEPi2
    ${SOURCES}
    test/main.cpp
)

target_include_directories(RLWEPi2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(RLWEPi2
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)