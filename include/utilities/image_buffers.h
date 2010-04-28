/****************************************************************************
 *      image_buffers.h: A
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
 
#ifndef Y_IMAGE_BUFFERS_H
#define Y_IMAGE_BUFFERS_H

#include <yafray_config.h>
#include <core_api/color.h>
#include <vector>

__BEGIN_YAFRAY

/*! Color weighted pixel structure */
class pixel_t
{
	public:
	pixel_t() { col = colorA_t(0.f); weight = 0.f;}
	colorA_t normalized() const
	{
		if(weight > 0.f) return col / weight;
		else return colorA_t(0.f);
	}
	colorA_t col;
	float weight;
};

/*! Gray scale weighted pixel structure */
class pixelGray_t 
{
	public:
	pixelGray_t() { val = 0.f; weight = 0.f;}
	float normalized() const
	{
		if(weight > 0.f) return val / weight;
		else return 0.f;
	}
	float val;
	float weight;
};

template <class T> class generic2DBuffer_t
{
public:
	generic2DBuffer_t() {  }
	
	generic2DBuffer_t(int w, int h) : width(w), height(h)
	{
		data.resize(width);
		for(int i = 0; i < width; i++) data[i].resize(height);
	}
	
	~generic2DBuffer_t()
	{
		if(data.size() > 0)
		{
			for(int i = 0; i < width; i++) data[i].clear();
			data.clear();
		}
	}
	
	inline void clear()
	{
		if(data.size() > 0)
		{
			for(int i = 0; i < width; i++) data[i].clear();
			data.clear();
		}
		data.resize(width);
		for(int i = 0; i < width; i++) data[i].resize(height);
	}

	inline T &operator()(int x, int y)
	{
		return data[x][y];
	}

	inline const T &operator()(int x, int y) const
	{
		return data[x][y];
	}
		
private:
	
	std::vector< std::vector< T > > data;
	int width;
	int height;
};

template <class T> class genericScanlineBuffer_t
{
public:
	genericScanlineBuffer_t() {  }
	
	genericScanlineBuffer_t(int w, int h) : width(w), height(h)
	{
		data.resize(width * height);
	}
	
	~genericScanlineBuffer_t()
	{
		data.clear();
	}
	
	inline void clear()
	{
		data.clear();
		data.resize(width * height);
	}

	inline T &operator()(int x, int y)
	{
		return data[x * height + y];
	}

	inline const T &operator()(int x, int y) const
	{
		return data[x * height + y];
	}
		
private:
	
	std::vector< T > data;
	int width;
	int height;
};

typedef generic2DBuffer_t<pixel_t> 		rgba2DImage_t; //!< Weighted RGBA image buffer typedef
typedef generic2DBuffer_t<pixelGray_t> 	gray2DImage_t; //!< Weighted monochromatic image buffer typedef
typedef generic2DBuffer_t<color_t> 		rgb2DImage_nw_t; //!< Non-weighted RGB image buffer typedef
typedef generic2DBuffer_t<colorA_t> 	rgba2DImage_nw_t; //!< Non-weighted RGBA image buffer typedef
typedef generic2DBuffer_t<float> 		gray2DImage_nw_t; //!< Non-weighted gray scale image buffer typedef

__END_YAFRAY

#endif
