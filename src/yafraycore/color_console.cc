/****************************************************************************
 *      color_console.cc: A console coloring utility
 *      This is part of the yafray package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
 *		(pixel_t class wa originally written by someone else, don't know exactly who)
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
 
#include <yafray_config.h>
#ifdef _WIN32
	#ifndef __MINGW32__
		#define NOMINMAX
	#endif
	#include <windows.h>
#endif

__BEGIN_YAFRAY

std::ostream &operator << (std::ostream& o, const setColor& c)
{
#if !defined(_WIN32)
	o << "\033[" << (int)c.intense;
	if(c.fgCol != Default) o << ';' << c.fgCol;
	if(c.bgCol != Default) o << ';' << c.bgCol;
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
	
	yColor newFgCol = (c.fgCol != Default) ?  (c.fgCol | ((WORD)c.intense << 3)) : (origAttr & 0x0F);
	yColor newBgCol = (c.bgCol != Default) ? c.bgCol : (origAttr & 0xF0);
	
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), newBgCol | newFgCol);
#endif
	return o;
}

__END_YAFRAY

