add_executable(PiCZ main.cpp  tool.h GenData.h PVHSS.h  PKE.h  function.h PVHSS.h)

target_link_libraries(PiCZ
    PRIVATE
        common
        ${NTL_LIBRARIES}
        ${GMP_LIBRARIES}
        m
        ${RLC_LIBRARIES}
)