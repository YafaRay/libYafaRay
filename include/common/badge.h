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
 *
 */

#ifndef YAFARAY_BADGE_H
#define YAFARAY_BADGE_H

#include "render/render_control.h"
#include <string>
#include <memory>

namespace yafaray {

class Rgba;
class Image;
class ParamMap;
class Scene;
class Logger;

class Badge
{
	public:
		explicit Badge(Logger &logger) : logger_(logger) { }
		void setParams(const ParamMap &params);
		enum class Position : unsigned char { None, Top, Bottom };
		Position getPosition() const { return position_; }
		std::string getTitle() const { return title_; }
		std::string getAuthor() const { return author_; }
		std::string getContact() const { return contact_; }
		std::string getComments() const { return comments_; }
		std::string getIconPath() const { return icon_path_; }
		std::string getFontPath() const { return font_path_; }
		float getFontSizeFactor() const { return font_size_factor_; }
		std::string getFields() const;
		std::string getRenderInfo(const RenderControl &render_control, const Timer &timer) const;
		bool drawAaNoiseSettings() const { return draw_aa_; }
		bool drawRenderSettings() const { return draw_render_settings_; }

		void setPosition(const std::string &position);
		void setImageWidth(int width) { image_width_ = width; }
		void setImageHeight(int height) { image_height_ = height; }
		void setTitle(const std::string &title) { title_ = title; }
		void setAuthor(const std::string &author) { author_ = author; }
		void setContact(const std::string &contact) { contact_ = contact; }
		void setComments(const std::string &comments) { comments_ = comments; }
		void setDrawRenderSettings(bool draw_render_settings) { draw_render_settings_ = draw_render_settings; }
		void setDrawAaNoiseSettings(bool draw_noise_settings) { draw_aa_ = draw_noise_settings; }
		void setFontPath(const std::string &font_path) { font_path_ = font_path; }
		void setFontSizeFactor(float font_size_factor) { font_size_factor_ = font_size_factor; }
		void setIconPath(const std::string &icon_path) { icon_path_ = icon_path; }

		std::string print(const std::string &denoise_params, const RenderControl &render_control, const Timer &timer) const;
		Image *generateImage(const std::string &denoise_params, const RenderControl &render_control, const Timer &timer) const;

	protected:
		int image_width_ = 0;
		int image_height_ = 0;
		bool draw_aa_ = true;
		bool draw_render_settings_ = true;
		float font_size_factor_ = 1.f;
		std::string title_;
		std::string author_;
		std::string contact_;
		std::string comments_;
		std::string icon_path_;
		std::string font_path_;
		Logger &logger_;
		Position position_ = Position::None;
};

} //namespace yafaray

#endif
