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
		yafaray_c_api_film.cc
		yafaray_c_api_general.cc
		yafaray_c_api_logger.cc
		yafaray_c_api_param_map.cc
		yafaray_c_api_param_map_list.cc
		yafaray_c_api_render_control.cc
		yafaray_c_api_render_monitor.cc
		yafaray_c_api_scene.cc
		yafaray_c_api_surface_integrator.cc
)