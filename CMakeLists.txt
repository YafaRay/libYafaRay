project (yafaray)
cmake_minimum_required (VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
   
####### Options ########
option(WITH_YAF_PY_BINDINGS "Enable the YafaRay Python bindings" ON)
option(WITH_YAF_RUBY_BINDINGS "Enable the YafaRay Ruby bindings" OFF)
option(BUILDRELEASE "Enable release mode building of YafaRay" OFF)
option(EMBED_FONT_QT "Embed font for QT GUI (usefull for some buggy QT installations)" OFF)
option(DEBUG_BUILD "Enable debug build mode" OFF)

###### Packages and Definitions #########

# Load a library config file if any
include(CMakeConfig/LibsConfig.txt OPTIONAL)

if(YAF_USER_INCLUDE_DIRS)
	set(CMAKE_INCLUDE_PATH ${YAF_USER_INCLUDE_DIRS} CACHE STRING "Include search directories" FORCE)
endif(YAF_USER_INCLUDE_DIRS)

if(YAF_USER_LIBRARY_DIRS)
	set(CMAKE_LIBRARY_PATH ${YAF_USER_LIBRARY_DIRS} CACHE STRING "Library search directories" FORCE)
endif(YAF_USER_LIBRARY_DIRS)

if(YAF_USER_QT_BIN_DIR AND YAF_USER_SWIG_BIN_DIR)
	set(CMAKE_PROGRAM_PATH ${YAF_USER_QT_BIN_DIR} ${YAF_USER_SWIG_BIN_DIR} CACHE STRING "Executables search directories" FORCE)
endif(YAF_USER_QT_BIN_DIR AND YAF_USER_SWIG_BIN_DIR)

if(YAF_USER_EXTRA_LIBS)
	set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} ${YAF_USER_EXTRA_LIBS}" CACHE STRING "Standard C++ libraries" FORCE)
endif(YAF_USER_EXTRA_LIBS)

# find the required libs
find_package(ZLIB REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(OpenEXR REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)
find_package(Freetype REQUIRED)
find_package(Threads REQUIRED)
find_package(Qt4 4.4.0 COMPONENTS QtCore QtGui REQUIRED)

# Qt enabling header
include(${QT_USE_FILE})

#check for swig if needed
if (WITH_YAF_PY_BINDINGS OR WITH_YAF_RUBY_BINDINGS)
	find_package(SWIG REQUIRED)
	set(YAF_SWIG_MIN_VERSION 1.3.36)
	if(NOT SWIG_FOUND)
		message("Swig not found, please install it to build script bindings")
		set(WITH_YAF_PY_BINDINGS OFF)
		set(WITH_YAF_RUBY_BINDINGS OFF)
	else(NOT SWIG_FOUND)
		if(SWIG_VERSION VERSION_LESS ${YAF_SWIG_MIN_VERSION})
			message("Swig version < ${YAF_SWIG_MIN_VERSION}, please update or bindings can't be built")
			set(WITH_YAF_PY_BINDINGS OFF)
			set(WITH_YAF_RUBY_BINDINGS OFF)
		endif(SWIG_VERSION VERSION_LESS ${YAF_SWIG_MIN_VERSION})
	endif(NOT SWIG_FOUND)
endif (WITH_YAF_PY_BINDINGS OR WITH_YAF_RUBY_BINDINGS)

# Python bindings 
if (WITH_YAF_PY_BINDINGS)
	if(YAF_PY_VERSION)
		set(REQUIRED_PYTHON_VERSION ${YAF_PY_VERSION})
	else(YAF_PY_VERSION)
		set(REQUIRED_PYTHON_VERSION 2.6)
	endif(YAF_PY_VERSION)
	MESSAGE("Searching for Python ${REQUIRED_PYTHON_VERSION}")
	find_package(Python3Libs REQUIRED)
	if (NOT PYTHON_LIBRARIES)
		message("Python wasn't found, please install it to build python bindings")
		set(WITH_YAF_PY_BINDINGS OFF)
	else (NOT PYTHON_LIBRARIES)
		message("Python ${REQUIRED_PYTHON_VERSION} found at: ${PYTHON_LIBRARIES}")
	endif (NOT PYTHON_LIBRARIES)
	
endif (WITH_YAF_PY_BINDINGS)

# Ruby bindings TODO: finish Ruby bindings build
if (WITH_YAF_RUBY_BINDINGS)
	find_package(Ruby REQUIRED)

	if (NOT SWIG_FOUND OR NOT RUBY_FOUND)
		set(WITH_YAF_RUBY_BINDINGS OFF)
	endif (NOT SWIG_FOUND OR NOT RUBY_FOUND)
endif (WITH_YAF_RUBY_BINDINGS)

if(DEBUG_BUILD)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build mode" FORCE)
else(DEBUG_BUILD)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build mode" FORCE)
endif(DEBUG_BUILD)

set(YAF_INCLUDE_DIRS ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR})


