project (yafaray)
cmake_minimum_required (VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")
INCLUDE (CMakeForceCompiler)
   SET(CMAKE_SYSTEM_NAME Generic)
   CMAKE_FORCE_C_COMPILER   (gcc GNU)
   CMAKE_FORCE_CXX_COMPILER (g++ GNU)
   
####### Options ########
option(WITH_YF_QT "Enable the YafaRay Qt GUI" ON)
option(WITH_YF_PY_BINDINGS "Enable the YafaRay Python bindings" ON)
option(WITH_YF_RUBY_BINDINGS "Enable the YafaRay Ruby bindings" OFF)
option(BUILDRELEASE "Enable release mode building of YafaRay" OFF)
option(EMBED_FONT_QT "Embed font for QT GUI (usefull for some buggy QT installations)" OFF)
option(DEBUG "Enable debug build mode" OFF)

###### Packages and Definitions #########
find_package(LibXml2 REQUIRED)
find_package(OpenEXR REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(TIFF REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Freetype REQUIRED)
find_package(Threads)

# Python bindings 
if (WITH_YF_PY_BINDINGS)
	find_package(SWIG)
	find_package(PythonLibs REQUIRED)
	find_package(PythonInterp REQUIRED)

	if (NOT SWIG_FOUND OR NOT PYTHON_LIBRARY OR NOT PYTHON_EXECUTABLE)
		set(WITH_YF_PY_BINDINGS OFF)
	else (NOT SWIG_FOUND OR NOT PYTHON_LIBRARY OR NOT PYTHON_EXECUTABLE)
		execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "from distutils import sysconfig; \\
		                        print sysconfig.get_python_lib(1,0,prefix='${CMAKE_INSTALL_PREFIX}')"
		                OUTPUT_VARIABLE PYTHON_SITE_PACKAGE
		                OUTPUT_STRIP_TRAILING_WHITESPACE)

		if (NOT PYTHON_SITE_PACKAGE)
			set(WITH_YF_PY_BINDINGS OFF)
		endif (NOT PYTHON_SITE_PACKAGE)
	endif (NOT SWIG_FOUND OR NOT PYTHON_LIBRARY OR NOT PYTHON_EXECUTABLE)
endif (WITH_YF_PY_BINDINGS)

# Ruby bindings TODO: finish Ruby bindings build
if (WITH_YF_RUBY_BINDINGS)
	find_package(SWIG)
	find_package(Ruby REQUIRED)

	if (NOT SWIG_FOUND OR NOT RUBY_FOUND)
		set(WITH_YF_RUBY_BINDINGS OFF)
	endif (NOT SWIG_FOUND OR NOT RUBY_FOUND)
endif (WITH_YF_RUBY_BINDINGS)

if(DEBUG)
	set(CMAKE_BUILD_TYPE Debug)
else(DEBUG)
	set(CMAKE_BUILD_TYPE Release)
endif(DEBUG)

# definitions
set(YAF_INCLUDE_DIRS ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR})

# installation directories
set(YAF_LIB_DIR lib)
set(YAF_BIN_DIR bin)
set(YAF_BINDINGS_PY_DIR ${CMAKE_SOURCE_DIR}/bindings/python)

if(NOT WIN32)
	set(YAF_CXX_FLAGS_RELEASE "-Wall -O3 -ffast-math -mfpmath=sse")
	set(YAF_CXX_FLAGS_DEBUG "-Wall -ggdb")
else(NOT WIN32)
	set(YAF_CXX_FLAGS_RELEASE "")
	set(YAF_CXX_FLAGS_DEBUG "")
endif(NOT WIN32)

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

include(CMakeUserConfig.txt OPTIONAL)

set(YAF_PLUGIN_DIR ${YAF_LIB_DIR}/yafaray)

if (USER_INSTALL_PREFFIX)
	set(CMAKE_INSTALL_PREFIX ${USER_INSTALL_PREFFIX})
endif (USER_INSTALL_PREFFIX)

if (USER_RELFLAGS)
	set(CMAKE_CXX_FLAGS_RELEASE "${YAF_CXX_FLAGS_RELEASE} ${USER_RELFLAGS}")
endif (USER_RELFLAGS)

if (USER_DBGFLAGS)
	set(CMAKE_CXX_FLAGS_DEBUG "${YAF_CXX_FLAGS_DEBUG} ${USER_DEBUG}")
endif (USER_DBGFLAGS)

if (WITH_YF_QT)
	find_package(Qt4)
	if (!QT4_FOUND)
		set(WITH_YF_QT OFF)
	endif(!QT4_FOUND)
endif(WITH_YF_QT)

if (BUILDRELEASE)
	add_definitions(-DRELEASE)
endif (BUILDRELEASE)

add_subdirectory(src)

if (WITH_YF_PY_BINDINGS)
	include(${SWIG_USE_FILE})
	add_subdirectory(bindings)
endif (WITH_YF_PY_BINDINGS)

include(CMakeConfigGen.txt)

