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

# Inbuilt image formats that do not require any dependency libraries
target_sources(libyafaray4
	PRIVATE
		format.cc
		format_tga.cc
		format_hdr.cc
)

# Dependency libraries for other image formats

if(YAFARAY_WITH_OPENEXR OR YAFARAY_WITH_PNG)
	find_package(ZLIB)
	if(NOT ZLIB_FOUND)
		message(FATAL_ERROR "ZLib not found: if XML Loader, OpenEXR or PNG options are enabled, ZLib is required.")
	endif()
endif()

message_boolean("Using OpenEXR" YAFARAY_WITH_OPENEXR "yes" "no")
if(YAFARAY_WITH_OPENEXR)
	find_package(Imath CONFIG)
	if(NOT TARGET Imath::Imath)
		message("-- Imath not found (not OpenEXR v3.x), looking for IlmBase and OpenEXR v2.4/2.5")
		find_package(IlmBase CONFIG)
		if (NOT TARGET IlmBase::IlmBase)
			message("-- IlmBase not found (not OpenEXR v2.4/2.5), looking for old OpenEXR v2.2")
		endif()
	endif()
	add_library(libyafaray4_exr STATIC)
	set_property(TARGET libyafaray4_exr PROPERTY POSITION_INDEPENDENT_CODE ON)
	set_target_properties(libyafaray4_exr PROPERTIES PREFIX "")
	set_target_properties(libyafaray4_exr PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
	set_target_properties(libyafaray4_exr PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
	target_include_directories(libyafaray4_exr PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
	find_package(OpenEXR CONFIG)
	if(TARGET OpenEXR::OpenEXR)
		target_link_libraries(libyafaray4_exr PRIVATE OpenEXR::OpenEXR)
	elseif(TARGET OpenEXR::IlmImf)
		target_link_libraries(libyafaray4_exr PRIVATE OpenEXR::IlmImf)
	else()
		find_package(OpenEXR REQUIRED)
		target_include_directories(libyafaray4_exr SYSTEM BEFORE PRIVATE ${OPENEXR_INCLUDE_DIRS})
		target_link_libraries(libyafaray4_exr PRIVATE ${OPENEXR_LIBRARIES})
	endif()
	target_sources(libyafaray4_exr
			PRIVATE
			format_exr.cc
			)
	if(WIN32)
		#Make sure to define OPENEXR_DLL when using OpenEXR Windows DLL to avoid link errors.
		#FIXME: This should not be added if linking statically to OpenEXR, but that's unlikely to happen and I'm not sure how to detect that automatically.
		set_source_files_properties(
				format_exr.cc
				PROPERTIES COMPILE_DEFINITIONS
				"OPENEXR_DLL")
	endif()
	list(APPEND YAF_FORMAT_DEFINITIONS "HAVE_OPENEXR")
	target_link_libraries(libyafaray4 PRIVATE libyafaray4_exr)
endif()

message_boolean("Using JPEG" YAFARAY_WITH_JPEG "yes" "no")
if(YAFARAY_WITH_JPEG)
	find_package(JPEG REQUIRED)
	add_library(libyafaray4_jpg STATIC)
	set_property(TARGET libyafaray4_jpg PROPERTY POSITION_INDEPENDENT_CODE ON)
	set_target_properties(libyafaray4_jpg PROPERTIES PREFIX "")
	set_target_properties(libyafaray4_jpg PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
	set_target_properties(libyafaray4_jpg PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
	target_include_directories(libyafaray4_jpg PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
	target_sources(libyafaray4_jpg
			PRIVATE
			format_jpg.cc
			)
	list(APPEND YAF_FORMAT_DEFINITIONS "HAVE_JPEG")
	target_link_libraries(libyafaray4_jpg PRIVATE JPEG::JPEG)
	target_link_libraries(libyafaray4 PRIVATE libyafaray4_jpg)
endif()

message_boolean("Using PNG" YAFARAY_WITH_PNG "yes" "no")
if(YAFARAY_WITH_PNG)
	find_package(PNG REQUIRED)
	add_library(libyafaray4_png STATIC)
	set_property(TARGET libyafaray4_png PROPERTY POSITION_INDEPENDENT_CODE ON)
	set_target_properties(libyafaray4_png PROPERTIES PREFIX "")
	set_target_properties(libyafaray4_png PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
	set_target_properties(libyafaray4_png PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
	target_include_directories(libyafaray4_png PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
	target_sources(libyafaray4_png
			PRIVATE
			format_png.cc
			)
	list(APPEND YAF_FORMAT_DEFINITIONS "HAVE_PNG")
	target_link_libraries(libyafaray4_png PRIVATE PNG::PNG)
	target_link_libraries(libyafaray4 PRIVATE libyafaray4_png)
endif()

message_boolean("Using TIFF" YAFARAY_WITH_TIFF "yes" "no")
if(YAFARAY_WITH_TIFF)
	find_package(TIFF REQUIRED)
	add_library(libyafaray4_tiff STATIC)
	set_property(TARGET libyafaray4_tiff PROPERTY POSITION_INDEPENDENT_CODE ON)
	set_target_properties(libyafaray4_tiff PROPERTIES PREFIX "")
	set_target_properties(libyafaray4_tiff PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
	set_target_properties(libyafaray4_tiff PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
	target_include_directories(libyafaray4_tiff PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
	target_sources(libyafaray4_tiff
			PRIVATE
			format_tif.cc
			)
	list(APPEND YAF_FORMAT_DEFINITIONS "HAVE_TIFF")
	target_link_libraries(libyafaray4_tiff PRIVATE TIFF::TIFF)
	target_link_libraries(libyafaray4 PRIVATE libyafaray4_tiff)
endif()

set_source_files_properties(
		format.cc
		TARGET_DIRECTORY libyafaray4
		PROPERTIES COMPILE_DEFINITIONS
		"${YAF_FORMAT_DEFINITIONS}")
