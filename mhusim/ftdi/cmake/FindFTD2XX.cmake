# - Find FTD2XX library installation
# This module tries to find the libftd2xx installation on your system.
# Once done this will define
#

FIND_PATH(FTD2XX_INCLUDE_DIR
        NAMES   ftd2xx.h WinTypes.h
        PATHS   /usr/local/include
        /usr/include
        /usr/include/libftd2xx
        /usr/local/include/libftd2xx
        /opt/local/include
        /sw/include
        ${extern_lib_path}
        )

# determine if we run a 64bit compiler or not
set(bitness i386)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(bitness amd64)
endif()

SET(FTD2XX_LIBNAME ftd2xx)
IF(WIN32)
    SET(FTD2XX_LIBNAME ftd2xx.lib)
ENDIF(WIN32)

FIND_LIBRARY(FTD2XX_LIBRARY
        NAMES ${FTD2XX_LIBNAME}
        PATHS /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
        ${extern_lib_path}/${bitness}
        )

# set path to DLL for later installation
IF(WIN32 AND FTD2XX_LIBRARY)
    get_filename_component(ftd2xx_lib_path ${FTD2XX_LIBRARY} PATH)
    SET(FTD2XX_DLL ${ftd2xx_lib_path}/ftd2xx.dll)
endif(WIN32 AND FTD2XX_LIBRARY)

IF (FTD2XX_LIBRARY)
    IF(FTD2XX_INCLUDE_DIR)
        MESSAGE(STATUS "Found libFTD2XX: ${FTD2XX_INCLUDE_DIR}, ${FTD2XX_LIBRARY}")
    ELSE(FTD2XX_INCLUDE_DIR)
        MESSAGE(STATUS "libFTD2XX headers NOT FOUND. Make sure to install the development headers! Please refer to the documentation for instructions.")
    ENDIF(FTD2XX_INCLUDE_DIR)
ELSE (FTD2XX_LIBRARY)
    MESSAGE(STATUS "libFTD2XX NOT FOUND.\n Download it from http://www.ftdichip.com/Drivers/D2XX.htm")
ENDIF (FTD2XX_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set FTD2XX_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FTD2XX  DEFAULT_MSG
        FTD2XX_LIBRARY FTD2XX_INCLUDE_DIR)

SET(FTD2XX_INCLUDE_DIR  ${FTD2XX_INCLUDE_DIR} )
