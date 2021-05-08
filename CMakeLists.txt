project (yafaray)
cmake_minimum_required (VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
   
####### Options ########
option(WITH_Freetype "Build with font rendering FreeType support" ON)
option(WITH_OpenEXR "Build with OpenEXR image I/O support" ON)
option(WITH_JPEG "Build with JPEG image I/O support" ON)
option(WITH_PNG "Build with PNG image I/O support" ON)
option(WITH_TIFF "Build with TIFF image I/O support" ON)
option(WITH_OpenCV "Build OpenCV image processing support" ON)
option(DEBUG_BUILD "Enable debug build mode" OFF)
option(FAST_MATH "Enable mathematic approximations to make code faster" ON)
option(FAST_TRIG "Enable trigonometric approximations to make code faster" ON)
option(WITH_MINGW_STD_THREADS "Use MinGW-Std-Threads 3rd party library. Useful with old MinGW versions that do not include C++11 threads libraries or where they are slower than they should. Set it to OFF with newer versions of MinGW or a conflict might happen causing crashes." OFF)
option(BUILD_TESTS "Build test libYafaRay client examples" ON)

###### Packages and Definitions #########

if(YAF_CMAKE_OSX_SYSROOT)
	SET(CMAKE_OSX_SYSROOT ${YAF_CMAKE_OSX_SYSROOT})
else(YAF_CMAKE_OSX_SYSROOT)
	SET(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk")
endif(YAF_CMAKE_OSX_SYSROOT)

# Load user configurations if available
include(CMakeConfig/UserConfig.txt OPTIONAL)

find_package(Git)

if(NOT YAF_USER_CONFIG)
	include(${CMAKE_BINARY_DIR}/UserConfig.txt OPTIONAL)
endif(NOT YAF_USER_CONFIG)
if(NOT YAF_USER_CONFIG)
	include(${CMAKE_SOURCE_DIR}/UserConfig.txt OPTIONAL)
endif(NOT YAF_USER_CONFIG)

if(NOT YAF_MIN_RAY_DIST)
	set(YAF_MIN_RAY_DIST 0.00005)
endif(NOT YAF_MIN_RAY_DIST)

if(NOT YAF_SHADOW_BIAS)
	set(YAF_SHADOW_BIAS 0.0005)
endif(NOT YAF_SHADOW_BIAS)

if(YAF_USER_INCLUDE_DIRS)
	set(CMAKE_INCLUDE_PATH ${YAF_USER_INCLUDE_DIRS} CACHE STRING "Include search directories" FORCE)
endif(YAF_USER_INCLUDE_DIRS)

if(YAF_CMAKE_DEPENDENCIES_SEARCH_PATH)
	set(CMAKE_PREFIX_PATH ${YAF_CMAKE_DEPENDENCIES_SEARCH_PATH} CACHE STRING "CMAKE dependencies package search directories" FORCE)
endif(YAF_CMAKE_DEPENDENCIES_SEARCH_PATH)

if(YAF_USER_LIBRARY_DIRS)
	set(CMAKE_LIBRARY_PATH ${YAF_USER_LIBRARY_DIRS} CACHE STRING "Library search directories" FORCE)
endif(YAF_USER_LIBRARY_DIRS)

SET(YAF_PROGRAM_PATH "")

if(YAF_USER_EXTRA_LIBS)
	set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} ${YAF_USER_EXTRA_LIBS}" CACHE STRING "Standard C++ libraries" FORCE)
endif(YAF_USER_EXTRA_LIBS)

# find the required libs

if(WITH_OpenCV)
	FIND_PACKAGE(OpenCV COMPONENTS core imgproc photo REQUIRED)
	message("Using OpenCV: yes")
	if(OpenCV_FOUND)
		INCLUDE_DIRECTORIES(SYSTEM ${OpenCV_INCLUDE_DIR} )
		add_definitions(-DHAVE_OPENCV)
	endif(OpenCV_FOUND)
else(WITH_OpenCV)
	message("Using OpenCV: no")
endif(WITH_OpenCV)

if(WITH_OpenEXR OR WITH_PNG OR WITH_XML_LOADER)
	INCLUDE(FindZLIB)
	if(NOT ZLIB_FOUND)
		message(FATAL_ERROR "ZLib not found: if XML Loader, OpenEXR or PNG options are enabled, ZLib is required.")
	endif(NOT ZLIB_FOUND)
endif(WITH_OpenEXR OR WITH_PNG OR WITH_XML_LOADER)

if(WIN32)
	add_definitions(-DWIN32 )
	add_definitions(-D_USE_MATH_DEFINES )
	add_definitions(-D_hypot=hypot )
endif(WIN32)

if(WITH_Freetype)
	find_package(Freetype REQUIRED)
	message("Using FreeType: yes")
	if(FREETYPE_FOUND)
		add_definitions(-DHAVE_FREETYPE)
	endif(FREETYPE_FOUND)
else(WITH_Freetype)
	message("Using FreeType: no")
endif(WITH_Freetype)

if(WITH_OpenEXR)
	find_package(OpenEXR REQUIRED)
	message("Using OpenEXR: yes")
else(WITH_OpenEXR)
	message("Using OpenEXR: no")
endif(WITH_OpenEXR)

if(WITH_JPEG)
	find_package(JPEG REQUIRED)
	message("Using JPEG: yes")
else(WITH_JPEG)
	message("Using JPEG: no")
endif(WITH_JPEG)

if(WITH_PNG)
	find_package(PNG REQUIRED)
	message("Using PNG: yes")
else(WITH_PNG)
	message("Using PNG: no")
endif(WITH_PNG)

if(WITH_TIFF)
	find_package(TIFF REQUIRED)
	message("Using TIFF: yes")
else(WITH_TIFF)
	message("Using TIFF: no")
endif(WITH_TIFF)

if(DEBUG_BUILD)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build mode" FORCE)
else(DEBUG_BUILD)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build mode" FORCE)
endif(DEBUG_BUILD)

if(WIN32 AND MINGW)
	if(WITH_MINGW_STD_THREADS)
		message("Using MinGW_Std_Threads library: yes")
		add_definitions("-DHAVE_MINGW_STD_THREADS")
	set(MINGW_STD_THREADS_INCLUDE_DIR "MINGW_STD_THREADS_INCLUDE_DIR-NOTFOUND")
	find_path(MINGW_STD_THREADS_INCLUDE_DIR mingw-std-threads/mingw.thread.h)
	if(${MINGW_STD_THREADS_INCLUDE_DIR} STREQUAL "MINGW_STD_THREADS_INCLUDE_DIR-NOTFOUND")
		message(FATAL_ERROR "Error: MinGW-Threads headers not found. Download them into the include directory with: git clone https://github.com/meganz/mingw-std-threads")
	else(${MINGW_STD_THREADS_INCLUDE_DIR} STREQUAL "MINGW_STD_THREADS_INCLUDE_DIR-NOTFOUND")
		include_directories(${MINGW_STD_THREADS_INCLUDE_DIR})
	endif(${MINGW_STD_THREADS_INCLUDE_DIR} STREQUAL "MINGW_STD_THREADS_INCLUDE_DIR-NOTFOUND")
	else(WITH_MINGW_STD_THREADS)
		message("Using MinGW_Std_Threads library: no (using standard C++11 threads)")
	endif(WITH_MINGW_STD_THREADS)
endif(WIN32 AND MINGW)

set(YAF_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR})

