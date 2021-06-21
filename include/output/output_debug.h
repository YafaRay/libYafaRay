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

#ifndef YAFARAY_OUTPUT_DEBUG_H
#define YAFARAY_OUTPUT_DEBUG_H
#include "output/output.h"

BEGIN_YAFARAY

class DebugOutput final : public ColorOutput
{
	public:
		static UniquePtr_t <ColorOutput> factory(Logger &logger, const ParamMap &params, const Scene &scene, void *callback_user_data, yafaray_OutputPutpixelCallback_t output_putpixel_callback, yafaray_OutputFlushAreaCallback_t output_flush_area_callback, yafaray_OutputFlushCallback_t output_flush_callback);
		DebugOutput(Logger &logger, const std::string &name = "out", const ColorSpace color_space = ColorSpace::RawManualGamma, float gamma = 1.f, bool with_alpha = true, bool alpha_premultiply = false, void *callback_user_data = nullptr, yafaray_OutputPutpixelCallback_t output_putpixel_callback = nullptr, yafaray_OutputFlushAreaCallback_t output_flush_area_callback = nullptr, yafaray_OutputFlushCallback_t output_flush_callback = nullptr) : ColorOutput(logger, name, color_space, gamma, with_alpha, alpha_premultiply, callback_user_data, output_putpixel_callback, output_flush_area_callback, output_flush_callback) { }

	private:
		virtual bool putPixel(int x, int y, const ColorLayer &color_layer) override;
		void flush(const RenderControl &render_control) override;
};

END_YAFARAY

#endif //YAFARAY_OUTPUT_DEBUG_H
