add_executable(p5_ot_acc
    p5_ot_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/ot/src/PiOTGroup.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/ot/src/Ped.cpp
)

target_include_directories(p5_ot_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/group/ot/include
)

target_link_libraries(p5_ot_acc
    PRIVATE
        common
)

add_executable(p5_dc_acc
    p5_dc_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/dc/src/PiDCGroup.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/dc/src/DecPed.cpp
)

target_include_directories(p5_dc_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/group/dc/include
)

target_link_libraries(p5_dc_acc
    PRIVATE
        common
)

add_test(NAME p5_ot_acc COMMAND p5_ot_acc)
add_test(NAME p5_dc_acc COMMAND p5_dc_acc)

set_tests_properties(p5_ot_acc PROPERTIES LABELS "correctness;fast;p5")
set_tests_properties(p5_dc_acc PROPERTIES LABELS "correctness;fast;p5")

add_custom_target(check_correctness_fast
    COMMAND ${CMAKE_CTEST_COMMAND}
        --test-dir ${CMAKE_BINARY_DIR}
        -L fast
        --output-on-failure
    DEPENDS p5_ot_acc p5_dc_acc
)

add_custom_target(check_correctness
    COMMAND ${CMAKE_CTEST_COMMAND}
        --test-dir ${CMAKE_BINARY_DIR}
        -L correctness
        --output-on-failure
    DEPENDS p5_ot_acc p5_dc_acc
)