if (USER_INSTALL_PREFIX)
	set(CMAKE_INSTALL_PREFIX ${USER_INSTALL_PREFIX})
endif (USER_INSTALL_PREFIX)

# Load the packaging config

include(CMakeConfig/CPackConfig.txt)

# installation directories

if(NOT YAF_BIN_DIR)
	set(YAF_BIN_DIR bin)
endif(NOT YAF_BIN_DIR)

if(WIN32)
	set(YAF_LIB_DIR ${YAF_BIN_DIR})
else(WIN32)
	if(NOT YAF_LIB_DIR)
		set(YAF_LIB_DIR lib)
	endif(NOT YAF_LIB_DIR)
endif(WIN32)

# build flags

if(WIN32 AND NOT MINGW) # MSVC
	set(CMAKE_CXX_FLAGS "/DWIN32 /D_WIN32 /D_USE_MATH_DEFINES /DNOMINMAX /EHsc /MD /nologo" CACHE STRING "Default compiler flags" FORCE)
else(WIN32 AND NOT MINGW)
	set(CMAKE_CXX_FLAGS "-Wall -Wno-unused-result" CACHE STRING "Default compiler flags" FORCE)
endif(WIN32 AND NOT MINGW)

if(USER_RELEASE_OPTIMIZATION_FLAGS)
	set(YAF_CXX_FLAGS_RELEASE ${USER_RELEASE_OPTIMIZATION_FLAGS})
