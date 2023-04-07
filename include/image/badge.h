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

#ifndef LIBYAFARAY_BADGE_H
#define LIBYAFARAY_BADGE_H

#include "render/render_control.h"
#include "geometry/rect.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"

namespace yafaray {

class Image;

class Badge
{
	public:
		struct Position : public Enum<Position>
		{
			enum : ValueType_t { None, Top, Bottom };
			inline static const EnumMap<ValueType_t> map_{{
					{"none", None, ""},
					{"top", Top, ""},
					{"bottom", Bottom, ""},
				}};
		};
		Badge(Logger &logger, bool draw_aa, bool draw_render_settings, float font_size_factor, Position position, std::string title, std::string author, std::string contact, std::string comments, std::string icon_path, std::string font_path);
		Position getPosition() const { return position_; }
		std::string getTitle() const { return title_; }
		std::string getAuthor() const { return author_; }
		std::string getContact() const { return contact_; }
		std::string getComments() const { return comments_; }
		std::string getIconPath() const { return icon_path_; }
		std::string getFontPath() const { return font_path_; }
		float getFontSizeFactor() const { return font_size_factor_; }
		std::string getFields() const;
		std::string getRenderInfo(const RenderControl &render_control) const;
		bool drawAaNoiseSettings() const { return draw_aa_; }
		bool drawRenderSettings() const { return draw_render_settings_; }
		void setImageSize(const Size2i &size) { image_size_ = size; }
		std::string print(const std::string &denoise_params, const RenderControl &render_control) const;
		std::unique_ptr<Image> generateImage(const std::string &denoise_params, const RenderControl &render_control) const;

	protected:
		Size2i image_size_{0};
		bool draw_aa_ = true;
		bool draw_render_settings_ = true;
		float font_size_factor_ = 1.f;
		Position position_{Position::None};
		std::string title_;
		std::string author_;
		std::string contact_;
		std::string comments_;
		std::string icon_path_;
		std::string font_path_;
		Logger &logger_;
};

} //namespace yafaray

#endif
