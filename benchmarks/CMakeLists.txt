add_subdirectory(micro)
add_subdirectory(protocols)
add_subdirectory(overhead)

add_custom_target(incremental_timing
    COMMAND $<TARGET_FILE:incremental_timing_generator>
        --protocol-components ${CMAKE_SOURCE_DIR}/benchmarks/results/protocols/logs/protocol_components.csv
        --out-dir ${CMAKE_SOURCE_DIR}/benchmarks/results/overhead
    DEPENDS incremental_timing_generator
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(benchmark_tables_p5
    COMMAND $<TARGET_FILE:incremental_timing_generator>
        --protocol-components ${CMAKE_SOURCE_DIR}/benchmarks/results/protocols/logs/protocol_components.csv
        --out-dir ${CMAKE_SOURCE_DIR}/benchmarks/results/overhead
    DEPENDS
        microbenchmark_results
        protocol_benchmarks_p5
        incremental_timing_generator
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(benchmark_tables
    COMMAND $<TARGET_FILE:incremental_timing_generator>
        --protocol-components ${CMAKE_SOURCE_DIR}/benchmarks/results/protocols/logs/protocol_components.csv
        --out-dir ${CMAKE_SOURCE_DIR}/benchmarks/results/overhead
    DEPENDS
        microbenchmark_results
        protocol_benchmarks
        incremental_timing_generator
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
