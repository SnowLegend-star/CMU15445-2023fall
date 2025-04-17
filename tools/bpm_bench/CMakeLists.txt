set(BPM_BENCH_SOURCES bpm_bench.cpp)
add_executable(bpm-bench ${BPM_BENCH_SOURCES} "${PROJECT_SOURCE_DIR}/tools/backtrace.cpp")
add_backward(bpm-bench)

target_link_libraries(bpm-bench bustub)
set_target_properties(bpm-bench PROPERTIES OUTPUT_NAME bustub-bpm-bench)

# 👇 安装可执行文件到 bin 目录（会以 OUTPUT_NAME 命名，即 bustub-bpm-bench）
install(TARGETS bpm-bench RUNTIME DESTINATION bin)