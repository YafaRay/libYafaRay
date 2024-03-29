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

add_executable(yafaray_test00 test00.c)
set_target_properties(yafaray_test00 PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_link_libraries(yafaray_test00 PRIVATE libyafaray4)
target_include_directories(yafaray_test00 PRIVATE ${PROJECT_BINARY_DIR}/include)

# To check strict ANSI C89/C90 API compliance
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(yafaray_test00 PRIVATE /W4 /wd4100) # /WX would turn warnings as errors
else()
	target_compile_options(yafaray_test00 PRIVATE -Wall -Wextra -Wpedantic -pedantic -Wno-unused-parameter)
endif()

install(TARGETS yafaray_test00
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		)
#set_target_properties(yafaray_test00 PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE INSTALL_RPATH "@executable_path/;@executable_path/../../src")
