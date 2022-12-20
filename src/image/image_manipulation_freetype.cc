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

#include "image/image_manipulation.h"
#include "image/image_manipulation_freetype.h"
#include "math/interpolation.h"
#include "common/string.h"
#include "common/logger.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace yafaray {

void image_manipulation_freetype::drawFontBitmap(FT_Bitmap_ *bitmap, Image *badge_image, const Point2i &point)
{
	int i, j, p, q;
	const int width = badge_image->getWidth();
	const int height = badge_image->getHeight();
	const int x_max = std::min(point[Axis::X] + static_cast<int>(bitmap->width), width);
	const int y_max = std::min(point[Axis::Y] + static_cast<int>(bitmap->rows), height);
	Rgb text_color(1.f);

	for(i = point[Axis::X], p = 0; i < x_max; i++, p++)
	{
		for(j = point[Axis::Y], q = 0; j < y_max; j++, q++)
		{
			if(i >= width || j >= height) continue;
			const int tmp_buf = bitmap->buffer[q * bitmap->width + p];
			if(tmp_buf > 0)
			{
				Rgba col = badge_image->getColor({{std::max(0, i), std::max(0, j)}});
				const float alpha = static_cast<float>(tmp_buf) / 255.f;
				col = Rgba(math::lerp(static_cast<Rgb>(col), text_color, alpha), col.getA());
				badge_image->setColor({{std::max(0, i), std::max(0, j)}}, col);
			}
		}
	}
}

bool image_manipulation_freetype::drawTextInImage(Logger &logger, Image *image, const std::string &text_utf_8, float font_size_factor, const std::string &font_path)
{
	FT_Library library;
	FT_Face face;

	FT_GlyphSlot slot;
	FT_Vector pen; // untransformed origin

	const std::u32string wtext_utf_32 = string::utf8ToWutf32(text_utf_8);
	float font_size = 12.5f * font_size_factor;

	// initialize library
	if(FT_Init_FreeType(&library))
	{
		logger.logError("FreeType lib couldn't be initialized!");
		return false;
	}

	// create face object
	if(font_path.empty())
	{
		if(FT_New_Memory_Face(library, (const FT_Byte *)font::gui.data(), font::gui.size(), 0, &face))
		{
			logger.logError("FreeType couldn't load the default font!");
			return false;
		}
	}
	else if(FT_New_Face(library, font_path.c_str(), 0, &face))
	{
		logger.logWarning("FreeType couldn't load the font '", font_path, "', loading default font.");

		if(FT_New_Memory_Face(library, (const FT_Byte *)font::gui.data(), font::gui.size(), 0, &face))
		{
			logger.logError("FreeType couldn't load the default font!");
			return false;
		}
	}

	FT_Select_Charmap(face, ft_encoding_unicode);

	// set character size
	if(FT_Set_Char_Size(face, (FT_F26Dot6)(font_size * 64.0), 0, 0, 0))
	{
		logger.logError("FreeType couldn't set the character size!");
		return false;
	}

	slot = face->glyph;

	// offsets
	const int text_offset_x = 4;
	const int text_offset_y = -1 * static_cast<int>(std::ceil(12 * font_size_factor));
	const int text_interline_offset = static_cast<int>(std::ceil(13 * font_size_factor));

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
			font_size = 9.5f * font_size_factor;
			if(FT_Set_Char_Size(face, (FT_F26Dot6)(font_size * 64.0), 0, 0, 0))
			{
				logger.logError("FreeType couldn't set the character size!");
				return false;
			}

			continue;
		}

		// Set transformation
		FT_Set_Transform(face, nullptr, &pen);

		// Load glyph image into the slot (erase previous one)
		if(FT_Load_Char(face, ch, FT_LOAD_DEFAULT))
		{
			logger.logError("Badge: FreeType Couldn't load the glyph image for character code: ", static_cast<int>(ch), "!");
			continue;
		}

		// Render the glyph into the slot
		FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

		// Now, draw to our target surface (convert position)
		drawFontBitmap(&slot->bitmap, image, {{slot->bitmap_left, -slot->bitmap_top}});

		// increment pen position
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}

	// Cleanup
	FT_Done_Face(face);
	FT_Done_FreeType(library);
	return true;
}


} //namespace yafaray