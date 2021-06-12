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
		${CMAKE_CURRENT_SOURCE_DIR}/light.cc
		${CMAKE_CURRENT_SOURCE_DIR}/light_area.cc
		${CMAKE_CURRENT_SOURCE_DIR}/light_background.cc
		${CMAKE_CURRENT_SOURCE_DIR}/light_background_portal.cc
		${CMAKE_CURRENT_SOURCE_DIR}/light_directional.cc
		${CMAKE_CURRENT_SOURCE_DIR}/light_ies.cc
		${CMAKE_CURRENT_SOURCE_DIR}/light_meshlight.cc
		${CMAKE_CURRENT_SOURCE_DIR}/light_point.cc
		${CMAKE_CURRENT_SOURCE_DIR}/light_sphere.cc
		${CMAKE_CURRENT_SOURCE_DIR}/light_spot.cc
		${CMAKE_CURRENT_SOURCE_DIR}/light_sun.cc
)