#pragma once
/****************************************************************************
 *
 *      imageOutput.h: generic color output based on imageHandlers
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

#ifndef YAFARAY_OUTPUT_IMAGE_H
#define YAFARAY_OUTPUT_IMAGE_H

#include "output/output.h"
#include "common/layers.h"

BEGIN_YAFARAY

class ImageLayers;
class Format;

class LIBYAFARAY_EXPORT ImageOutput final : public ColorOutput
{
	public:
		static ColorOutput *factory(const ParamMap &params, const Scene &scene);
		ImageOutput(const std::string &image_path, int border_x, int border_y, const DenoiseParams denoise_params, const std::string &name = "out", const ColorSpace color_space = ColorSpace::RawManualGamma, float gamma = 1.f, bool with_alpha = true, bool alpha_premultiply = false, bool multi_layer = true);
		virtual ~ImageOutput() override;

	private:
		virtual bool putPixel(int x, int y, const ColorLayer &color_layer) override;
		virtual void flush(const RenderControl &render_control) override;
		virtual void flushArea(int x_0, int y_0, int x_1, int y_1) override {} // not used by images... yet
		virtual bool isImageOutput() const override { return true; }
		virtual std::string printDenoiseParams() const override;
		virtual void init(int width, int height, const Layers *layers, const std::map<std::string, RenderView *> *render_views) override;
		void saveImageFile(const std::string &filename, const Layer::Type &layer_type, Format *format, const RenderControl &render_control);
		void saveImageFileMultiChannel(const std::string &filename, Format *format, const RenderControl &render_control);
		void clearImageLayers();
		bool denoiseEnabled() const { return denoise_params_.enabled_; }
		DenoiseParams getDenoiseParams() const { return denoise_params_; }

		std::string image_path_;
		float border_x_;
		float border_y_;
		bool multi_layer_ = true;
		DenoiseParams denoise_params_;
		ImageLayers *image_layers_ = nullptr;
};

END_YAFARAY

#endif // YAFARAY_OUTPUT_IMAGE_H