if(NOT WIN32)
	# installation directories

	if(NOT YAF_BIN_DIR)
		set(YAF_BIN_DIR bin)
	endif(NOT YAF_BIN_DIR)

	if(NOT YAF_LIB_DIR)
		set(YAF_LIB_DIR lib)
	endif(NOT YAF_LIB_DIR)

	if(NOT YAF_BINDINGS_PY_DIR)
		set(YAF_BINDINGS_PY_DIR ${CMAKE_SOURCE_DIR}/bindings/python)
	endif(NOT YAF_BINDINGS_PY_DIR)

	# build flags
	set(CMAKE_CXX_FLAGS "-Wall" CACHE STRING "Default compiler flags" FORCE)
	set(YAF_CXX_FLAGS_RELEASE "-O3 -ffast-math")
	set(YAF_CXX_FLAGS_DEBUG "-ggdb")
	set(YAF_TARGET_TYPE LIBRARY)
else(NOT WIN32)
	# installation directories

	if(NOT YAF_BIN_DIR)
		set(YAF_BIN_DIR Win32Pak)
	endif(NOT YAF_BIN_DIR)

	set(YAF_LIB_DIR ${YAF_BIN_DIR})

	if(NOT YAF_BINDINGS_PY_DIR)
		set(YAF_BINDINGS_PY_DIR ${YAF_BIN_DIR}/python)
	endif(NOT YAF_BINDINGS_PY_DIR)

	# build flags
	if(MINGW)
		set(CMAKE_CXX_FLAGS "-Wall" CACHE STRING "Default compiler flags" FORCE)
		set(YAF_CXX_FLAGS_RELEASE "-O3 -ffast-math")
		set(YAF_CXX_FLAGS_DEBUG "-ggdb")
	else(MINGW)
		set(CMAKE_CXX_FLAGS "/DWIN32 /D_WIN32 /D_USE_MATH_DEFINES /EHsc /MD /nologo" CACHE STRING "Default compiler flags" FORCE)
		set(YAF_CXX_FLAGS_RELEASE "/Ot /Ox")
		set(YAF_CXX_FLAGS_DEBUG "/Zi /GS /RTC1")
	endif(MINGW)
	set(YAF_TARGET_TYPE ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/libs RUNTIME)
endif(NOT WIN32)

# definitions
if(CMAKE_HAVE_PTHREAD_H)
	add_definitions(-DHAVE_PTHREAD)
endif(CMAKE_HAVE_PTHREAD_H)

if(JPEG_FOUND)
	add_definitions(-DHAVE_JPEG)
endif(JPEG_FOUND)

if(PNG_FOUND)
	add_definitions(-DHAVE_PNG)
endif(PNG_FOUND)

if(OPENEXR_FOUND)
	add_definitions(-DHAVE_EXR)
endif(OPENEXR_FOUND)

if(LIBXML2_FOUND)
	add_definitions(-DHAVE_XML)
endif(LIBXML2_FOUND)

if(FREETYPE_FOUND)
	add_definitions(-DHAVE_FREETYPE)
endif(FREETYPE_FOUND)

if(YAF_USER_EXTRA_DEFS)
	add_definitions(${YAF_USER_EXTRA_DEFS})
endif(YAF_USER_EXTRA_DEFS)

if (BUILDRELEASE)
	add_definitions(-DRELEASE)
endif (BUILDRELEASE)

# Paths and flags configuration

include(CMakeConfig/UserConfig.txt OPTIONAL)

if(NOT WIN32)
	set(YAF_PLUGIN_DIR ${YAF_LIB_DIR}/yafaray)
else(NOT WIN32)
	set(YAF_PLUGIN_DIR ${YAF_LIB_DIR}/plugins)
endif(NOT WIN32)

if (USER_INSTALL_PREFFIX)
	set(CMAKE_INSTALL_PREFIX ${USER_INSTALL_PREFFIX})
endif (USER_INSTALL_PREFFIX)

if (USER_RELFLAGS)
	set(CMAKE_CXX_FLAGS_RELEASE "${YAF_CXX_FLAGS_RELEASE} ${USER_RELFLAGS}" CACHE STRING "Release compiler flags" FORCE)
else (USER_RELFLAGS)
	set(CMAKE_CXX_FLAGS_RELEASE "${YAF_CXX_FLAGS_RELEASE}" CACHE STRING "Release compiler flags" FORCE)
endif (USER_RELFLAGS)

if (USER_DBGFLAGS)
	set(CMAKE_CXX_FLAGS_DEBUG "${YAF_CXX_FLAGS_DEBUG} ${USER_DEBUG}" CACHE STRING "Debug compiler flags" FORCE)
else (USER_DBGFLAGS)
	set(CMAKE_CXX_FLAGS_DEBUG "${YAF_CXX_FLAGS_DEBUG}" CACHE STRING "Debug compiler flags" FORCE)
endif (USER_DBGFLAGS)

# Adding subdirectories

add_subdirectory(src)

if (WITH_YAF_PY_BINDINGS)
	include(${SWIG_USE_FILE})
	add_subdirectory(bindings)
endif (WITH_YAF_PY_BINDINGS)

# Generating config headers

include(CMakeConfig/GenConfHeaders.txt)

