add_executable(microbench
    microbench.cpp
    ${CMAKE_SOURCE_DIR}/schemes/group/hss/src/HSSElg.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/hss/src/HSSRLWE.cpp
)

target_include_directories(microbench PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/group/hss/include
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/hss/include
)

target_link_libraries(microbench
    PRIVATE
        common
)

add_executable(microbench_vhss_rlwe
    microbench_vhss_rlwe.cpp
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/vhss/src/VHSSRLWE.cpp
)

target_include_directories(microbench_vhss_rlwe PRIVATE
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/vhss/include
)

target_link_libraries(microbench_vhss_rlwe
    PRIVATE
        common
)

add_custom_target(microbenchmark_results
    COMMAND python3
        ${CMAKE_CURRENT_SOURCE_DIR}/run_microbenchmarks.py
        --build-dir ${CMAKE_BINARY_DIR}
        --out-dir ${CMAKE_SOURCE_DIR}/benchmarks/results/micro
    DEPENDS microbench microbench_vhss_rlwe
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
