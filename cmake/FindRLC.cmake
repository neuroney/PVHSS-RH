# FindRLC.cmake - Find the RELIC library
#
# This module defines:
#  RLC_FOUND - True if RELIC was found
#  RLC_INCLUDE_DIR - Include directories for RELIC
#  RLC_LIBRARIES - Libraries to link against RELIC
#  RLC_STATIC_LIBRARY - Path to static library (librelic_s.a)
#  RLC_DYNAMIC_LIBRARY - Path to dynamic library (librelic.dylib)

# Look for the header files
find_path(RLC_INCLUDE_DIR relic.h
  PATHS /usr/local/include
  PATH_SUFFIXES relic
)

# Look for the static library
find_library(RLC_STATIC_LIBRARY
  NAMES relic_s
  PATHS /usr/local/lib
)

# Look for the dynamic library
find_library(RLC_DYNAMIC_LIBRARY
  NAMES relic
  PATHS /usr/local/lib
)

# Set RLC_LIBRARY to the preferred library (dynamic by default)
if(RLC_DYNAMIC_LIBRARY)
  set(RLC_LIBRARY ${RLC_DYNAMIC_LIBRARY})
elseif(RLC_STATIC_LIBRARY)
  set(RLC_LIBRARY ${RLC_STATIC_LIBRARY})
endif()

# Handle the QUIETLY and REQUIRED arguments and set RLC_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RLC DEFAULT_MSG
  RLC_LIBRARY
  RLC_INCLUDE_DIR
)

# Set output variables
if(RLC_FOUND)
  set(RLC_LIBRARIES ${RLC_LIBRARY})
  set(RLC_INCLUDE_PATHS ${RLC_INCLUDE_DIR})
  
  message(STATUS "Found RELIC: ${RLC_LIBRARY}")
endif()