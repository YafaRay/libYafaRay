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
set_target_properties(libyafaray4 PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
set_target_properties(libyafaray4 PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
target_include_directories(libyafaray4 PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
target_include_directories(libyafaray4 INTERFACE $<INSTALL_INTERFACE:include>)

# To check strict compliance
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(libyafaray4 PRIVATE /W4 /WX)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(libyafaray4 PRIVATE -Wall -Wextra -Wpedantic -pedantic -Wno-unused-parameter -Wno-unused-local-typedef -Wno-keyword-macro)
else()
	target_compile_options(libyafaray4 PRIVATE -Wall -Wextra -Wpedantic -pedantic -Wno-unused-parameter -Wno-unused-local-typedefs)
endif()

add_subdirectory(accelerator)
add_subdirectory(background)
add_subdirectory(color)
add_subdirectory(common)
add_subdirectory(camera)
add_subdirectory(geometry)
add_subdirectory(image)
add_subdirectory(format)
add_subdirectory(integrator)
#add_subdirectory(interface)
add_subdirectory(light)
add_subdirectory(material)
add_subdirectory(noise)
add_subdirectory(param)
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
	target_compile_definitions(libyafaray4 PRIVATE "FAST_MATH")
endif()

message_boolean("Using trigonometric" YAFARAY_FAST_TRIG "approximations (faster)" "precise values (slower)")
if(YAFARAY_FAST_TRIG)
	target_compile_definitions(libyafaray4 PRIVATE "FAST_TRIG")
endif()

find_package(Threads REQUIRED)
if(WIN32 AND MINGW AND NOT CMAKE_HAVE_LIBC_PTHREAD)
	message("Using mingw-std-threads 3rd party library: yes")
	find_path(MINGW_STD_THREADS_PATH mingw.thread.h PATH_SUFFIXES mingw-std-threads NO_CACHE)
	if(MINGW_STD_THREADS_PATH)
		option(MINGW_STDTHREADS_GENERATE_STDHEADERS "" ON)
		add_subdirectory(${MINGW_STD_THREADS_PATH} mingw-std-threads-generated)
		target_link_libraries(libyafaray4 PRIVATE mingw_stdthreads)
		target_compile_definitions(libyafaray4 PRIVATE "HAVE_MINGW_STD_THREADS")
	else()
		message(SEND_ERROR "mingw-std-threads library not found.\nWhen using a MinGW compiler with the Win32 threads model instead of the Posix/Pthread model, MinGW does not support the standard thread library such as std::thread, std::mutex, etc.\nIn this case a third party library called 'mingw-std-threads' is needed. It can be obtained from 'https://github.com/meganz/mingw-std-threads' and put in a system include folder.")
	endif()
else()
	target_link_libraries(libyafaray4 PRIVATE Threads::Threads)
endif()

# Custom definitions
target_compile_definitions(libyafaray4
	PRIVATE
		"YAFARAY_BUILD_TYPE=\"$<UPPER_CASE:$<CONFIG>>\""
		"YAFARAY_BUILD_FLAGS=\"${CMAKE_CXX_FLAGS} $<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}>$<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}>$<$<CONFIG:RelWithDebInfo>:${CMAKE_CXX_FLAGS_RELWITHDEBINFO}>$<$<CONFIG:MinSizeRel>:${CMAKE_CXX_FLAGS_MINSIZEREL}>\"")

# Custom linker options
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_link_options(libyafaray4 PRIVATE
				"LINKER:SHELL:--no-undefined"
				"LINKER:SHELL:--version-script=${PROJECT_SOURCE_DIR}/include/public_api/yafaray_c_api_symbols.map" # Custom symbols export and versioning map
				"$<$<CONFIG:RELEASE>:LINKER:SHELL:-s>" # Strip in release builds
				)
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_link_options(libyafaray4 PRIVATE
				"LINKER:SHELL:-undefined,error"
				"LINKER:SHELL:--version-script=${PROJECT_SOURCE_DIR}/include/public_api/yafaray_c_api_symbols.map" # Custom symbols export and versioning map
				"$<$<CONFIG:RELEASE>:LINKER:SHELL:-s>" # Strip in release builds
				)
	endif()
endif()

# Copy C API include files to binary directory for other targets to use
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include)
configure_file(${PROJECT_SOURCE_DIR}/include/public_api/yafaray_c_api.h ${PROJECT_BINARY_DIR}/include/)

# Shared target used for generating export header for both the shared and static targets
include(GenerateExportHeader)
generate_export_header(libyafaray4 BASE_NAME yafaray_c_api EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/yafaray_c_api_export.h)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(${PROJECT_BINARY_DIR}/cmake/LibYafaRayConfigVersion.cmake
		COMPATIBILITY "SameMajorVersion"
		)

install(TARGETS libyafaray4
		EXPORT LibYafaRay
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		)

install(FILES
		${PROJECT_SOURCE_DIR}/include/public_api/yafaray_c_api.h
		${PROJECT_BINARY_DIR}/include/yafaray_c_api_export.h
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		)

install(EXPORT LibYafaRay
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibYafaRay
		NAMESPACE LibYafaRay::
		FILE LibYafaRayConfig.cmake
		)

install(FILES
		${PROJECT_BINARY_DIR}/cmake/LibYafaRayConfigVersion.cmake
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LibYafaRay
		)