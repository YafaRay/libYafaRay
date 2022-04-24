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

target_sources(libyafaray4
		PRIVATE
		image.cc
		image_layers.cc
		image_output.cc
		image_manipulation.cc
		)

message_boolean("Using FreeType" YAFARAY_WITH_FREETYPE "yes" "no")
if(YAFARAY_WITH_FREETYPE)
	find_package(Freetype REQUIRED)
	add_library(libyafaray4_freetype STATIC)
	target_sources(libyafaray4_freetype
			PRIVATE
			image_manipulation_freetype.cc
			)
	set_property(TARGET libyafaray4_freetype PROPERTY POSITION_INDEPENDENT_CODE ON)
	set_target_properties(libyafaray4_freetype PROPERTIES PREFIX "")
	set_target_properties(libyafaray4_freetype PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
	set_target_properties(libyafaray4_freetype PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
	target_include_directories(libyafaray4_freetype PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
	target_link_libraries(libyafaray4_freetype PRIVATE Freetype::Freetype)
	target_link_libraries(libyafaray4 PRIVATE libyafaray4_freetype)
	list(APPEND YAF_IMAGE_DEFINITIONS "HAVE_FREETYPE")
endif()

message_boolean("Using OpenCV" YAFARAY_WITH_OPENCV "yes" "no")
if(YAFARAY_WITH_OPENCV)
	FIND_PACKAGE(OpenCV COMPONENTS core imgproc photo REQUIRED)
	add_library(libyafaray4_opencv STATIC)
	target_sources(libyafaray4_opencv
			PRIVATE
			image_manipulation_opencv.cc
			)
	set_property(TARGET libyafaray4_opencv PROPERTY POSITION_INDEPENDENT_CODE ON)
	set_target_properties(libyafaray4_opencv PROPERTIES PREFIX "")
	set_target_properties(libyafaray4_opencv PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
	set_target_properties(libyafaray4_opencv PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
	target_include_directories(libyafaray4_opencv PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include)
	list(APPEND YAF_IMAGE_DEFINITIONS "HAVE_OPENCV")
	target_link_libraries(libyafaray4_opencv PRIVATE opencv_core opencv_imgproc opencv_photo)
	target_link_libraries(libyafaray4 PRIVATE libyafaray4_opencv)
endif()

set_source_files_properties(
		image_manipulation.cc
		TARGET_DIRECTORY libyafaray4
		PROPERTIES COMPILE_DEFINITIONS
		"${YAF_IMAGE_DEFINITIONS}")