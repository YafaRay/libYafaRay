/****************************************************************************
 *      This is part of the libYafaRay package
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
 */

#include "color/color.h"
#include "output/output_callback.h"
#include "color/color_layers.h"
#include "common/param.h"
#include "scene/scene.h"

BEGIN_YAFARAY

UniquePtr_t <ColorOutput> CallbackOutput::factory(Logger &logger, const ParamMap &params, const Scene &scene)
{
	std::string name;
	int width = 0;
	int height = 0;
	std::string color_space_str;
	float gamma = 1.f;
	bool alpha_premultiply = false;

	params.getParam("name", name);
	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("color_space", color_space_str);
	params.getParam("gamma", gamma);
	params.getParam("alpha_premultiply", alpha_premultiply);

	const ColorSpace color_space = Rgb::colorSpaceFromName(color_space_str);

	return UniquePtr_t<ColorOutput>(new CallbackOutput(logger, width, height, name, color_space, gamma, alpha_premultiply));
}

bool CallbackOutput::putPixel(int x, int y, const ColorLayer &color_layer)
{
	const float &r = color_layer.color_.r_;
	const float &g = color_layer.color_.g_;
	const float &b = color_layer.color_.b_;
	const float &a = color_layer.color_.a_;
	if(put_pixel_callback_) put_pixel_callback_(current_render_view_->getName().c_str(), Layer::getTypeName(color_layer.layer_type_).c_str(), x, y, r, g, b, a, put_pixel_callback_user_data_);
	return true;
}

void CallbackOutput::flush(const RenderControl &render_control)
{
	if(flush_callback_) flush_callback_(current_render_view_->getName().c_str(), flush_callback_user_data_);
}

void CallbackOutput::flushArea(int x_0, int y_0, int x_1, int y_1)
{
	if(flush_area_callback_) flush_area_callback_(current_render_view_->getName().c_str(), x_0, y_0, x_1, y_1, flush_area_callback_user_data_);
}

void CallbackOutput::highlightArea(int area_number, int x_0, int y_0, int x_1, int y_1)
{
	if(highlight_callback_) highlight_callback_(current_render_view_->getName().c_str(), area_number, x_0, y_0, x_1, y_1, highlight_callback_user_data_);
}

END_YAFARAY

