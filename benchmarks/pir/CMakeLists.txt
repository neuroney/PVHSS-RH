set(ROOT ${CMAKE_SOURCE_DIR})
set(S ${ROOT}/schemes)

add_executable(pir_group_dc
    pir_group_dc.cpp
    ${S}/dc/group_dc/src/DecPed.cpp
    ${S}/dc/group_dc/src/PiDCGroup.cpp
)

set_target_properties(pir_group_dc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks/pir
)

target_include_directories(pir_group_dc PRIVATE
    ${ROOT}
    ${ROOT}/common/include
    ${ROOT}/benchmarks/common
    ${S}/dc/group_dc/include
    ${S}/hss/group_hss/include
)

target_link_libraries(pir_group_dc PRIVATE common)
