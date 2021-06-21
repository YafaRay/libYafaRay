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

add_library(libyafaray4)
set_target_properties(libyafaray4 PROPERTIES PREFIX "" VERSION 4.0.0 SOVERSION 4)
set_target_properties(libyafaray4 PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
set_target_properties(libyafaray4 PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
target_include_directories(libyafaray4 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
target_include_directories(libyafaray4 INTERFACE $<INSTALL_INTERFACE:include>)

find_package(Threads REQUIRED)
target_link_libraries(libyafaray4 PRIVATE Threads::Threads)

add_subdirectory(accelerator)
add_subdirectory(background)
add_subdirectory(color)
add_subdirectory(common)
add_subdirectory(camera)
add_subdirectory(geometry)
add_subdirectory(image)
add_subdirectory(format)
add_subdirectory(integrator)
add_subdirectory(interface)
add_subdirectory(light)
add_subdirectory(material)
add_subdirectory(math)
add_subdirectory(output)
add_subdirectory(photon)
add_subdirectory(public_api)
add_subdirectory(render)
add_subdirectory(sampler)
add_subdirectory(scene)
add_subdirectory(shader)
add_subdirectory(texture)
add_subdirectory(volume)

message_boolean("Using mathematical" YAFARAY_FAST_MATH "approximations (faster)" "precise values (slower)")
if(YAFARAY_FAST_MATH)
	list(APPEND YAF_COMMON_DEFINITIONS "FAST_MATH")
endif()

message_boolean("Using trigonometric" YAFARAY_FAST_TRIG "approximations (faster)" "precise values (slower)")
if(YAFARAY_FAST_TRIG)
	list(APPEND YAF_COMMON_DEFINITIONS "FAST_TRIG")
endif()

message_boolean("Using FreeType" YAFARAY_WITH_Freetype "yes" "no")
if(YAFARAY_WITH_Freetype)
	find_package(Freetype REQUIRED)
	list(APPEND YAF_COMMON_DEFINITIONS "HAVE_FREETYPE")
	target_link_libraries(libyafaray4 PRIVATE Freetype::Freetype)
endif()

message_boolean("Using OpenCV" YAFARAY_WITH_OpenCV "yes" "no")
if(YAFARAY_WITH_OpenCV)
	FIND_PACKAGE(OpenCV COMPONENTS core imgproc photo REQUIRED)
	list(APPEND YAF_COMMON_DEFINITIONS "HAVE_OPENCV")
	target_link_libraries(libyafaray4 PRIVATE opencv_core opencv_imgproc opencv_photo)
endif()

# Custom definitions
target_compile_definitions(libyafaray4
	PRIVATE
		${YAF_COMMON_DEFINITIONS}
		"YAFARAY_BUILD_TYPE=\"$<UPPER_CASE:$<CONFIG>>\""
		"YAFARAY_BUILD_FLAGS=\"${CMAKE_CXX_FLAGS} $<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}>$<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}>$<$<CONFIG:RelWithDebInfo>:${CMAKE_CXX_FLAGS_RELWITHDEBINFO}>$<$<CONFIG:MinSizeRel>:${CMAKE_CXX_FLAGS_MINSIZEREL}>\"")

# Custom linker options
if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	target_link_options(libyafaray4 PRIVATE
			"LINKER:SHELL:--no-undefined"
			"LINKER:SHELL:--version-script=${PROJECT_SOURCE_DIR}/include/public_api/yafaray_c_api_symbols.map" # Custom symbols export and versioning map
			"$<$<CONFIG:RELEASE>:LINKER:SHELL:-s>" # Strip in release builds
			)
endif()

# Copy C API include files to binary directory for other targets to use
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include)
configure_file(${PROJECT_SOURCE_DIR}/include/public_api/yafaray_c_api.h ${PROJECT_BINARY_DIR}/include/)

# Shared target used for generating export header # with the name mytools_export.h, which will be suitable # for both the shared and static targets
include(GenerateExportHeader)
generate_export_header(libyafaray4 BASE_NAME yafaray_c_api EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/yafaray_c_api_export.h)

install(TARGETS libyafaray4
		EXPORT LibYafaRay
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		)

install(FILES
		${PROJECT_SOURCE_DIR}/include/public_api/yafaray_c_api.h
		${PROJECT_BINARY_DIR}/include/yafaray_c_api_export.h
		${PROJECT_BINARY_DIR}/include/yafaray_build_info.h
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		)

install(EXPORT LibYafaRay
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibYafaRay
		NAMESPACE LibYafaRay::
		FILE LibYafaRayConfig.cmake
		)
