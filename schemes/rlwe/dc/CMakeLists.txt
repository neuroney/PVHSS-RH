add_executable(PiDCRLWE
    src/DecPed.cpp
    src/PiDCRLWE.cpp
    src/VHSSRLWE.cpp
    src/tester.cpp
    test/main.cpp
)

target_include_directories(PiDCRLWE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(PiDCRLWE
    PRIVATE
        common
)
