# Root source dir
set(ROOT ${CMAKE_SOURCE_DIR})
# Schemes base
set(S ${ROOT}/schemes)

# ---- Helper macro for new RMS benchmarks ----
macro(add_rms_bench NAME PROTO_DIR SRC_FILES EXTRA_INCLUDES)
    add_executable(${NAME} ${ARGN})
    target_include_directories(${NAME} PRIVATE
        ${ROOT}
        ${ROOT}/common/include
        ${ROOT}/programs/include
        ${ROOT}/benchmarks/common
        ${PROTO_DIR}
        ${EXTRA_INCLUDES}
    )
    target_link_libraries(${NAME} PRIVATE common pvhss_programs)
    # Add source files
    foreach(f ${SRC_FILES})
        target_sources(${NAME} PRIVATE ${f})
    endforeach()
endmacro()

# ============================================================
# 1. Group HSS
# ============================================================
add_rms_bench(protocol_hss_rms_bench
    "${S}/hss/group_hss/include"
    "${S}/hss/group_hss/src/HSSElg.cpp"
    "${S}/hss/group_hss/include;${ROOT}/backends/group_hss/include"
    protocol_bench_hss.cpp
)

# ============================================================
# 2. Group VHSS
# ============================================================
add_rms_bench(protocol_vhss_rms_bench
    "${S}/vhss/group_vhss/include"
    "${S}/hss/group_hss/src/HSSElg.cpp"
    "${S}/vhss/group_vhss/include;${ROOT}/backends/group_vhss/include;${S}/hss/group_hss/include"
    protocol_bench_vhss.cpp
)

# ============================================================
# 3. Group OT
# ============================================================
add_rms_bench(protocol_ot_rms_bench
    "${S}/ot/group_ot/include"
    "${S}/ot/group_ot/src/Ped.cpp;${S}/ot/group_ot/src/PiOTGroup.cpp"
    "${S}/ot/group_ot/include;${S}/hss/group_hss/include;${ROOT}/backends/group_vhss/include"
    protocol_bench_ot.cpp
)

# ============================================================
# 4. Group DC
# ============================================================
add_rms_bench(protocol_dc_rms_bench
    "${S}/dc/group_dc/include"
    "${S}/dc/group_dc/src/DecPed.cpp;${S}/dc/group_dc/src/PiDCGroup.cpp"
    "${S}/dc/group_dc/include;${S}/hss/group_hss/include;${ROOT}/backends/group_vhss/include"
    protocol_bench_dc.cpp
)

# ============================================================
# 5. Group TCC25
# ============================================================
add_rms_bench(protocol_tcc25_rms_bench
    "${S}/tcc25/include"
    "${S}/tcc25/src/Tcc25Group.cpp;${S}/hss/group_hss/src/HSSElg.cpp"
    "${S}/tcc25/include;${S}/hss/group_hss/include"
    protocol_bench_tcc25.cpp
)

# ============================================================
# 6. CZ
# ============================================================
add_rms_bench(protocol_cz_rms_bench
    "${S}/cz"
    ""
    "${S}/cz;${ROOT}/backends/cz/include"
    protocol_bench_cz_new.cpp
)

# ============================================================
# 7. RLWE HSS
# ============================================================
add_rms_bench(protocol_rlwe_hss_rms_bench
    "${S}/hss/rlwe_hss/include"
    "${S}/hss/rlwe_hss/src/HSSRLWE.cpp"
    "${S}/hss/rlwe_hss/include"
    protocol_bench_rlwe_hss.cpp
)

# ============================================================
# 8. RLWE VHSS
# ============================================================
add_rms_bench(protocol_rlwe_vhss_rms_bench
    "${S}/vhss/rlwe_vhss/include"
    "${S}/vhss/rlwe_vhss/src/VHSSRLWE.cpp"
    "${S}/vhss/rlwe_vhss/include"
    protocol_bench_rlwe_vhss.cpp
)

# ============================================================
# 9. RLWE OT
# ============================================================
add_rms_bench(protocol_rlwe_ot_rms_bench
    "${S}/ot/rlwe_ot/include"
    "${S}/ot/rlwe_ot/src/Ped.cpp;${S}/ot/rlwe_ot/src/PiOTRLWE.cpp;${S}/ot/rlwe_ot/src/VHSSRLWE.cpp"
    "${S}/ot/rlwe_ot/include;${S}/vhss/rlwe_vhss/include"
    protocol_bench_rlwe_ot.cpp
)

# ============================================================
# 10. RLWE DC
# ============================================================
add_rms_bench(protocol_rlwe_dc_rms_bench
    "${S}/dc/rlwe_dc/include"
    "${S}/dc/rlwe_dc/src/DecPed.cpp;${S}/dc/rlwe_dc/src/PiDCRLWE.cpp;${S}/dc/rlwe_dc/src/VHSSRLWE.cpp"
    "${S}/dc/rlwe_dc/include;${S}/vhss/rlwe_vhss/include"
    protocol_bench_rlwe_dc.cpp
)

# ---- Old benchmark runner (compatibility) ----
add_executable(protocol_benchmark_runner protocol_benchmark_runner.cpp)
target_link_libraries(protocol_benchmark_runner PRIVATE common)
