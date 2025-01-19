find_path(OTF2_PREFIX
    NAMES include/otf2/otf2.h
    HINTS $ENV{OTF2_PATH}
    PATHS
    ${CMAKE_PREFIX_PATH}
)

set(OTF2_INCLUDE_DIRS ${OTF2_PREFIX}/include)
find_library(OTF2_LIBRARY
    NAMES otf2
    HINTS ${OTF2_PREFIX}/lib
    PATHS
    ENV LD_LIBRARY_PATH
    ENV LIBRARY_PATH
)

set(OTF2_LIBRARY_DIRS ${OTF2_PREFIX}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OTF2 DEFAULT_MSG OTF2_LIBRARY OTF2_PREFIX)