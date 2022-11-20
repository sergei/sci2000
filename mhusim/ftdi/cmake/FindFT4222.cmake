# - Find FT4222 library installation
# This module tries to find the libFT4222 installation on your system.

find_package(FTD2XX REQUIRED)

FIND_PATH(FT4222_INCLUDE_DIR
        NAMES   libft4222.h
        PATHS   /usr/local/include
        /usr/local/include/
        /usr/include
        /usr/include/libFT4222
        /usr/local/include/libFT4222
        /opt/local/include
        /sw/include
        ${extern_lib_path}
        )

# determine if we run a 64bit compiler or not
set(bitness i386)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(bitness amd64)
endif()

SET(FT4222_LIBNAME ft4222)
IF(WIN32)
    SET(FT4222_LIBNAME FT4222.lib)
ENDIF(WIN32)

FIND_LIBRARY(FT4222_LIBRARY
        NAMES ${FT4222_LIBNAME}
        PATHS /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
        ${extern_lib_path}/${bitness}
        )

# set path to DLL for later installation
IF(WIN32 AND FT4222_LIBRARY)
    get_filename_component(FT4222_lib_path ${FT4222_LIBRARY} PATH)
    SET(FT4222_DLL ${FT4222_lib_path}/FT4222.dll)
endif(WIN32 AND FT4222_LIBRARY)

IF (FT4222_LIBRARY)
    IF(FT4222_INCLUDE_DIR)
        MESSAGE(STATUS "Found libFT4222: ${FT4222_INCLUDE_DIR}, ${FT4222_LIBRARY}")
    ELSE(FT4222_INCLUDE_DIR)
        MESSAGE(STATUS "libFT4222 headers NOT FOUND. Make sure to install the development headers! Please refer to the documentation for instructions.")
    ENDIF(FT4222_INCLUDE_DIR)
ELSE (FT4222_LIBRARY)
    MESSAGE(STATUS "libFT4222 NOT FOUND. Download it from http://www.ftdichip.com/Products/ICs/FT4222H.html")
ENDIF (FT4222_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set FT4222_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FT4222  DEFAULT_MSG
        FT4222_LIBRARY FT4222_INCLUDE_DIR)

SET(FT4222_INCLUDE_DIR  ${FT4222_INCLUDE_DIR} )
