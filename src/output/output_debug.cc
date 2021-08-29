/****************************************************************************
 *
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "output/output_debug.h"
#include "color/color_layers.h"
#include "common/param.h"
#include "common/logger.h"
#include "render/render_view.h"
#include "scene/scene.h"

BEGIN_YAFARAY

UniquePtr_t <ColorOutput> DebugOutput::factory(Logger &logger, const ParamMap &params, const Scene &scene)
{
	std::string name;
	std::string color_space_str = "Raw_Manual_Gamma";
	float gamma = 1.f;
	bool with_alpha = false;
	bool alpha_premultiply = false;

	params.getParam("name", name);
	params.getParam("color_space", color_space_str);
	params.getParam("gamma", gamma);
	params.getParam("alpha_channel", with_alpha);
	params.getParam("alpha_premultiply", alpha_premultiply);

	const ColorSpace color_space = Rgb::colorSpaceFromName(color_space_str);
	auto output = UniquePtr_t<ColorOutput>(new DebugOutput(logger, name, color_space, gamma, with_alpha, alpha_premultiply));
	return output;
}

bool DebugOutput::putPixel(int x, int y, const ColorLayer &color_layer)
{
	const std::string layer_type_name = Layer::getTypeName(color_layer.layer_type_);
	const std::string view = current_render_view_ ? current_render_view_->getName() : "";
	if(logger_.isDebug()) logger_.logDebug("DebugOutput::putPixel x=", std::to_string(x), " y=", std::to_string(y), " layer=", layer_type_name, " color=", color_layer.color_, " view = ", view);
	return true;
}

void DebugOutput::flush(const RenderControl &render_control)
{
	const std::string view = current_render_view_ ? current_render_view_->getName() : "";
	if(logger_.isDebug()) logger_.logDebug("DebugOutput::flush", " view = ", view);
}

void DebugOutput::flushArea(int area_id, int x_0, int y_0, int x_1, int y_1)
{
	const std::string view = current_render_view_ ? current_render_view_->getName() : "";
	if(logger_.isDebug()) logger_.logDebug("DebugOutput::flushArea area_id=", std::to_string(area_id), " x_0=", std::to_string(x_0), " y_0=", std::to_string(y_0), " x_1=", std::to_string(x_1), " y_1=", std::to_string(y_1), " view = ", view);
}

END_YAFARAY