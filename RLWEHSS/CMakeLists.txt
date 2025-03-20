file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(RLWEHSS
    ${SOURCES}
    test/main.cpp
)

target_include_directories(RLWEHSS PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(RLWEHSS
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)