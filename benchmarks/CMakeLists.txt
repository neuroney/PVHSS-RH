add_subdirectory(micro)
add_subdirectory(schemes)

add_custom_target(benchmark_tables_p5
    DEPENDS
        microbenchmark_results
        scheme_benchmarks_p5
)

add_custom_target(benchmark_tables
    DEPENDS
        microbenchmark_results
        scheme_benchmarks
)
