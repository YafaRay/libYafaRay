#
# Try to find older OpenEXR v2.2.x libraries, and include path.
# Once done this will define:
#
# OPENEXR_FOUND = OpenEXR found. 
# OPENEXR_INCLUDE_DIRS = OpenEXR include directories.
# OPENEXR_LIBRARIES = libraries that are needed to use OpenEXR.
# 

find_package(ZLIB)

if(ZLIB_FOUND)
	set(LIBRARY_DIRS 
		/usr/lib
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		$ENV{PROGRAM_FILES}/OpenEXR/lib/static)

	find_path(OPENEXR_INCLUDE_PATH ImfRgbaFile.h
		PATH_SUFFIXES OpenEXR
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include)

	find_library(OPENEXR_HALF_LIBRARY 
		NAMES Half Half-2_2 Half-2_3 Half-2_4
		PATHS ${LIBRARY_DIRS})
  
	find_library(OPENEXR_IEX_LIBRARY 
		NAMES Iex Iex-2_2 Iex-2_3 Iex-2_4
		PATHS ${LIBRARY_DIRS})
 
	find_library(OPENEXR_IMATH_LIBRARY
		NAMES Imath Imath-2_2 Imath-2_3 Imath-2_4
		PATHS ${LIBRARY_DIRS})
  
	find_library(OPENEXR_ILMIMF_LIBRARY
		NAMES IlmImf IlmImf-2_2 IlmImf-2_3 IlmImf-2_4
		PATHS ${LIBRARY_DIRS})

	find_library(OPENEXR_ILMTHREAD_LIBRARY
		NAMES IlmThread IlmThread-2_2 IlmThread-2_3 IlmThread-2_4
		PATHS ${LIBRARY_DIRS})
endif()

IF (OPENEXR_INCLUDE_PATH AND OPENEXR_IMATH_LIBRARY AND OPENEXR_ILMIMF_LIBRARY AND OPENEXR_IEX_LIBRARY AND OPENEXR_HALF_LIBRARY)
	set(OPENEXR_FOUND TRUE)
	set(OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_PATH} CACHE STRING "The include paths needed to use OpenEXR")
	set(OPENEXR_LIBRARIES ${OPENEXR_IMATH_LIBRARY} ${OPENEXR_ILMIMF_LIBRARY} ${OPENEXR_IEX_LIBRARY} ${OPENEXR_HALF_LIBRARY} ${ZLIB_LIBRARY} CACHE STRING "The libraries needed to use OpenEXR")
	if(OPENEXR_ILMTHREAD_LIBRARY)
		set(OPENEXR_LIBRARIES ${OPENEXR_LIBRARIES} ${OPENEXR_ILMTHREAD_LIBRARY})
	endif()
endif()

if(OPENEXR_FOUND)
	if(NOT OPENEXR_FIND_QUIETLY)
		message(STATUS "Found OpenEXR: ${OPENEXR_ILMIMF_LIBRARY}")
	endif()
else()
	if(OPENEXR_FIND_REQUIRED)
		message(FATAL_ERROR "Could not find OpenEXR library")
	endif()
endif()

mark_as_advanced(
	OPENEXR_INCLUDE_DIRS
	OPENEXR_LIBRARIES
	OPENEXR_ILMIMF_LIBRARY
	OPENEXR_IMATH_LIBRARY
	OPENEXR_IEX_LIBRARY
	OPENEXR_HALF_LIBRARY
)
