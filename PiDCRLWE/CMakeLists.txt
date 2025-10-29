file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(PiDCRLWE
    ${SOURCES}
    test/main.cpp
)

target_include_directories(PiDCRLWE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiDCRLWE
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)