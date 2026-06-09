cmake_minimum_required(VERSION 3.10)

# This benchmark compares the original HSSRLWE (NTL-based) with lhss (RNS/NTT-based)
# for N=32768 polynomial evaluations.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# lhss include paths
set(LHSS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lhss/include)
set(LHSS_LATTICE_INCLUDE_DIR ${LHSS_INCLUDE_DIR}/lattice)
set(LHSS_LHSS_INCLUDE_DIR ${LHSS_INCLUDE_DIR}/lhss)

# Find gmpxx (C++ wrapper for GMP)
find_library(GMPXX_LIB NAMES gmpxx)

add_executable(hss_lhss_bench
    hss_lhss_bench.cpp
)

target_include_directories(hss_lhss_bench PRIVATE
    ${LHSS_INCLUDE_DIR}
    ${LHSS_LATTICE_INCLUDE_DIR}
    ${LHSS_LHSS_INCLUDE_DIR}
    ${GMP_INCLUDE_PATHS}
    ${NTL_INCLUDE_PATHS}
    ${RLC_INCLUDE_PATHS}
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/schemes/rlwe/hss/include
)

target_link_libraries(hss_lhss_bench PRIVATE
    ${GMPXX_LIB}
    ${GMP_LIBRARIES}
    ${NTL_LIBRARIES}
    ${RLC_LIBRARIES}
    common
)
