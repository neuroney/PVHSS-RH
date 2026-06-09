# lhss-based RLWE HSS library
# Requires C++17 (lhss uses inline static)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# lhss headers are vendored under this directory
set(LHSS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lhss)

add_library(PiHSSRLWE STATIC
    src/HSSRLWE.cpp
)

target_include_directories(PiHSSRLWE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LHSS_INCLUDE_DIR}
    ${LHSS_INCLUDE_DIR}/lattice
    ${LHSS_INCLUDE_DIR}/lhss
    ${GMP_INCLUDE_PATHS}
    ${NTL_INCLUDE_PATHS}
    ${RLC_INCLUDE_PATHS}
)

# Find gmpxx (C++ wrapper for GMP)
find_library(GMPXX_LIB NAMES gmpxx)
if(NOT GMPXX_LIB)
  message(FATAL_ERROR "gmpxx library not found for PiHSSRLWE")
endif()

target_link_libraries(PiHSSRLWE PRIVATE
    ${GMPXX_LIB}
    ${GMP_LIBRARIES}
    ${NTL_LIBRARIES}
    common
)
