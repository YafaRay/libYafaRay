/****************************************************************************
 *      image_buffers.h: An image buffer handler
 *      This is part of the yafray package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
 *      Copyright (C) 2015 David Bluecame for rgba8888, rgba7773, rgb888 and
 * 		rgb565 buffers for texture RAM optimization
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
#include <stdint.h>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>

__BEGIN_YAFRAY

/*! Color weighted pixel structure */
class pixel_t
{
	public:
	pixel_t() { col = colorA_t(0.f); weight = 0.f;}
	colorA_t normalized() const
	{
		if(weight != 0.f) return col / weight;	//Changed from if(weight > 0.f) to if(weight != 0.f) because lanczos and mitchell filters, as they have a negative lobe, sometimes generate pixels with all negative values and also negative weight. Having if(weight > 0.f) caused such pixels to be incorrectly set to 0,0,0,0 and were shown as black dots (with alpha=0). Options are: clipping the filter output to values >=0, but they would lose ability to sharpen the image. Other option (applied here) is to allow negative values and normalize them correctly. This solves a problem stated in http://yafaray.org/node/712 but perhaps could cause other artifacts? We have to keep an eye on this to decide the best option.
		else return colorA_t(0.f);
	}
	
	colorA_t col;
	float weight;
	
	friend class boost::serialization::access;
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(col);
		ar & BOOST_SERIALIZATION_NVP(weight);
	}
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

	friend class boost::serialization::access;
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(val);
		ar & BOOST_SERIALIZATION_NVP(weight);
	}
};

class rgba8888_t
{
	public:
		rgba8888_t():r(0),g(0),b(0),a(1) {}
	
		void setColor(const colorA_t& col) { setR((uint8_t)roundf(col.R*255.f)); setG((uint8_t)roundf(col.G*255.f)); setB((uint8_t)roundf(col.B*255.f)); setA((uint8_t)roundf(col.A*255.f)); }
		
		void setR(uint8_t red8) { r = red8; }
		void setG(uint8_t green8) { g = green8; }
		void setB(uint8_t blue8) { b = blue8; }
		void setA(uint8_t alpha8) { a = alpha8; }
		
		uint8_t getR() const { return r; }
		uint8_t getG() const { return g; }
		uint8_t getB() const { return b; }
		uint8_t getA() const { return a; }

		colorA_t getColor() { return colorA_t((float) getR()/255.f, (float) getG()/255.f, (float) getB()/255.f, (float) getA()/255.f); }
	
	protected:
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
};

class rgba7773_t
{
    //RGBA7773 lossy 24bit format: rrrr rrra gggg ggga bbbb bbba   "invented" by David Bluecame
    //7 bits for each color, 3 bits for alpha channel
    
    public:
		rgba7773_t():ra(0x01),ga(0x01),ba(0x01) {}
		
		void setColor(const colorA_t& col) { setR((uint8_t)roundf(col.R*255.f)); setG((uint8_t)roundf(col.G*255.f)); setB((uint8_t)roundf(col.B*255.f));  setA((uint8_t)roundf(col.A*255.f)); }
		
		void setR(uint8_t red8) { ra = (ra & 0x01) | (red8 & 0xFE); }
		void setG(uint8_t green8) { ga = (ga & 0x01) | (green8 & 0xFE); }
		void setB(uint8_t blue8) { ba = (ba & 0x01) | (blue8 & 0xFE); }
		void setA(uint8_t alpha8)
		{
			ra = (ra & 0xFE) | ((alpha8 & 0x80) >> 7);
			ga = (ga & 0xFE) | ((alpha8 & 0x40) >> 6);
			ba = (ba & 0xFE) | ((alpha8 & 0x20) >> 5);
		}
		
		uint8_t getR() const { return ra & 0xFE; }
		uint8_t getG() const { return ga & 0xFE; }
		uint8_t getB() const { return ba & 0xFE; }
		uint8_t getA() const { return ((ra & 0x01) << 7) | ((ga & 0x01) << 6) | ((ba & 0x01) << 5); }

		colorA_t getColor() { return colorA_t((float) getR()/254.f, (float) getG()/254.f, (float) getB()/254.f, (float) getA()/224.f); } //maximum range is 7bit 0xFE (254) for colors and 3bit 0xE0 (224) for alpha, so I'm scaling acordingly. Loss of color data is happening and scaling may make it worse, but it's the only way of doing this consistently
    
    protected:
		uint8_t ra;		//red + alpha most significant bit
		uint8_t ga;		//green + alpha centre bit
		uint8_t ba;		//blue + alpha least significant bit
};