else(USER_RELEASE_OPTIMIZATION_FLAGS)
	if(WIN32 AND NOT MINGW) # MSVC
		set(YAF_CXX_FLAGS_RELEASE "/Ot /Ox")
	else(WIN32 AND NOT MINGW)
		set(YAF_CXX_FLAGS_RELEASE "-Wall -std=c++11 -pthread -O3 -ffast-math -Wno-unused-result -fvisibility=hidden -fvisibility-inlines-hidden -Wl,--no-undefined,--version-script=${CMAKE_SOURCE_DIR}/include/interface/interface_c_export_symbols.map")
	endif(WIN32 AND NOT MINGW)
endif(USER_RELEASE_OPTIMIZATION_FLAGS)

if(USER_DEBUG_OPTIMIZATION_FLAGS)
	set(YAF_CXX_FLAGS_DEBUG ${USER_DEBUG_OPTIMIZATION_FLAGS})
else(USER_DEBUG_OPTIMIZATION_FLAGS)
	if(WIN32 AND NOT MINGW) # MSVC
		set(YAF_CXX_FLAGS_DEBUG "/Zi /GS /RTC1 /Od")
	else(WIN32 AND NOT MINGW)
		set(YAF_CXX_FLAGS_DEBUG "-Wall -std=c++11 -pthread -ggdb -Wno-unused-result -fvisibility=hidden -Wl,--no-undefined")
	endif(WIN32 AND NOT MINGW)
endif(USER_DEBUG_OPTIMIZATION_FLAGS)

if(APPLE)
	set(CMAKE_INSTALL_NAME_DIR @rpath)
endif(APPLE)

set(YAF_TARGET_TYPE LIBRARY)

if(WIN32)
	set(YAF_TARGET_TYPE ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib RUNTIME)
endif(WIN32)

if(USER_RELEASE_EXTRA_FLAGS)
	set(YAF_CXX_FLAGS_RELEASE "${YAF_CXX_FLAGS_RELEASE} ${USER_RELEASE_EXTRA_FLAGS}")
endif(USER_RELEASE_EXTRA_FLAGS)

if(USER_DEBUG_EXTRA_FLAGS)
	set(YAF_CXX_FLAGS_DEBUG "${YAF_CXX_FLAGS_DEBUG} ${USER_DEBUG_EXTRA_FLAGS}")
endif(USER_DEBUG_EXTRA_FLAGS)

if(YAF_USER_EXTRA_DEFS)
	add_definitions(${YAF_USER_EXTRA_DEFS})
endif(YAF_USER_EXTRA_DEFS)

# Flags configuration

if (USER_RELFLAGS)
	set(CMAKE_CXX_FLAGS_RELEASE "${YAF_CXX_FLAGS_RELEASE} ${USER_RELFLAGS}" CACHE STRING "Release compiler flags" FORCE)
else (USER_RELFLAGS)
	set(CMAKE_CXX_FLAGS_RELEASE "${YAF_CXX_FLAGS_RELEASE}" CACHE STRING "Release compiler flags" FORCE)
endif (USER_RELFLAGS)

if (USER_DBGFLAGS)
	set(CMAKE_CXX_FLAGS_DEBUG "${YAF_CXX_FLAGS_DEBUG} ${USER_DBGFLAGS}" CACHE STRING "Debug compiler flags" FORCE)
else (USER_DBGFLAGS)
	set(CMAKE_CXX_FLAGS_DEBUG "${YAF_CXX_FLAGS_DEBUG}" CACHE STRING "Debug compiler flags" FORCE)
endif (USER_DBGFLAGS)

# Code speedup due mathematical approximations

if (FAST_MATH)
	add_definitions(-DFAST_MATH)
endif (FAST_MATH)

if (FAST_TRIG)
	add_definitions(-DFAST_TRIG)
endif (FAST_TRIG)

# Adding subdirectories

set(dir include)
file (GLOB_RECURSE headers "${dir}/*.h")

add_subdirectory(src)

if(BUILD_TESTS)
	FIND_PACKAGE(libYafaRay)
	message("Building libYafaRay test code clients: yes")
	if(libYafaRay_FOUND)
		INCLUDE_DIRECTORIES(SYSTEM ${libYafaRay_INCLUDE_DIR} )
		add_subdirectory(tests/test01)
	else(libYafaRay_FOUND)
		message("libYafaRay not found")
		add_subdirectory(tests)
	endif(libYafaRay_FOUND)
else(BUILD_TESTS)
	message("Building libYafaRay test code clients: no")
endif(BUILD_TESTS)

# Generating config headers

include(CMakeConfig/GenConfHeaders.txt)

include(CPack)
