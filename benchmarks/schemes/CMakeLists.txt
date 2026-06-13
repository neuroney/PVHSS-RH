# Root source dir
set(ROOT ${CMAKE_SOURCE_DIR})
# Schemes base
set(S ${ROOT}/schemes)

# ---- Helper macro for scheme benchmarks ----
macro(add_scheme_bench NAME SCHEME_DIR SRC_FILES EXTRA_INCLUDES)
    add_executable(${NAME} ${ARGN})
    set_target_properties(${NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks/schemes
    )
    target_include_directories(${NAME} PRIVATE
        ${ROOT}
        ${ROOT}/common/include
        ${ROOT}/benchmarks/common
        ${SCHEME_DIR}
        ${EXTRA_INCLUDES}
    )
    target_link_libraries(${NAME} PRIVATE common)
    # Add source files
    foreach(f ${SRC_FILES})
        target_sources(${NAME} PRIVATE ${f})
    endforeach()
endmacro()

# ============================================================
# 1. Group HSS
# ============================================================
add_scheme_bench(scheme_group_hss
    "${S}/hss/group_hss/include"
    "${S}/hss/group_hss/src/HSSElg.cpp"
    "${S}/hss/group_hss/include"
    scheme_group_hss.cpp
)

# ============================================================
# 2. Group VHSS
# ============================================================
add_scheme_bench(scheme_group_vhss
    "${S}/vhss/group_vhss/include"
    "${S}/hss/group_hss/src/HSSElg.cpp"
    "${S}/vhss/group_vhss/include;${S}/hss/group_hss/include"
    scheme_group_vhss.cpp
)

# ============================================================
# 3. Group OT
# ============================================================
add_scheme_bench(scheme_group_ot
    "${S}/ot/group_ot/include"
    "${S}/ot/group_ot/src/Ped.cpp;${S}/ot/group_ot/src/PiOTGroup.cpp"
    "${S}/ot/group_ot/include;${S}/hss/group_hss/include"
    scheme_group_ot.cpp
)

# ============================================================
# 4. Group DC
# ============================================================
add_scheme_bench(scheme_group_dc
    "${S}/dc/group_dc/include"
    "${S}/dc/group_dc/src/DecPed.cpp;${S}/dc/group_dc/src/PiDCGroup.cpp"
    "${S}/dc/group_dc/include;${S}/hss/group_hss/include"
    scheme_group_dc.cpp
)

# ============================================================
# 5. Group TCC25
# ============================================================
add_scheme_bench(scheme_group_tcc25
    "${S}/tcc25/include"
    "${S}/tcc25/src/Tcc25Group.cpp;${S}/hss/group_hss/src/HSSElg.cpp"
    "${S}/tcc25/include;${S}/hss/group_hss/include"
    scheme_group_tcc25.cpp
)

# ============================================================
# 6. CZ
# ============================================================
add_scheme_bench(scheme_rlwe_cz
    "${S}/cz"
    ""
    "${S}/cz"
    scheme_rlwe_cz.cpp
)

# ============================================================
# 7. RLWE HSS
# ============================================================
add_scheme_bench(scheme_rlwe_hss
    "${S}/hss/rlwe_hss/include"
    "${S}/hss/rlwe_hss/src/HSSRLWE.cpp"
    "${S}/hss/rlwe_hss/include"
    scheme_rlwe_hss.cpp
)

# ============================================================
# 8. RLWE VHSS
# ============================================================
add_scheme_bench(scheme_rlwe_vhss
    "${S}/vhss/rlwe_vhss/include"
    "${S}/vhss/rlwe_vhss/src/VHSSRLWE.cpp"
    "${S}/vhss/rlwe_vhss/include"
    scheme_rlwe_vhss.cpp
)

# ============================================================
# 9. RLWE OT
# ============================================================
add_scheme_bench(scheme_rlwe_ot
    "${S}/ot/rlwe_ot/include"
    "${S}/ot/rlwe_ot/src/Ped.cpp;${S}/ot/rlwe_ot/src/PiOTRLWE.cpp;${S}/ot/rlwe_ot/src/VHSSRLWE.cpp"
    "${S}/ot/rlwe_ot/include;${S}/vhss/rlwe_vhss/include"
    scheme_rlwe_ot.cpp
)

# ============================================================
# 10. RLWE DC
# ============================================================
add_scheme_bench(scheme_rlwe_dc
    "${S}/dc/rlwe_dc/include"
    "${S}/dc/rlwe_dc/src/DecPed.cpp;${S}/dc/rlwe_dc/src/PiDCRLWE.cpp;${S}/dc/rlwe_dc/src/VHSSRLWE.cpp"
    "${S}/dc/rlwe_dc/include;${S}/vhss/rlwe_vhss/include"
    scheme_rlwe_dc.cpp
)


