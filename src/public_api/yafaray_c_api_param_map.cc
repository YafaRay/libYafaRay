/****************************************************************************
 *   This is part of the libYafaRay package
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "public_api/yafaray_c_api.h"
#include "param/param.h"

yafaray_ParamMap *yafaray_createParamMap()
{
	auto param_map{new yafaray::ParamMap()};
	return reinterpret_cast<yafaray_ParamMap *>(param_map);
}

void yafaray_destroyParamMap(yafaray_ParamMap *param_map)
{
	delete reinterpret_cast<yafaray::ParamMap *>(param_map);
}

void yafaray_setParamMapVector(yafaray_ParamMap *param_map, const char *name, double x, double y, double z)
{
	if(!param_map || !name) return;
	auto &yaf_param_map{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	yaf_param_map[std::string(name)] = yafaray::Vec3f{{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}};
}

void yafaray_setParamMapString(yafaray_ParamMap *param_map, const char *name, const char *s)
{
	if(!param_map || !name || !s) return;
	auto &yaf_param_map{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	yaf_param_map[std::string(name)] = std::string(s);
}

void yafaray_setParamMapBool(yafaray_ParamMap *param_map, const char *name, yafaray_Bool b)
{
	if(!param_map || !name) return;
	auto &yaf_param_map{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	yaf_param_map[std::string(name)] = (b == YAFARAY_BOOL_FALSE) ? false : true;
}

void yafaray_setParamMapInt(yafaray_ParamMap *param_map, const char *name, int i)
{
	if(!param_map || !name) return;
	auto &yaf_param_map{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	yaf_param_map[std::string(name)] = i;
}

void yafaray_setParamMapFloat(yafaray_ParamMap *param_map, const char *name, double f)
{
	if(!param_map || !name) return;
	auto &yaf_param_map{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	yaf_param_map[std::string(name)] = yafaray::Parameter{f};
}

void yafaray_setParamMapColor(yafaray_ParamMap *param_map, const char *name, double r, double g, double b, double a)
{
	if(!param_map || !name) return;
	auto &yaf_param_map{*reinterpret_cast<yafaray::ParamMap *>(param_map)};
	yaf_param_map[std::string(name)] = yafaray::Rgba{static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), static_cast<float>(a)};
}

void paramsSetMatrix(yafaray::ParamMap &param_map, const std::string &name, yafaray::Matrix4f &&matrix, bool transpose) noexcept
{
	param_map[name] = std::move(transpose ? matrix.transpose() : matrix);
}

void yafaray_setParamMapMatrix(yafaray_ParamMap *param_map, const char *name, double m_00, double m_01, double m_02, double m_03, double m_10, double m_11, double m_12, double m_13, double m_20, double m_21, double m_22, double m_23, double m_30, double m_31, double m_32, double m_33, yafaray_Bool transpose)
{
	if(!param_map || !name) return;
	paramsSetMatrix(*reinterpret_cast<yafaray::ParamMap *>(param_map), name, yafaray::Matrix4f{std::array<std::array<float, 4>, 4>{{{static_cast<float>(m_00), static_cast<float>(m_01), static_cast<float>(m_02), static_cast<float>(m_03) }, {static_cast<float>(m_10), static_cast<float>(m_11), static_cast<float>(m_12), static_cast<float>(m_13) }, {static_cast<float>(m_20), static_cast<float>(m_21), static_cast<float>(m_22), static_cast<float>(m_23) }, {static_cast<float>(m_30), static_cast<float>(m_31), static_cast<float>(m_32), static_cast<float>(m_33) }}}}, transpose);
}

void yafaray_setParamMapMatrixArray(yafaray_ParamMap *param_map, const char *name, const double *matrix, yafaray_Bool transpose)
{
	if(!param_map || !name || !matrix) return;
	paramsSetMatrix(*reinterpret_cast<yafaray::ParamMap *>(param_map), name, yafaray::Matrix4f{matrix}, transpose);
}

void yafaray_clearParamMap(yafaray_ParamMap *param_map) 	//!< clear the paramMap and paramList
{
	if(!param_map) return;
	reinterpret_cast<yafaray::ParamMap *>(param_map)->clear();
}

void yafaray_setInputColorSpace(yafaray_ParamMap *param_map, const char *color_space_string, float gamma_val)
{
	if(!param_map || !color_space_string) return;
	reinterpret_cast<yafaray::ParamMap *>(param_map)->setInputColorSpace(color_space_string, gamma_val);
}
