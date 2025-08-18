file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(PiRERHRLWE
    ${SOURCES}
    test/main.cpp
)

target_include_directories(PiRERHRLWE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiRERHRLWE
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)