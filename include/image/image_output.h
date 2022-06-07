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
		static ImageOutput *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		void setLoggingParams(const ParamMap &params);
		void setBadgeParams(const ParamMap &params);
		void flush(const RenderControl &render_control, const Timer &timer);
		void init(int width, int height, const ImageLayers *exported_image_layers, const std::map<std::string, std::unique_ptr<RenderView>> *render_views);
		void setRenderView(const RenderView *render_view) { current_render_view_ = render_view; }
		std::string getName() const { return name_; }
		std::string printBadge(const RenderControl &render_control, const Timer &timer) const;
		Image * generateBadgeImage(const RenderControl &render_control, const Timer &timer) const;

	private:
		ImageOutput(Logger &logger, std::string image_path, const DenoiseParams &denoise_params, std::string name = "out", ColorSpace color_space = ColorSpace::RawManualGamma, float gamma = 1.f, bool with_alpha = true, bool alpha_premultiply = false, bool multi_layer = false);
		bool denoiseEnabled() const { return denoise_params_.enabled_; }
		void saveImageFile(const std::string &filename, LayerDef::Type layer_type, Format *format, const RenderControl &render_control, const Timer &timer);
		void saveImageFileMultiChannel(const std::string &filename, Format *format, const RenderControl &render_control, const Timer &timer);

		std::string name_ = "out";
		std::string image_path_;
		float gamma_ = 1.f;
		bool with_alpha_ = false;
		bool alpha_premultiply_ = false;
		bool multi_layer_ = true;
		DenoiseParams denoise_params_;
		const ImageLayers *image_layers_ = nullptr;
		bool save_log_txt_ = false; //Enable/disable text log file saving with exported images
		bool save_log_html_ = false; //Enable/disable HTML file saving with exported images
		Badge badge_;
		const RenderView *current_render_view_ = nullptr;
		const std::map<std::string, std::unique_ptr<RenderView>> *render_views_ = nullptr;
		Logger &logger_;
		ColorSpace color_space_ = ColorSpace::RawManualGamma;
};

END_YAFARAY

#endif // YAFARAY_IMAGE_OUTPUT_H
