cmake_minimum_required(VERSION 3.10)
project(PVHSS VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(NTL REQUIRED)
find_package(GMP REQUIRED)
find_package(RLC REQUIRED)

include_directories(${NTL_INCLUDE_PATHS} ${GMP_INCLUDE_PATHS})

include_directories(
    ${NTL_INCLUDE_PATHS}
    ${GMP_INCLUDE_PATHS}
    ${RLC_INCLUDE_PATHS}
    ${CMAKE_SOURCE_DIR}/include
)

link_directories(${PVHSS_LIBRARY_DIR})

file(GLOB SOURCES 
    "test/main.cpp"
    "src/*.cpp"
)

add_executable(PVHSS ${SOURCES})
target_link_libraries(PVHSS ${NTL_LIBRARIES}
${GMP_LIBRARIES} ${RLC_LIBRARIES} m pthread )