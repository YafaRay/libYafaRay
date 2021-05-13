#pragma once
/****************************************************************************
 *
 *      output.h: Output base class
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
#ifndef YAFARAY_OUTPUT_H
#define YAFARAY_OUTPUT_H

#include "yafaray_conf.h"
#include "color/color.h"
#include "common/badge.h"
#include "common/memory.h"
#include <vector>
#include <map>
#include <string>
#include <memory>

BEGIN_YAFARAY

class Image;
class Scene;
class Layers;
struct ColorLayer;
class ColorLayers;
class ParamMap;
class RenderView;
class ColorOutput;

class ColorOutput
{
	public:
		static UniquePtr_t<ColorOutput> factory(const ParamMap &params, const Scene &scene);
		virtual ~ColorOutput() = default;
		void setAutoDelete(bool value) { auto_delete_ = value; }
		void setLoggingParams(const ParamMap &params);
		void setBadgeParams(const ParamMap &params);
		virtual bool putPixel(int x, int y, const ColorLayer &color_layer) = 0;
		virtual void flush(const RenderControl &render_control) = 0;
		virtual void flushArea(int x_0, int y_0, int x_1, int y_1) { }
		virtual void highlightArea(int x_0, int y_0, int x_1, int y_1) { }
		virtual bool isImageOutput() const { return false; }
		virtual bool isPreview() const { return false; }
		virtual void init(int width, int height, const Layers *layers, const std::map<std::string, std::unique_ptr<RenderView>> *render_views);
		bool putPixel(int x, int y, const ColorLayers &color_layers);
		void setRenderView(const RenderView *render_view) { current_render_view_ = render_view; }
		bool isAutoDeleted() const { return auto_delete_; }
		int getWidth() const { return width_; }
		int getHeight() const { return height_; }
		std::string getName() const { return name_; }
		std::string printBadge(const RenderControl &render_control) const;
		std::unique_ptr<Image> generateBadgeImage(const RenderControl &render_control) const;

	protected:
		ColorOutput(const std::string &name = "out", const ColorSpace color_space = ColorSpace::RawManualGamma, float gamma = 1.f, bool with_alpha = true, bool alpha_premultiply = false);
		ColorLayer preProcessColor(const ColorLayer &color_layer);
		virtual std::string printDenoiseParams() const { return ""; }
		ColorSpace getColorSpace() const { return color_space_; }
		float getGamma() const { return gamma_; }
		bool getAlphaPremultiply() const { return alpha_premultiply_; }

		std::string name_ = "out";
		bool auto_delete_ = true; //!< If true, the output is owned by libYafaRay and it is automatically deleted when removed from scene outputs list or when scene is deleted. Set it to false when the libYafaRay client owns the output.
		int width_ = 0;
		int height_ = 0;
		ColorSpace color_space_ = ColorSpace::RawManualGamma;
		float gamma_ = 1.f;
		bool with_alpha_ = false;
		bool alpha_premultiply_ = false;
		bool save_log_txt_ = false; //Enable/disable text log file saving with exported images
		bool save_log_html_ = false; //Enable/disable HTML file saving with exported images
		Badge badge_;
		const RenderView *current_render_view_ = nullptr;
		const std::map<std::string, std::unique_ptr<RenderView>> *render_views_ = nullptr;
		const Layers *layers_ = nullptr;
};

END_YAFARAY

#endif // YAFARAY_OUTPUT_H
