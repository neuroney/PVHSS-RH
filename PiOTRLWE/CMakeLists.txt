file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(PiOTRLWE
    ${SOURCES}
    test/main.cpp
)

target_include_directories(PiOTRLWE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiOTRLWE
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)