#****************************************************************************
#      This is part of the libYafaRay package
#
#      This library is free software; you can redistribute it and/or
#      modify it under the terms of the GNU Lesser General Public
#      License as published by the Free Software Foundation; either
#      version 2.1 of the License, or (at your option) any later version.
#
#      This library is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#      Lesser General Public License for more details.
#
#      You should have received a copy of the GNU Lesser General Public
#      License along with this library; if not, write to the Free Software
#      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

cmake_minimum_required(VERSION 3.18)

set(YAFARAY_VERSION_MAJOR 4)
set(YAFARAY_VERSION_MINOR 0)
set(YAFARAY_VERSION_PATCH 0)
set(YAFARAY_VERSION "${YAFARAY_VERSION_MAJOR}.${YAFARAY_VERSION_MINOR}.${YAFARAY_VERSION_PATCH}")
set(YAFARAY_VERSION_PRE_RELEASE "pre-alpha")
set(YAFARAY_VERSION_PRE_RELEASE_DESCRIPTION "PRE-ALPHA UNSTABLE VERSION, WORK IN PROGRESS.")
set(YAFARAY_VERSION_DESCRIPTION "libYafaRay is a free open-source montecarlo raytracing engine released under the LGPL 2.1 license. Raytracing is a rendering technique for generating realistic images by tracing the path of light through a 3D scene. ${YAFARAY_VERSION_PRE_RELEASE_DESCRIPTION}")

project(libYafaRay
		VERSION "${YAFARAY_VERSION}"
		DESCRIPTION "${YAFARAY_VERSION_DESCRIPTION}"
		LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

option(BUILD_SHARED_LIBS "Build project libraries as shared libraries" ON)
option(YAFARAY_BUILD_TESTS "Build test libYafaRay client examples" ON)
option(YAFARAY_FAST_MATH "Enable mathematic approximations to make code faster" ON)
option(YAFARAY_FAST_TRIG "Enable trigonometric approximations to make code faster" ON)
option(YAFARAY_WITH_Freetype "Build with font rendering FreeType support")
option(YAFARAY_WITH_JPEG "Build with JPEG image I/O support")
option(YAFARAY_WITH_OpenCV "Build OpenCV image processing support")
option(YAFARAY_WITH_OpenEXR "Build with OpenEXR image I/O support")
option(YAFARAY_WITH_PNG "Build with PNG image I/O support")
option(YAFARAY_WITH_TIFF "Build with TIFF image I/O support")

get_property(BUILD_TYPE_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(BUILD_TYPE_MULTI_CONFIG)
	if(NOT "Profile" IN_LIST CMAKE_CONFIGURATION_TYPES)
#		list(APPEND CMAKE_CONFIGURATION_TYPES Profile) #This can be used to add additional build types to multi configuration IDEs
	endif()
else()
	set(BUILD_TYPES_ALLOWED Debug Release RelWithDebInfo MinSizeRel)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${BUILD_TYPES_ALLOWED}")
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
	elseif(NOT CMAKE_BUILD_TYPE IN_LIST BUILD_TYPES_ALLOWED)
		message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE option: ${CMAKE_BUILD_TYPE}")
	endif()
endif()

include(message_boolean)
message_boolean("Building libYafaRay test code clients" YAFARAY_BUILD_TESTS "yes" "no")
message_boolean("Building project libraries as" BUILD_SHARED_LIBS "shared" "static")

include(GNUInstallDirs)
if(APPLE)
	set(BASE_RPATH @loader_path)
else()
	set(BASE_RPATH $ORIGIN)
endif()

set(CMAKE_BUILD_WITH_INSTALL_RPATH YES)
file(RELATIVE_PATH RPATH_RELDIR
		${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
		${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
		)
set(CMAKE_INSTALL_RPATH ${BASE_RPATH} ${BASE_RPATH}/${RPATH_RELDIR})

add_subdirectory(src)

if(YAFARAY_BUILD_TESTS)
	add_subdirectory(tests)
endif()

add_subdirectory(cmake)

# Print all available CMake variables (for debugging)
#[[
get_cmake_property(ALL_CMAKE_VARIABLES VARIABLES)
list(SORT ALL_CMAKE_VARIABLES)
foreach(VARIABLE_CMAKE ${ALL_CMAKE_VARIABLES})
	message("${VARIABLE_CMAKE} = '${${VARIABLE_CMAKE}}'")
endforeach()
]]
