function(add_protocol_benchmark_target target backend scheme include_dir time_function)
    add_executable(${target}
        protocol_bench_main.cpp
        ${ARGN}
    )

    target_compile_definitions(${target} PRIVATE
        PVHSS_PROTOCOL_HEADER="tester.h"
        PVHSS_PROTOCOL_TIME_TEST=${time_function}
        PVHSS_PROTOCOL_BACKEND="${backend}"
        PVHSS_PROTOCOL_SCHEME="${scheme}"
    )

    target_include_directories(${target} PRIVATE
        ${include_dir}
    )

    target_link_libraries(${target}
        PRIVATE
            common
    )
endfunction()

add_protocol_benchmark_target(protocol_group_hss_bench group hss
    ${CMAKE_SOURCE_DIR}/schemes/group/hss/include
    HSS_TIME_TEST
    ${CMAKE_SOURCE_DIR}/schemes/group/hss/src/HSSElg.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/hss/src/tester.cpp
)

add_protocol_benchmark_target(protocol_group_vhss_bench group vhss
    ${CMAKE_SOURCE_DIR}/schemes/group/vhss/include
    VHSSElg_TIME_TEST
    ${CMAKE_SOURCE_DIR}/schemes/group/vhss/src/tester.cpp
)

add_protocol_benchmark_target(protocol_group_ot_bench group ot
    ${CMAKE_SOURCE_DIR}/schemes/group/ot/include
    PVHSSElg1_TIME_TEST
    ${CMAKE_SOURCE_DIR}/schemes/group/ot/src/Ped.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/ot/src/PiOTGroup.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/ot/src/tester.cpp
)

add_protocol_benchmark_target(protocol_group_dc_bench group dc
    ${CMAKE_SOURCE_DIR}/schemes/group/dc/include
    PVHSSElg2_TIME_TEST
    ${CMAKE_SOURCE_DIR}/schemes/group/dc/src/DecPed.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/dc/src/PiDCGroup.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/dc/src/tester.cpp
)

add_protocol_benchmark_target(protocol_rlwe_hss_bench rlwe hss
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/hss/include
    HSS_TIME_TEST
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/hss/src/tester.cpp
)

# Link lhss-based HSS library
target_link_libraries(protocol_rlwe_hss_bench PRIVATE PiHSSRLWE)

# Find gmpxx for lhss linkage
find_library(GMPXX_LIB NAMES gmpxx)
if(GMPXX_LIB)
    target_link_libraries(protocol_rlwe_hss_bench PRIVATE ${GMPXX_LIB})
endif()

add_protocol_benchmark_target(protocol_rlwe_vhss_bench rlwe vhss
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/vhss/include
    VHSSRLWE_TIME_TEST
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/vhss/src/VHSSRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/vhss/src/tester.cpp
)

add_protocol_benchmark_target(protocol_rlwe_ot_bench rlwe ot
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/ot/include
    PVHSS_TIME_TEST
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/ot/src/Ped.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/ot/src/PiOTRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/ot/src/VHSSRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/ot/src/tester.cpp
)

add_protocol_benchmark_target(protocol_rlwe_dc_bench rlwe dc
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/dc/include
    PVHSS_TIME_TEST
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/dc/src/DecPed.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/dc/src/PiDCRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/dc/src/VHSSRLWE.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/dc/src/tester.cpp
)

add_executable(protocol_cz_bench
    protocol_bench_cz.cpp
)

target_include_directories(protocol_cz_bench PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/cz
)

target_link_libraries(protocol_cz_bench
    PRIVATE
        common
)

set(PVHSS_PROTOCOL_BENCH_TARGETS
    protocol_group_hss_bench
    protocol_group_vhss_bench
    protocol_group_ot_bench
    protocol_group_dc_bench
    protocol_rlwe_hss_bench
    protocol_rlwe_vhss_bench
    protocol_rlwe_ot_bench
    protocol_rlwe_dc_bench
    protocol_cz_bench
)

add_custom_target(protocol_benchmarks_p5
    COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/run_protocol_benchmarks.py
        --build-dir ${CMAKE_BINARY_DIR}
        --out-dir ${CMAKE_SOURCE_DIR}/benchmarks/results/protocols
        --degrees 5
        --samples 1
    DEPENDS ${PVHSS_PROTOCOL_BENCH_TARGETS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(protocol_benchmarks
    COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/run_protocol_benchmarks.py
        --build-dir ${CMAKE_BINARY_DIR}
        --out-dir ${CMAKE_SOURCE_DIR}/benchmarks/results/protocols
        --degrees 5,10,15
        --samples 1
    DEPENDS ${PVHSS_PROTOCOL_BENCH_TARGETS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
