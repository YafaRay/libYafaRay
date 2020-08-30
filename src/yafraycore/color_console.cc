/****************************************************************************
 *      color_console.cc: A console coloring utility
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
 */

#include <core_api/color_console.h>

#ifdef _WIN32
#include <windows.h>
#endif

BEGIN_YAFRAY

std::ostream &operator << (std::ostream &o, const SetColor &c)
{
#if !defined(_WIN32)
	o << "\033[" << (int)c.intense_;
	if(c.fg_col_ != Default) o << ';' << c.fg_col_;
	if(c.bg_col_ != Default) o << ';' << c.bg_col_;
	return (o << 'm');
#else
	static WORD origAttr = 0;

	if(origAttr == 0)
	{
		CONSOLE_SCREEN_BUFFER_INFO info;
		if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
		{
			origAttr = info.wAttributes;
		}
	}

	auto new_fg_col = (c.fg_col_ != Default) ? (c.fg_col_ | ((WORD)c.intense_ << 3)) : (origAttr & 0x0F);
	auto new_bg_col = (c.bg_col_ != Default) ? c.bg_col_ : (origAttr & 0xF0);

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), new_bg_col | new_fg_col);
#endif
	return o;
}

END_YAFRAY

