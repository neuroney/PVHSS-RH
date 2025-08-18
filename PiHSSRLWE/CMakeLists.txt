file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(PiHSSRLWE
    ${SOURCES}
    test/main.cpp
)

target_include_directories(PiHSSRLWE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiHSSRLWE
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)