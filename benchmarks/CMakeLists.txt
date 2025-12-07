add_executable(bench_chunking
    bench_chunking.cpp
)

target_link_libraries(bench_chunking
    dropbox_common
)

add_executable(bench_dedup
    bench_dedup.cpp
)

target_link_libraries(bench_dedup
    dropbox_common
)

add_executable(bench_delta_sync
    bench_delta_sync.cpp
)

target_link_libraries(bench_delta_sync
    dropbox_common
)

# Add custom target to run all benchmarks
add_custom_target(run_benchmarks
    COMMAND echo "Running chunking benchmark..."
    COMMAND bench_chunking
    COMMAND echo ""
    COMMAND echo "Running deduplication benchmark..."
    COMMAND bench_dedup
    COMMAND echo ""
    COMMAND echo "Running delta sync benchmark..."
    COMMAND bench_delta_sync
    DEPENDS bench_chunking bench_dedup bench_delta_sync
)
