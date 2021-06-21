#pragma once
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

#ifndef YAFARAY_OUTPUT_CALLBACK_H
#define YAFARAY_OUTPUT_CALLBACK_H

#include "output/output.h"

BEGIN_YAFARAY

class ImageLayers;

class CallbackOutput final : public ColorOutput
{
	public:
		static UniquePtr_t <ColorOutput> factory(Logger &logger, const ParamMap &params, const Scene &scene, void *callback_user_data, yafaray_OutputPutpixelCallback_t output_putpixel_callback, yafaray_OutputFlushAreaCallback_t output_flush_area_callback, yafaray_OutputFlushCallback_t output_flush_callback);
		CallbackOutput(Logger &logger, int width, int height, void *callback_user_data, yafaray_OutputPutpixelCallback_t output_putpixel_callback, yafaray_OutputFlushAreaCallback_t output_flush_area_callback, yafaray_OutputFlushCallback_t output_flush_callback, const std::string &name = "out", const ColorSpace color_space = ColorSpace::RawManualGamma, float gamma = 1.f, bool with_alpha = true, bool alpha_premultiply = false) : ColorOutput(logger, name, color_space, gamma, with_alpha, alpha_premultiply, callback_user_data, output_putpixel_callback, output_flush_area_callback, output_flush_callback) { width_ = width; height_ = height; }

	private:
		virtual bool putPixel(int x, int y, const ColorLayer &color_layer) override;
		virtual void flush(const RenderControl &render_control) override;
		virtual void flushArea(int x_0, int y_0, int x_1, int y_1) override;
};


END_YAFARAY

#endif // YAFARAY_OUTPUT_CALLBACK_H
