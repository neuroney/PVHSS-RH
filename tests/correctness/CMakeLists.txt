# ---- RMS Program Layer Tests ----
add_executable(test_programs
    test_programs.cpp
)

target_include_directories(test_programs PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(test_programs
    PRIVATE
        pvhss_programs
        pvhss_external_deps
)

add_test(NAME test_programs COMMAND test_programs)

add_executable(p5_ot_group_acc
    p5_ot_group_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/ot/group_ot/src/PiOTGroup.cpp
    ${CMAKE_SOURCE_DIR}/schemes/ot/group_ot/src/Ped.cpp
)

target_include_directories(p5_ot_group_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/ot/group_ot/include
)

target_link_libraries(p5_ot_group_acc
    PRIVATE
        common
)

add_executable(p5_dc_group_acc
    p5_dc_group_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/dc/group_dc/src/PiDCGroup.cpp
    ${CMAKE_SOURCE_DIR}/schemes/dc/group_dc/src/DecPed.cpp
)

target_include_directories(p5_dc_group_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/dc/group_dc/include
)

target_link_libraries(p5_dc_group_acc
    PRIVATE
        common
)

add_executable(p5_tcc25_group_acc
    p5_tcc25_group_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/tcc25/src/Tcc25Group.cpp
    ${CMAKE_SOURCE_DIR}/schemes/hss/group_hss/src/HSSElg.cpp
)

target_include_directories(p5_tcc25_group_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/tcc25/include
    ${CMAKE_SOURCE_DIR}/schemes/hss/group_hss/include
)

target_link_libraries(p5_tcc25_group_acc
    PRIVATE
        common
)

# ---- RLWE VHSS P5 ----
add_executable(p5_vhss_rlwe_acc
    p5_vhss_rlwe_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/vhss/rlwe_vhss/src/VHSSRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/vhss/rlwe_vhss/src/tester.cpp
)

target_include_directories(p5_vhss_rlwe_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/vhss/rlwe_vhss/include
)

target_link_libraries(p5_vhss_rlwe_acc
    PRIVATE
        common
)

# ---- RLWE OT P5 ----
# Note: VHSSRLWE.cpp has its own HSS functions; do NOT link PiHSSRLWE to avoid conflicts.
add_executable(p5_ot_rlwe_acc
    p5_ot_rlwe_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/ot/rlwe_ot/src/PiOTRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/ot/rlwe_ot/src/VHSSRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/ot/rlwe_ot/src/Ped.cpp
)

target_include_directories(p5_ot_rlwe_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/ot/rlwe_ot/include
)

target_link_libraries(p5_ot_rlwe_acc
    PRIVATE
        common
)

# ---- RLWE DC P5 ----
add_executable(p5_dc_rlwe_acc
    p5_dc_rlwe_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/dc/rlwe_dc/src/PiDCRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/dc/rlwe_dc/src/VHSSRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/dc/rlwe_dc/src/DecPed.cpp
)

target_include_directories(p5_dc_rlwe_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/dc/rlwe_dc/include
)

target_link_libraries(p5_dc_rlwe_acc
    PRIVATE
        common
)

# ---- Tests ----
add_test(NAME p5_ot_group_acc COMMAND p5_ot_group_acc)
add_test(NAME p5_dc_group_acc COMMAND p5_dc_group_acc)
add_test(NAME p5_tcc25_group_acc COMMAND p5_tcc25_group_acc)
add_test(NAME p5_vhss_rlwe_acc COMMAND p5_vhss_rlwe_acc)
add_test(NAME p5_ot_rlwe_acc COMMAND p5_ot_rlwe_acc)
add_test(NAME p5_dc_rlwe_acc COMMAND p5_dc_rlwe_acc)

set_tests_properties(p5_ot_group_acc PROPERTIES LABELS "correctness;fast;p5")
set_tests_properties(p5_dc_group_acc PROPERTIES LABELS "correctness;fast;p5")
set_tests_properties(p5_tcc25_group_acc PROPERTIES LABELS "correctness;fast;p5;group")
set_tests_properties(p5_vhss_rlwe_acc PROPERTIES LABELS "correctness;fast;p5;rlwe")
set_tests_properties(p5_ot_rlwe_acc PROPERTIES LABELS "correctness;fast;p5;rlwe")
set_tests_properties(p5_dc_rlwe_acc PROPERTIES LABELS "correctness;fast;p5;rlwe")

add_custom_target(check_correctness_fast
    COMMAND ${CMAKE_CTEST_COMMAND}
        --test-dir ${CMAKE_BINARY_DIR}
        -L fast
        --output-on-failure
    DEPENDS p5_ot_group_acc p5_dc_group_acc p5_tcc25_group_acc p5_vhss_rlwe_acc p5_ot_rlwe_acc p5_dc_rlwe_acc
)

add_custom_target(check_correctness
    COMMAND ${CMAKE_CTEST_COMMAND}
        --test-dir ${CMAKE_BINARY_DIR}
        -L correctness
        --output-on-failure
    DEPENDS p5_ot_group_acc p5_dc_group_acc p5_tcc25_group_acc p5_vhss_rlwe_acc p5_ot_rlwe_acc p5_dc_rlwe_acc
)
