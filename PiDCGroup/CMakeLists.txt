file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(PiDCGroup
    ${SOURCES}
    test/main.cpp
)

target_include_directories(PiDCGroup PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiDCGroup
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)