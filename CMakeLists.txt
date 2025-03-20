cmake_minimum_required(VERSION 3.10)
project(PVHSS-RH VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(NTL REQUIRED)
find_package(GMP REQUIRED)
find_package(RLC REQUIRED)

include_directories(
    ${NTL_INCLUDE_PATHS}
    ${GMP_INCLUDE_PATHS}
    ${RLC_INCLUDE_PATHS}
)

add_subdirectory(common)
add_subdirectory(GroupPi1)
add_subdirectory(GroupPi2)
add_subdirectory(GroupHSS)
add_subdirectory(RLWEHSS)

