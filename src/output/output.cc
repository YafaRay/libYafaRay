/****************************************************************************
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#include "output/output.h"
#include "output/output_image.h"
#include "output/output_memory.h"
#include "output/output_debug.h"
#include "color/color_layers.h"
#include "scene/scene.h"
#include "common/logger.h"
#include "common/param.h"

BEGIN_YAFARAY

ColorOutput *ColorOutput::factory(const ParamMap &params, const Scene &scene)
{
	Y_DEBUG PRTEXT(**ColorOutput) PREND; params.printDebug();
	std::string type;
	params.getParam("type", type);
	if(type == "image_output") return ImageOutput::factory(params, scene);
	else if(type == "memory_output") return MemoryInputOutput::factory(params, scene);
	else if(type == "debug_output") return DebugOutput::factory(params, scene);
	else return nullptr;
}

ColorOutput::ColorOutput(const std::string &name, const ColorSpace color_space, float gamma, bool with_alpha, bool alpha_premultiply) : name_(name), color_space_(color_space), gamma_(gamma), with_alpha_(with_alpha), alpha_premultiply_(alpha_premultiply)
{
	if(color_space == RawManualGamma)
	{
		//If the gamma is too close to 1.f, or negative, ignore gamma and do a pure linear RGB processing without gamma.
		if(gamma <= 0 || std::abs(1.f - gamma) <= 0.001)
		{
			color_space_ = LinearRgb;
			gamma_ = 1.f;
		}
	}
}

void ColorOutput::init(int width, int height, const Layers *layers, const std::map<std::string, RenderView *> *render_views)
{
	width_ = width;
	height_ = height;
	render_views_ = render_views;
	layers_ = layers;
	badge_.setImageWidth(width_);
	badge_.setImageHeight(height_);
}

bool ColorOutput::putPixel(int x, int y, const ColorLayers &color_layers)
{
	for(const auto &color_layer : color_layers)
	{
		const ColorLayer preprocessed_color_layer = preProcessColor(color_layer.second);
		putPixel(x, y, preprocessed_color_layer);
	}
	return true;
}

ColorLayer ColorOutput::preProcessColor(const ColorLayer &color_layer)
{
	ColorLayer result = color_layer;
	result.color_.clampRgb0();
	if(Layer::applyColorSpace(result.layer_type_)) result.color_.colorSpaceFromLinearRgb(color_space_, gamma_);
	if(alpha_premultiply_) result.color_.alphaPremultiply();

	//To make sure we don't have any weird Alpha values outside the range [0.f, +1.f]
	if(result.color_.a_ < 0.f) result.color_.a_ = 0.f;
	else if(result.color_.a_ > 1.f) result.color_.a_ = 1.f;

	return result;
}

std::string ColorOutput::printBadge(const RenderControl &render_control) const
{
	return badge_.print(printDenoiseParams(), render_control);
}

std::unique_ptr<const Image> ColorOutput::generateBadgeImage(const RenderControl &render_control) const
{
	return badge_.generateImage(printDenoiseParams(), render_control);
}

void ColorOutput::setLoggingParams(const ParamMap &params)
{
	params.getParam("logging_save_txt", save_log_txt_);
	params.getParam("logging_save_html", save_log_html_);
}

void ColorOutput::setBadgeParams(const ParamMap &params)
{
	badge_.setParams(params);
}

END_YAFARAY