class rgb888_t
{
	public:
		rgb888_t():r(0),g(0),b(0) {}
	
		void setColor(const colorA_t& col) { setR((uint8_t)roundf(col.R*255.f)); setG((uint8_t)roundf(col.G*255.f)); setB((uint8_t)roundf(col.B*255.f)); }
		
		void setR(uint8_t red8) { r = red8; }
		void setG(uint8_t green8) { g = green8; }
		void setB(uint8_t blue8) { b = blue8; }
		void setA(uint8_t alpha8) {}
		
		uint8_t getR() const { return r; }
		uint8_t getG() const { return g; }
		uint8_t getB() const { return b; }
		uint8_t getA() const { return 255; }

		colorA_t getColor() { return colorA_t((float) getR()/255.f, (float) getG()/255.f, (float) getB()/255.f, 1.f); }
	
	protected:
		uint8_t r;
		uint8_t g;
		uint8_t b;
};

class rgb565_t
{
    //RGB565 lossy 16bit format: rrrr rggg gggb bbbb
    public:
		rgb565_t():rgb565(0) {}
		
		void setColor(const colorA_t& col) { setR((uint8_t)roundf(col.R*255.f)); setG((uint8_t)roundf(col.G*255.f)); setB((uint8_t)roundf(col.B*255.f)); }
		
		void setR(uint8_t red8) { rgb565 = (rgb565 & 0x07FF) | ((red8 & 0xF8) << 8); }
		void setG(uint8_t green8) { rgb565 = (rgb565 & 0xF81F) | ((green8 & 0xFC) << 3); }
		void setB(uint8_t blue8) { rgb565 = (rgb565 & 0xFFE0) | ((blue8 & 0xF8) >> 3); }
		void setA(uint8_t alpha8) {}
		
		uint8_t getR() const { return ((rgb565 & 0xF800) >> 8); }
		uint8_t getG() const { return ((rgb565 & 0x07E0) >> 3); }
		uint8_t getB() const { return ((rgb565 & 0x001F) << 3); }
		uint8_t getA() const { return 255; }

		colorA_t getColor() { return colorA_t((float) getR()/248.f, (float) getG()/252.f, (float) getB()/248.f, 1.f); } //maximum range is 5bit 0xF8 (248) for r,b colors and 6bit 0xFC (252) for g color, so I'm scaling acordingly. Loss of color data is happening and scaling may make it worse, but it's the only way of doing this consistently
    
    protected:
		uint16_t rgb565;
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
		
protected:
	std::vector< std::vector< T > > data;
	int width;
	int height;

	friend class boost::serialization::access;
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(data);
		ar & BOOST_SERIALIZATION_NVP(width);
		ar & BOOST_SERIALIZATION_NVP(height);
	}
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
		
protected:
	std::vector< T > data;
	int width;
	int height;
	
	friend class boost::serialization::access;
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(data);
		ar & BOOST_SERIALIZATION_NVP(width);
		ar & BOOST_SERIALIZATION_NVP(height);
	}
};

typedef generic2DBuffer_t<pixel_t> 		rgba2DImage_t; //!< Weighted RGBA image buffer typedef
typedef generic2DBuffer_t<pixelGray_t> 	gray2DImage_t; //!< Weighted monochromatic image buffer typedef
typedef generic2DBuffer_t<color_t> 		rgb2DImage_nw_t; //!< Non-weighted RGB (96bit/pixel) image buffer typedef
typedef generic2DBuffer_t<colorA_t> 	rgba2DImage_nw_t; //!< Non-weighted RGBA (128bit/pixel) image buffer typedef
typedef generic2DBuffer_t<float> 		gray2DImage_nw_t; //!< Non-weighted gray scale (32bit/gray pixel) image buffer typedef
typedef generic2DBuffer_t<rgb888_t>		rgbOptimizedImage_nw_t; //!< Non-weighted optimized (24bit/pixel) without alpha image buffer typedef
typedef generic2DBuffer_t<rgb565_t>		rgbCompressedImage_nw_t; //!< Non-weighted compressed (16bit/pixel) LOSSY image buffer typedef
typedef generic2DBuffer_t<rgba8888_t>	rgbaOptimizedImage_nw_t; //!< Non-weighted optimized (32bit/pixel) with alpha buffer typedef
typedef generic2DBuffer_t<rgba7773_t>	rgbaCompressedImage_nw_t; //!< Non-weighted compressed (24bit/pixel) LOSSY with alpha buffer typedef

__END_YAFRAY

#endif
