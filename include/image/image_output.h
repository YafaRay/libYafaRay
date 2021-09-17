#pragma once
/****************************************************************************
 *
 *      image_output.h: Image Output class
 *      This is part of the libYafaRay package
 *      Copyright (C) 2002  Alejandro Conty Estévez
 *      Modifyed by Rodrigo Placencia Vazquez (2009)
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
#ifndef YAFARAY_IMAGE_OUTPUT_H
#define YAFARAY_IMAGE_OUTPUT_H

#include "public_api/yafaray_c_api.h"
#include "common/badge.h"
#include "image/image_layers.h"

BEGIN_YAFARAY

class RenderView;
class ColorLayers;
class Format;

class ImageOutput final
{
	public:
		static UniquePtr_t <ImageOutput> factory(Logger &logger, const ParamMap &params, const Scene &scene);
		void setAutoDelete(bool value) { auto_delete_ = value; }
		void setLoggingParams(const ParamMap &params);
		void setBadgeParams(const ParamMap &params);
		bool putPixel(int x, int y, const ColorLayer &color_layer);
		void flush(const RenderControl &render_control);
		void init(int width, int height, const Layers *layers, const std::map<std::string, std::unique_ptr<RenderView>> *render_views);
		bool putPixel(int x, int y, const ColorLayers &color_layers);
		void setRenderView(const RenderView *render_view) { current_render_view_ = render_view; }
		bool isAutoDeleted() const { return auto_delete_; }
		int getWidth() const { return width_; }
		int getHeight() const { return height_; }
		std::string getName() const { return name_; }
		std::string printBadge(const RenderControl &render_control) const;
		std::unique_ptr<Image> generateBadgeImage(const RenderControl &render_control) const;

	private:
		ImageOutput(Logger &logger, const std::string &image_path, const DenoiseParams denoise_params, const std::string &name = "out", const ColorSpace color_space = ColorSpace::RawManualGamma, float gamma = 1.f, bool with_alpha = true, bool alpha_premultiply = false, bool multi_layer = false);
		ColorLayer preProcessColor(const ColorLayer &color_layer);
		std::string printDenoiseParams() const;
		ColorSpace getColorSpace() const { return color_space_; }
		float getGamma() const { return gamma_; }
		bool getAlphaPremultiply() const { return alpha_premultiply_; }
		bool denoiseEnabled() const { return denoise_params_.enabled_; }
		void saveImageFile(const std::string &filename, const Layer::Type &layer_type, Format *format, const RenderControl &render_control);
		void saveImageFileMultiChannel(const std::string &filename, Format *format, const RenderControl &render_control);

		std::string name_ = "out";
		std::string image_path_;
		bool auto_delete_ = true; //!< If true, the output is owned by libYafaRay and it is automatically deleted when removed from scene outputs list or when scene is deleted. Set it to false when the libYafaRay client owns the output.
		int width_ = 0;
		int height_ = 0;
		ColorSpace color_space_ = ColorSpace::RawManualGamma;
		float gamma_ = 1.f;
		bool with_alpha_ = false;
		bool alpha_premultiply_ = false;
		bool multi_layer_ = true;
		DenoiseParams denoise_params_;
		std::unique_ptr<ImageLayers> image_layers_;
		bool save_log_txt_ = false; //Enable/disable text log file saving with exported images
		bool save_log_html_ = false; //Enable/disable HTML file saving with exported images
		Badge badge_;
		const RenderView *current_render_view_ = nullptr;
		const std::map<std::string, std::unique_ptr<RenderView>> *render_views_ = nullptr;
		const Layers *layers_ = nullptr;
		Logger &logger_;
};

END_YAFARAY

#endif // YAFARAY_IMAGE_OUTPUT_H
