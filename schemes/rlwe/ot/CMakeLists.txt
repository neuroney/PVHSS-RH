add_executable(PiOTRLWE
    src/Ped.cpp
    src/PiOTRLWE.cpp
    src/VHSSRLWE.cpp
    src/tester.cpp
    test/main.cpp
)

target_include_directories(PiOTRLWE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiOTRLWE
    PRIVATE
        common
)
