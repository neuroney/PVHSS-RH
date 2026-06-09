add_subdirectory(micro)
add_subdirectory(protocols)

add_custom_target(incremental_timing
    COMMAND python3
        ${CMAKE_SOURCE_DIR}/benchmarks/overhead/generate_incremental_overhead.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(benchmark_tables_p5
    COMMAND python3
        ${CMAKE_SOURCE_DIR}/benchmarks/overhead/generate_incremental_overhead.py
    DEPENDS
        microbenchmark_results
        protocol_benchmarks_p5
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(benchmark_tables
    COMMAND python3
        ${CMAKE_SOURCE_DIR}/benchmarks/overhead/generate_incremental_overhead.py
    DEPENDS
        microbenchmark_results
        protocol_benchmarks
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
