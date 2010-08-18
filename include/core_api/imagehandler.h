/****************************************************************************
 *
 * 		imagehandler.h: image load and save abstract class
 *      This is part of the yafray package
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
 
#ifndef Y_IMAGE_HANDLER_H
#define Y_IMAGE_HANDLER_H

#include "yafray_constants.h"
#include <utilities/image_buffers.h>
#include <string>

__BEGIN_YAFRAY

typedef unsigned char yByte;
typedef unsigned short yWord;

class YAFRAYCORE_EXPORT imageHandler_t
{
public:
	virtual void initForOutput(int width, int height, bool withAlpha = false, bool withDepth = false) = 0;
	virtual ~imageHandler_t() {};
	virtual bool loadFromFile(const std::string &name) = 0;
	virtual bool loadFromMemory(const yByte *data, size_t size) {return false; }
	virtual bool saveToFile(const std::string &name) = 0;
	virtual void putPixel(int x, int y, const colorA_t &rgba, float depth = 0.f) = 0;
	virtual colorA_t getPixel(int x, int y) = 0;
	virtual int getWidth() { return m_width; }
	virtual int getHeight() { return m_height; }
	virtual bool isHDR() { return false; }
	
protected:
	std::string handlerName;
	int m_width;
	int m_height;
	bool m_hasAlpha;
	bool m_hasDepth;
	rgba2DImage_nw_t *m_rgba;
	gray2DImage_nw_t *m_depth;
};

__END_YAFRAY

#endif
