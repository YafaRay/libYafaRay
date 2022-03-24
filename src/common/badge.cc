/****************************************************************************
 *
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

#include "common/yafaray_common.h"
#include "common/version_build_info.h"
#include "common/badge.h"
#include "common/param.h"
#include "color/color.h"
#include "common/logger.h"
#include "common/timer.h"
#include "resource/yafLogoTiny.h"
#include "format/format.h"
#include "math/interpolation.h"
#include "common/string.h"

#if HAVE_FREETYPE
	#include "resource/guifont.h"
	#include <ft2build.h>
	#include FT_FREETYPE_H
#endif

BEGIN_YAFARAY

void Badge::setParams(const ParamMap &params)
{
	std::string position_str;
	params.getParam("badge_position", position_str);
	params.getParam("badge_draw_render_settings", draw_render_settings_);
	params.getParam("badge_draw_aa_noise_settings", draw_aa_);
	params.getParam("badge_author", author_);
	params.getParam("badge_title", title_);
	params.getParam("badge_contact", contact_);
	params.getParam("badge_comment", comments_);
	params.getParam("badge_icon_path", icon_path_);
	params.getParam("badge_font_path", font_path_);
	params.getParam("badge_font_size_factor", font_size_factor_);
	setPosition(position_str);
}

void Badge::setPosition(const std::string &position)
{
	if(position == "top") position_ = Badge::Position::Top;
	else if(position == "bottom") position_ = Badge::Position::Bottom;
	else position_ = Badge::Position::None;
}

std::string Badge::getFields() const
{
	std::stringstream ss_badge;
	if(!getTitle().empty()) ss_badge << getTitle() << "\n";
	if(!getAuthor().empty() && !getContact().empty()) ss_badge << getAuthor() << " | " << getContact() << "\n";
	else if(!getAuthor().empty() && getContact().empty()) ss_badge << getAuthor() << "\n";
	else if(getAuthor().empty() && !getContact().empty()) ss_badge << getContact() << "\n";
	if(!getComments().empty()) ss_badge << getComments() << "\n";
	return ss_badge.str();
}

std::string Badge::getRenderInfo(const RenderControl &render_control, const Timer &timer) const
{
	std::stringstream ss_badge;
	ss_badge << "\nYafaRay (" << buildinfo::getVersionString() << buildinfo::getBuildTypeSuffix() << ")" << " " << buildinfo::getBuildOs() << " " << buildinfo::getBuildArchitectureBits() << "bit (" << buildinfo::getBuildCompiler() << ")";
	ss_badge << std::setprecision(2);
	double times = timer.getTimeNotStopping("rendert");
	if(render_control.finished()) times = timer.getTime("rendert");
	int timem, timeh;
	timer.splitTime(times, &times, &timem, &timeh);
	ss_badge << " | " << image_width_ << "x" << image_height_;
	if(render_control.inProgress()) ss_badge << " | " << (render_control.resumed() ? "film loaded + " : "") << "in progress " << std::fixed << std::setprecision(1) << render_control.currentPassPercent() << "% of pass: " << render_control.currentPass() << " / " << render_control.totalPasses();
	else if(render_control.canceled()) ss_badge << " | " << (render_control.resumed() ? "film loaded + " : "") << "stopped at " << std::fixed << std::setprecision(1) << render_control.currentPassPercent() << "% of pass: " << render_control.currentPass() << " / " << render_control.totalPasses();
	else
	{
		if(render_control.resumed()) ss_badge << " | film loaded + " << render_control.totalPasses() - 1 << " passes";
		else ss_badge << " | " << render_control.totalPasses() << " passes";
	}
	//if(cx0 != 0) ssBadge << ", xstart=" << cx0;
	//if(cy0 != 0) ssBadge << ", ystart=" << cy0;
	ss_badge << " | Render time:";
	if(timeh > 0) ss_badge << " " << timeh << "h";
	if(timem > 0) ss_badge << " " << timem << "m";
	ss_badge << " " << times << "s";

	times = timer.getTimeNotStopping("rendert") + timer.getTime("prepass");
	if(render_control.finished()) times = timer.getTime("rendert") + timer.getTime("prepass");
	timer.splitTime(times, &times, &timem, &timeh);
	ss_badge << " | Total time:";
	if(timeh > 0) ss_badge << " " << timeh << "h";
	if(timem > 0) ss_badge << " " << timem << "m";
	ss_badge << " " << times << "s";
	return ss_badge.str();
}

std::string Badge::print(const std::string &denoise_params, const RenderControl &render_control, const Timer &timer) const
{
	std::stringstream ss_badge;
	ss_badge << getFields() << "\n";
	//ss_badge << getRenderInfo(render_control) << " | " << render_control.getRenderInfo() << "\n";
	//ss_badge << render_control.getAaNoiseInfo() << " " << denoise_params;
	ss_badge << denoise_params;
	return ss_badge.str();
}

#if HAVE_FREETYPE

void Badge::drawFontBitmap(FT_Bitmap_ *bitmap, Image *badge_image, int x, int y)
{
	int i, j, p, q;
	const int width = badge_image->getWidth();
	const int height = badge_image->getHeight();
	const int x_max = std::min(x + static_cast<int>(bitmap->width), width);
	const int y_max = std::min(y + static_cast<int>(bitmap->rows), height);
	Rgb text_color(1.f);

	for(i = x, p = 0; i < x_max; i++, p++)
	{
		for(j = y, q = 0; j < y_max; j++, q++)
		{
			if(i >= width || j >= height) continue;
			const int tmp_buf = bitmap->buffer[q * bitmap->width + p];
			if(tmp_buf > 0)
			{
				Rgba col = badge_image->getColor(std::max(0, i), std::max(0, j));
				const float alpha = static_cast<float>(tmp_buf) / 255.f;
				col = Rgba(math::lerp(static_cast<Rgb>(col), text_color, alpha), col.getA());
				badge_image->setColor(std::max(0, i), std::max(0, j), col);
			}
		}
	}
}

#endif

Image * Badge::generateImage(const std::string &denoise_params, const RenderControl &render_control, const Timer &timer) const
{
	if(position_ == Badge::Position::None) return nullptr;
	std::stringstream ss_badge;
	ss_badge << getFields();
	ss_badge << getRenderInfo(render_control, timer);
	if(drawRenderSettings()) ss_badge << " | " << render_control.getRenderInfo();
	if(drawAaNoiseSettings()) ss_badge << "\n" << render_control.getAaNoiseInfo();
	if(!denoise_params.empty()) ss_badge << " | " << denoise_params;

	int badge_line_count = 0;
	constexpr float line_height = 13.f; //Pixels-measured baseline line height for automatic badge height calculation
	constexpr float additional_blank_lines = 1;
	std::string line;
	while(std::getline(ss_badge, line)) ++badge_line_count;
	const int badge_height = (badge_line_count + additional_blank_lines) * std::ceil(line_height * font_size_factor_);
	auto badge_image= Image::factory(logger_, image_width_, badge_height, Image::Type::Color, Image::Optimization::None);

#ifdef HAVE_FREETYPE
	FT_Library library;
	FT_Face face;

	FT_GlyphSlot slot;
	FT_Vector pen; // untransformed origin

	const std::string text_utf_8 = ss_badge.str();
	const std::u32string wtext_utf_32 = string::utf8ToWutf32(text_utf_8);

	// set font size at default dpi
	float fontsize = 12.5f * getFontSizeFactor();

	// initialize library
	if(FT_Init_FreeType(&library))
	{
		logger_.logError("Badge: FreeType lib couldn't be initialized!");
		return nullptr;
	}

	// create face object
	const std::string font_path = getFontPath();
	if(font_path.empty())
	{
		if(FT_New_Memory_Face(library, (const FT_Byte *)font::gui.data(), font::gui.size(), 0, &face))
		{
			logger_.logError("Badge: FreeType couldn't load the default font!");
			return nullptr;
		}
	}
	else if(FT_New_Face(library, font_path.c_str(), 0, &face))
	{
		logger_.logWarning("Badge: FreeType couldn't load the font '", font_path, "', loading default font.");

		if(FT_New_Memory_Face(library, (const FT_Byte *)font::gui.data(), font::gui.size(), 0, &face))
		{
			logger_.logError("Badge: FreeType couldn't load the default font!");
			return nullptr;
		}
	}

	FT_Select_Charmap(face, ft_encoding_unicode);

	// set character size
	if(FT_Set_Char_Size(face, (FT_F26Dot6)(fontsize * 64.0), 0, 0, 0))
	{
		logger_.logError("Badge: FreeType couldn't set the character size!");
		return nullptr;
	}

	slot = face->glyph;

	// offsets
	const int text_offset_x = 4;
	const int text_offset_y = -1 * (int) ceil(12 * getFontSizeFactor());
	const int text_interline_offset = (int) ceil(13 * getFontSizeFactor());

	// The pen position in 26.6 cartesian space coordinates
	pen.x = text_offset_x * 64;
	pen.y = text_offset_y * 64;

	// Draw the text
	for(char32_t ch : wtext_utf_32)
	{
		// Set Coordinates for the carrige return
		if(ch == '\n')
		{
			pen.x = text_offset_x * 64;
			pen.y -= text_interline_offset * 64;
			fontsize = 9.5f * getFontSizeFactor();
			if(FT_Set_Char_Size(face, (FT_F26Dot6)(fontsize * 64.0), 0, 0, 0))
			{
				logger_.logError("Badge: FreeType couldn't set the character size!");
				return nullptr;
			}

			continue;
		}

		// Set transformation
		FT_Set_Transform(face, 0, &pen);

		// Load glyph image into the slot (erase previous one)
		if(FT_Load_Char(face, ch, FT_LOAD_DEFAULT))
		{
			logger_.logError("Badge: FreeType Couldn't load the glyph image for: '", ch, "'!");
			continue;
		}

		// Render the glyph into the slot
		FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

		// Now, draw to our target surface (convert position)
		drawFontBitmap(&slot->bitmap, badge_image, slot->bitmap_left, -slot->bitmap_top);

		// increment pen position
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}

	// Cleanup
	FT_Done_Face(face);
	FT_Done_FreeType(library);
#endif

	std::unique_ptr<Image> logo;

	// Draw logo image
	if(!getIconPath().empty())
	{
		std::string icon_extension = getIconPath().substr(getIconPath().find_last_of(".") + 1);
		std::transform(icon_extension.begin(), icon_extension.end(), icon_extension.begin(), ::tolower);

		std::string imagehandler_type = "png";
		if(icon_extension == "jpeg") imagehandler_type = "jpg";
		else imagehandler_type = icon_extension;

		ParamMap logo_image_params;
		logo_image_params["type"] = imagehandler_type;
		std::unique_ptr<Format> logo_format = std::unique_ptr<Format>(Format::factory(logger_, logo_image_params));
		if(logo_format) logo = std::unique_ptr<Image>(logo_format->loadFromFile(getIconPath(), Image::Optimization::None, ColorSpace::Srgb, 1.f));
		if(!logo_format || !logo) logger_.logWarning("Badge: custom params badge icon '", getIconPath(), "' could not be loaded. Using default YafaRay icon.");
	}

	if(!logo)
	{
		ParamMap logo_image_params;
		logo_image_params["type"] = std::string("png");
		std::unique_ptr<Format> logo_format = std::unique_ptr<Format>(Format::factory(logger_, logo_image_params));
		if(logo_format) logo = std::unique_ptr<Image>(logo_format->loadFromMemory(logo::yafaray_tiny.data(), logo::yafaray_tiny.size(), Image::Optimization::None, ColorSpace::Srgb, 1.f));
	}

	if(logo)
	{
		int logo_width = logo->getWidth();
		int logo_height = logo->getHeight();
		if(logo_width > 80 || logo_height > 45) logger_.logWarning("Badge: custom params badge logo is quite big (", logo_width, " x ", logo_height, "). It could invade other areas in the badge. Please try to keep logo size smaller than 80 x 45, for example.");
		int lx, ly;
		logo_width = std::min(logo_width, image_width_);
		logo_height = std::min(logo_height, badge_height);

		for(lx = 0; lx < logo_width; lx++)
			for(ly = 0; ly < logo_height; ly++)
			{
				if(position_ == Badge::Position::Top) badge_image->setColor(image_width_ - logo_width + lx, ly, logo->getColor(lx, ly));
				else badge_image->setColor(image_width_ - logo_width + lx, badge_height - logo_height + ly, logo->getColor(lx, ly));
			}
	}
	else logger_.logWarning("Badge: default YafaRay params badge icon could not be loaded. No icon will be shown.");

	if(logger_.isVerbose()) logger_.logVerbose("Badge: Rendering parameters badge created.");

	return badge_image;
}

END_YAFARAY