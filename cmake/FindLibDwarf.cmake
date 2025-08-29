# - Try to find libdwarf
# Once done this will define
#
#  LIBDWARF_FOUND - system has libdwarf
#  LIBDWARF_INCLUDE_DIRS - the libdwarf include directory
#  LIBDWARF_LIBRARIES - Link these to use libdwarf
#  LIBDWARF_DEFINITIONS - Compiler switches required for using libdwarf
#

# Locate libelf library at first
if (NOT LIBELF_FOUND)
   find_package (LibElf)
endif (NOT LIBELF_FOUND)

if (LIBDWARF_LIBRARIES AND LIBDWARF_INCLUDE_DIRS)
  set (LibDwarf_FIND_QUIETLY ON)
endif (LIBDWARF_LIBRARIES AND LIBDWARF_INCLUDE_DIRS)

find_package(PkgConfig QUIET)

if(PKG_CONFIG_FOUND)
  set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
  pkg_check_modules(PC_LIBDW QUIET libdw)
endif()

find_path (LIBDWARF_INCLUDE_DIRS
    NAMES
      libdwarf.h
    HINTS
      ${PC_LIBDW_INCLUDE_DIRS}
    PATHS
      /usr/include
      /usr/local/include
      /usr/local/include/libdwarf-0/
      /opt/local/include
      /sw/include
      ENV CPATH) # PATH and INCLUDE will also work

find_library (LIBDWARF_LIBRARIES
    NAMES
      dwarf
    HINTS
      ${PC_LIBDW_LIBRARY_DIRS}
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
      ENV LIBRARY_PATH   # PATH and LIB will also work
      ENV LD_LIBRARY_PATH)

include (FindPackageHandleStandardArgs)


# handle the QUIETLY and REQUIRED arguments and set LIBDWARF_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibDwarf DEFAULT_MSG
    LIBDWARF_LIBRARIES
    LIBDWARF_INCLUDE_DIRS)