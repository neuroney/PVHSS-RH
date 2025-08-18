file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(PiHSSGroup
    ${SOURCES}
    test/main.cpp
)

target_include_directories(PiHSSGroup PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiHSSGroup
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)