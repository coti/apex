# - Try to find LibSTARPU
# Once done this will define
#  STARPU_FOUND - System has STARPU
#  STARPU_INCLUDE_DIRS - The STARPU include directories
#  STARPU_LIBRARIES - The libraries needed to use STARPU
#  STARPU_DEFINITIONS - Compiler switches required for using STARPU

find_package(PkgConfig)

# This if statement is specific to STARPU, and should not be copied into other
# Find cmake scripts.

if(NOT STARPU_ROOT AND NOT $ENV{STARPU_ROOT} STREQUAL "")
	set(STARPU_ROOT $ENV{STARPU_ROOT})
endif()

pkg_check_modules(PC_STARPU QUIET STARPU)
set(STARPU_DEFINITIONS ${PC_STARPU_CFLAGS_OTHER})

find_path(STARPU_INCLUDE_DIR starpu.h
          HINTS ${STARPU_ROOT}/include
          ${STARPU_ROOT}/include/starpu/1.3
          ${PC_STARPU_INCLUDEDIR}
          ${PC_STARPU_INCLUDE_DIRS}
          ${CMAKE_INSTALL_PREFIX}/include/starpu/1.3
          PATH_SUFFIXES STARPU )

set(TMP_PATH $ENV{LD_LIBRARY_PATH})
if ($TMP_PATH)
	  string(REPLACE ":" " " LD_LIBRARY_PATH_STR $TMP_PATH)
endif()
find_library(STARPU_LIBRARY NAMES starpu-1.3
             HINTS ${STARPU_ROOT}/lib ${STARPU_ROOT}/lib64
             ${PC_STARPU_LIBDIR}
             ${PC_STARPU_LIBRARY_DIRS}
             ${LD_LIBRARY_PATH_STR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set STARPU_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(STARPU  DEFAULT_MSG
                                  STARPU_LIBRARY STARPU_INCLUDE_DIR)

mark_as_advanced(STARPU_INCLUDE_DIR STARPU_LIBRARY)

if(STARPU_FOUND)
  set(STARPU_LIBRARIES ${STARPU_LIBRARY} )
  set(STARPU_INCLUDE_DIRS ${STARPU_INCLUDE_DIR})
  set(STARPU_DIR ${STARPU_ROOT})
  add_definitions(-DAPEX_HAVE_STARPU)
endif()

