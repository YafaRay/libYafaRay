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

class ImageOutput final : public ColorOutput
{
	public:
		static UniquePtr_t <ColorOutput> factory(Logger &logger, const ParamMap &params, const Scene &scene);
		ImageOutput(Logger &logger, const std::string &image_path, const DenoiseParams denoise_params, const std::string &name, const ColorSpace color_space, float gamma, bool with_alpha, bool alpha_premultiply, bool multi_layer);

	private:
		virtual bool putPixel(int x, int y, const ColorLayer &color_layer) override;
		virtual void flush(const RenderControl &render_control) override;
		virtual bool isImageOutput() const override { return true; }
		virtual std::string printDenoiseParams() const override;
		virtual void init(int width, int height, const Layers *layers, const std::map<std::string, std::unique_ptr<RenderView>> *render_views) override;
		void saveImageFile(const std::string &filename, const Layer::Type &layer_type, Format *format, const RenderControl &render_control);
		void saveImageFileMultiChannel(const std::string &filename, Format *format, const RenderControl &render_control);
		bool denoiseEnabled() const { return denoise_params_.enabled_; }

		std::string image_path_;
		bool multi_layer_ = true;
		DenoiseParams denoise_params_;
		std::unique_ptr<ImageLayers> image_layers_;
};

END_YAFARAY

#endif // YAFARAY_OUTPUT_IMAGE_H

