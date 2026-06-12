add_executable(p5_ot_group_acc
    p5_ot_group_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/ot/src/PiOTGroup.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/ot/src/Ped.cpp
)

target_include_directories(p5_ot_group_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/group/ot/include
)

target_link_libraries(p5_ot_group_acc
    PRIVATE
        common
)

add_executable(p5_dc_group_acc
    p5_dc_group_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/dc/src/PiDCGroup.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/dc/src/DecPed.cpp
)

target_include_directories(p5_dc_group_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/group/dc/include
)

target_link_libraries(p5_dc_group_acc
    PRIVATE
        common
)

add_executable(p5_tcc25_group_acc
    p5_tcc25_group_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/tcc25/src/Tcc25Group.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/hss/src/HSSElg.cpp
)

target_include_directories(p5_tcc25_group_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/group/tcc25/include
    ${CMAKE_SOURCE_DIR}/schemes/group/hss/include
)

target_link_libraries(p5_tcc25_group_acc
    PRIVATE
        common
)

# ---- RLWE VHSS P5 ----
add_executable(p5_vhss_rlwe_acc
    p5_vhss_rlwe_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/vhss/src/VHSSRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/vhss/src/tester.cpp
)

target_include_directories(p5_vhss_rlwe_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/vhss/include
)

target_link_libraries(p5_vhss_rlwe_acc
    PRIVATE
        common
)

# ---- RLWE OT P5 ----
# Note: VHSSRLWE.cpp has its own HSS functions; do NOT link PiHSSRLWE to avoid conflicts.
add_executable(p5_ot_rlwe_acc
    p5_ot_rlwe_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/ot/src/PiOTRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/ot/src/VHSSRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/ot/src/Ped.cpp
)

target_include_directories(p5_ot_rlwe_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/ot/include
)

target_link_libraries(p5_ot_rlwe_acc
    PRIVATE
        common
)

# ---- RLWE DC P5 ----
add_executable(p5_dc_rlwe_acc
    p5_dc_rlwe_acc.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/dc/src/PiDCRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/dc/src/VHSSRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/dc/src/DecPed.cpp
)

target_include_directories(p5_dc_rlwe_acc PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/dc/include
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
