file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.c"
)

add_executable(PiRERHGroup
    ${SOURCES}
    test/main.cpp
)

target_include_directories(PiRERHGroup PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiRERHGroup
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)