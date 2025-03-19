# FindRLC.cmake - Find the RELIC library
#
# This module defines:
#  RLC_FOUND - True if RELIC was found
#  RLC_INCLUDE_DIR - Include directories for RELIC
#  RLC_LIBRARIES - Libraries to link against RELIC
#  RLC_STATIC_LIBRARY - Path to static library (librelic_s.a)
#  RLC_DYNAMIC_LIBRARY - Path to dynamic library (librelic.dylib)

# Use cmake standard find_library package
include(FindPackageHandleStandardArgs)

# Standard `lib` and system specific `lib32/64` directory
list(APPEND lib_suffixes "lib" "${CMAKE_INSTALL_LIBDIR}")

# Standard `include` system specific directory
list(APPEND header_suffixes "include/relic" "${CMAKE_INSTALL_INCLUDEDIR}/relic")

# If CMAKE_LIBRARY_ARCHITECTURE is defined (e.g.: `x86_64-linux-gnu`) add that path
if (CMAKE_LIBRARY_ARCHITECTURE)
  list(APPEND lib_suffixes
       "lib/${CMAKE_LIBRARY_ARCHITECTURE}"
       "${CMAKE_INSTALL_LIBDIR}/${CMAKE_LIBRARY_ARCHITECTURE}")
  list(APPEND header_suffixes
       "include/${CMAKE_LIBRARY_ARCHITECTURE}/relic"
       "${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_LIBRARY_ARCHITECTURE}/relic")
endif (CMAKE_LIBRARY_ARCHITECTURE)

if (RLC_DIR)
  # If user-specified folders: look there
  find_library(RLC_STATIC_LIBRARY
               NAMES relic_s librelic_s
               PATHS ${RLC_DIR}
               PATH_SUFFIXES ${lib_suffixes}
               NO_DEFAULT_PATH
               DOC "RELIC static library")

  find_library(RLC_DYNAMIC_LIBRARY
               NAMES relic librelic
               PATHS ${RLC_DIR}
               PATH_SUFFIXES ${lib_suffixes}
               NO_DEFAULT_PATH
               DOC "RELIC dynamic library")

  find_path(RLC_INCLUDE_DIR
            NAMES relic.h
            PATHS ${RLC_DIR}
            PATH_SUFFIXES ${header_suffixes}
            NO_DEFAULT_PATH
            DOC "RELIC headers")
else (RLC_DIR)
  # Else: look in default paths
  find_library(RLC_STATIC_LIBRARY
               NAMES relic_s librelic_s
               PATH_SUFFIXES ${lib_suffixes}
               DOC "RELIC static library")

  find_library(RLC_DYNAMIC_LIBRARY
               NAMES relic librelic
               PATH_SUFFIXES ${lib_suffixes}
               DOC "RELIC dynamic library")

  find_path(RLC_INCLUDE_DIR
            NAMES relic.h
            PATH_SUFFIXES ${header_suffixes}
            DOC "RELIC headers")
endif (RLC_DIR)

unset(lib_suffixes)
unset(header_suffixes)

# Set RLC_LIBRARY to the preferred library (dynamic by default)
if(RLC_DYNAMIC_LIBRARY)
  set(RLC_LIBRARY ${RLC_DYNAMIC_LIBRARY})
elseif(RLC_STATIC_LIBRARY)
  set(RLC_LIBRARY ${RLC_STATIC_LIBRARY})
endif()

# Try to find RELIC version if header is found
if (RLC_INCLUDE_DIR)
  file(STRINGS "${RLC_INCLUDE_DIR}/relic_conf.h" RLC_VERSION_LINE REGEX "^#define[ \t]+RLC_VERSION[ \t]+\"[0-9]+\\.[0-9]+\\.[0-9]+\"")
  
  string(REGEX REPLACE "^#define[ \t]+RLC_VERSION[ \t]+\"([0-9]+\\.[0-9]+\\.[0-9]+)\"" "\\1" RLC_VERSION "${RLC_VERSION_LINE}")
  
  # Extract major, minor, and patch if needed
  
  string(REGEX REPLACE "([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" RLC_VERSION_MAJOR "${RLC_VERSION}")
  string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" RLC_VERSION_MINOR "${RLC_VERSION}")
  string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" RLC_VERSION_PATCH "${RLC_VERSION}")
  
  
  if(RLC_VERSION_MAJOR AND RLC_VERSION_MINOR AND RLC_VERSION_PATCH)
    set(RLC_VERSION "${RLC_VERSION_MAJOR}.${RLC_VERSION_MINOR}.${RLC_VERSION_PATCH}")
  endif()
endif()

# Raising an error if RELIC has not been found
set(fail_msg
    "RELIC required library has not been found. (Try cmake -DRLC_DIR=<RELIC-root-path>).")
if (RLC_DIR)
  set(fail_msg
      "RELIC required library has not been found in ${RLC_DIR}.")
endif (RLC_DIR)

# Check the library has been found, handle QUIET/REQUIRED arguments
find_package_handle_standard_args(RLC
                                  REQUIRED_VARS RLC_LIBRARY RLC_INCLUDE_DIR
                                  VERSION_VAR RLC_VERSION
                                  FAIL_MESSAGE "${fail_msg}")

unset(fail_msg)

# If RLC has been found set the default variables
if (RLC_FOUND)
  set(RLC_LIBRARIES "${RLC_LIBRARY}")
  set(RLC_INCLUDE_PATHS "${RLC_INCLUDE_DIR}")
  
  # Create an imported target for RELIC
  if(NOT TARGET RLC::RLC)
    add_library(RLC::RLC UNKNOWN IMPORTED)
    set_target_properties(RLC::RLC PROPERTIES
      IMPORTED_LOCATION "${RLC_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${RLC_INCLUDE_DIR}"
    )
  endif()
endif (RLC_FOUND)

# Mark variables as advanced
mark_as_advanced(RLC_INCLUDE_DIR RLC_STATIC_LIBRARY RLC_DYNAMIC_LIBRARY RLC_LIBRARIES RLC_INCLUDE_PATHS)