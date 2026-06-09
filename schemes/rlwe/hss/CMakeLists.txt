# NTL-based RLWE HSS library

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(PiHSSRLWE STATIC
    src/HSSRLWE.cpp
)

target_include_directories(PiHSSRLWE PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(PiHSSRLWE PRIVATE
    ${GMP_INCLUDE_PATHS}
    ${NTL_INCLUDE_PATHS}
    ${RLC_INCLUDE_PATHS}
)

target_link_libraries(PiHSSRLWE PRIVATE
    ${GMP_LIBRARIES}
    ${NTL_LIBRARIES}
    common
)
