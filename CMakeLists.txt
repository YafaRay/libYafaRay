cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0048 NEW)

project(libYafaRay
		VERSION 4.0.0
		DESCRIPTION "(PRE-ALPHA UNSTABLE VERSION) libYafaRay is a free open-source montecarlo raytracing engine released under the LGPL 2.1 license. Raytracing is a rendering technique for generating realistic images by tracing the path of light through a 3D scene"
		LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${libYafaRay_SOURCE_DIR}/cmake/modules")

option(BUILD_SHARED_LIBS "Build project libraries as shared libraries" ON)
option(BUILD_TESTS "Build test libYafaRay client examples" ON)
option(FAST_MATH "Enable mathematic approximations to make code faster" ON)
option(FAST_TRIG "Enable trigonometric approximations to make code faster" ON)
option(WITH_Freetype "Build with font rendering FreeType support")
option(WITH_JPEG "Build with JPEG image I/O support")
option(WITH_OpenCV "Build OpenCV image processing support")
option(WITH_OpenEXR "Build with OpenEXR image I/O support")
option(WITH_PNG "Build with PNG image I/O support")
option(WITH_TIFF "Build with TIFF image I/O support")

function(message_boolean MESSAGE BOOL_VAR TEXT_TRUE TEXT_FALSE)
	if(${BOOL_VAR})
		message("${MESSAGE}: ${TEXT_TRUE}")
	else()
		message("${MESSAGE}: ${TEXT_FALSE}")
	endif()
endfunction()

message_boolean("Building libYafaRay test code clients" BUILD_TESTS "yes" "no")
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

if(BUILD_TESTS)
	add_subdirectory(tests)
endif()

