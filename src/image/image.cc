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
 */

#include "image/image.h"
#include "render/passes.h"

#ifdef HAVE_OPENCV
#include <opencv2/photo/photo.hpp>
#endif

BEGIN_YAFARAY
Image::Image(int width, int height, const ImageType &type, const Optimization &optimization): width_(width), height_(height), type_(type), optimization_(optimization)
{
	switch(type_)
	{
		case ImageType::ColorAlphaWeight: buffer_ = new Rgba2DImageWeighed_t(width, height); break; //!< Rgba weighted float (160 bit/pixel)
		case ImageType::ColorAlpha:
			switch(optimization_)
			{
				default:
				case Optimization::None: buffer_ = new Rgba2DImage_t(width, height); break; //!< Rgba float (128 bit/pixel)
				case Optimization::Optimized: buffer_ = new RgbaOptimizedImage_t(width, height); break; //!< optimized Rgba (40 bit/pixel)
				case Optimization::Compressed: buffer_ = new RgbaCompressedImage_t(width, height); break; //!< compressed Rgba (24 bit/pixel) LOSSY!
			}
			break;
		case ImageType::Color:
			switch(optimization_)
			{
				default:
				case Optimization::None: buffer_ = new Rgb2DImage_t(width, height); break; //!< Rgb float (128 bit/pixel)
				case Optimization::Optimized: buffer_ = new RgbOptimizedImage_t(width, height); break; //!< optimized Rgb (32 bit/pixel)
				case Optimization::Compressed: buffer_ = new RgbCompressedImage_t(width, height); break; //!< compressed Rgb (16 bit/pixel) LOSSY!
			}
			break;
		case ImageType::GrayWeight: buffer_ = new Gray2DImageWeighed_t(width, height); break; //!< grayscale weighted float (64 bit/pixel)
		case ImageType::GrayAlpha: buffer_ = new GrayAlpha2DImage_t (width, height); break; //!< grayscale with alpha float (64 bit/pixel)
		case ImageType::GrayAlphaWeight: buffer_ = new GrayAlpha2DImageWeighed_t (width, height); break; //!< grayscale weighted with alpha float (96 bit/pixel)
		case ImageType::Gray:
			switch(optimization_)
			{
				default:
				case Optimization::None: buffer_ = new Gray2DImage_t(width, height); break; //!< grayscale float (32 bit/pixel)
				case Optimization::Optimized:
				case Optimization::Compressed: buffer_ = new GrayOptimizedImage_t(width, height); break; //!< optimized grayscale (8 bit/pixel)
			}
			break;
		default:
			break;
	}
}

Image::Image(Image &&image)
{
	width_ = image.width_;
	height_ = image.height_;
	type_ = image.type_;
	optimization_ = image.optimization_;
	buffer_ = image.buffer_;
	image.buffer_ = nullptr;
}


void Image::clear()
{
	if(buffer_)
	{
		switch(type_)
		{
			case ImageType::ColorAlphaWeight: static_cast<Rgba2DImageWeighed_t *>(buffer_)->clear(); break;
			case ImageType::ColorAlpha:
				switch(optimization_)
				{
					default:
					case Optimization::None: static_cast<Rgba2DImage_t *>(buffer_)->clear(); break;
					case Optimization::Optimized: static_cast<RgbaOptimizedImage_t *>(buffer_)->clear(); break;
					case Optimization::Compressed: static_cast<RgbaCompressedImage_t *>(buffer_)->clear(); break;
				}
				break;
			case ImageType::Color:
				switch(optimization_)
				{
					default:
					case Optimization::None: static_cast<Rgb2DImage_t *>(buffer_)->clear(); break;
					case Optimization::Optimized: static_cast<RgbOptimizedImage_t *>(buffer_)->clear(); break;
					case Optimization::Compressed: static_cast<RgbCompressedImage_t *>(buffer_)->clear(); break;
				}
				break;
			case ImageType::GrayWeight: static_cast<Gray2DImageWeighed_t *>(buffer_)->clear(); break;
			case ImageType::GrayAlpha: static_cast<GrayAlpha2DImage_t  *>(buffer_)->clear(); break;
			case ImageType::GrayAlphaWeight: static_cast<GrayAlpha2DImageWeighed_t  *>(buffer_)->clear(); break;
			case ImageType::Gray:
				switch(optimization_)
				{
					default:
					case Optimization::None: static_cast<Gray2DImage_t *>(buffer_)->clear(); break;
					case Optimization::Optimized:
					case Optimization::Compressed: static_cast<GrayOptimizedImage_t *>(buffer_)->clear(); break;
				}
				break;
			default:
				break;
		}
	}
}

Image::~Image()
{
	if(buffer_)
	{
		switch(type_)
		{
			case ImageType::ColorAlphaWeight: delete static_cast<Rgba2DImageWeighed_t *>(buffer_); break;
			case ImageType::ColorAlpha:
				switch(optimization_)
				{
					default:
					case Optimization::None: delete static_cast<Rgba2DImage_t *>(buffer_); break;
					case Optimization::Optimized: delete static_cast<RgbaOptimizedImage_t *>(buffer_); break;
					case Optimization::Compressed: delete static_cast<RgbaCompressedImage_t *>(buffer_); break;
				}
				break;
			case ImageType::Color:
				switch(optimization_)
				{
					default:
					case Optimization::None: delete static_cast<Rgb2DImage_t *>(buffer_); break;
					case Optimization::Optimized: delete static_cast<RgbOptimizedImage_t *>(buffer_); break;
					case Optimization::Compressed: delete static_cast<RgbCompressedImage_t *>(buffer_); break;
				}
				break;
			case ImageType::GrayWeight: delete static_cast<Gray2DImageWeighed_t *>(buffer_); break;
			case ImageType::GrayAlpha: delete static_cast<GrayAlpha2DImage_t  *>(buffer_); break;
			case ImageType::GrayAlphaWeight: delete static_cast<GrayAlpha2DImageWeighed_t  *>(buffer_); break;
			case ImageType::Gray:
				switch(optimization_)
				{
					default:
					case Optimization::None: delete static_cast<Gray2DImage_t *>(buffer_); break;
					case Optimization::Optimized:
					case Optimization::Compressed: delete static_cast<GrayOptimizedImage_t *>(buffer_); break;
				}
				break;
			default:
				break;
		}
	}
}

Rgba Image::getColor(int x, int y) const
{
	if(buffer_)
	{
		switch(type_)
		{
			case ImageType::ColorAlphaWeight: return (*static_cast<Rgba2DImageWeighed_t *>(buffer_))(x, y).getColor();
			case ImageType::ColorAlpha:
				switch(optimization_)
				{
					default:
					case Optimization::None: return (*static_cast<Rgba2DImage_t *>(buffer_))(x, y).getColor();
					case Optimization::Optimized: return (*static_cast<RgbaOptimizedImage_t *>(buffer_))(x, y).getColor();
					case Optimization::Compressed: return (*static_cast<RgbaCompressedImage_t *>(buffer_))(x, y).getColor();
				}
			case ImageType::Color:
				switch(optimization_)
				{
					default:
					case Optimization::None: return (*static_cast<Rgb2DImage_t *>(buffer_))(x, y);
					case Optimization::Optimized: return (*static_cast<RgbOptimizedImage_t *>(buffer_))(x, y).getColor();
					case Optimization::Compressed: return (*static_cast<RgbCompressedImage_t *>(buffer_))(x, y).getColor();
				}
			case ImageType::GrayWeight: return (*static_cast<Gray2DImageWeighed_t *>(buffer_))(x, y).getColor();
			case ImageType::GrayAlpha: return (*static_cast<GrayAlpha2DImage_t *>(buffer_))(x, y).getColor();
			case ImageType::GrayAlphaWeight: return (*static_cast<GrayAlpha2DImageWeighed_t *>(buffer_))(x, y).getColor();
			case ImageType::Gray:
				switch(optimization_)
				{
					default:
					case Optimization::None: return (*static_cast<Gray2DImage_t *>(buffer_))(x, y).getColor();
					case Optimization::Optimized:
					case Optimization::Compressed: return (*static_cast<GrayOptimizedImage_t *>(buffer_))(x, y).getColor();
				}
			default: return Rgba(0.f);
		}
	}
	else return Rgba(0.f);
}

float Image::getFloat(int x, int y) const
{
	if(buffer_)
	{
		switch(type_)
		{
			case ImageType::ColorAlphaWeight: return (*static_cast<Rgba2DImageWeighed_t *>(buffer_))(x, y).getColor().r_;
			case ImageType::ColorAlpha:
				switch(optimization_)
				{
					default:
					case Optimization::None: return (*static_cast<Rgba2DImage_t *>(buffer_))(x, y).getColor().r_;
					case Optimization::Optimized: return (*static_cast<RgbaOptimizedImage_t *>(buffer_))(x, y).getColor().r_;
					case Optimization::Compressed: return (*static_cast<RgbaCompressedImage_t *>(buffer_))(x, y).getColor().r_;
				}
			case ImageType::Color:
				switch(optimization_)
				{
					default:
					case Optimization::None: return (*static_cast<Rgb2DImage_t *>(buffer_))(x, y).r_;
					case Optimization::Optimized: return (*static_cast<RgbOptimizedImage_t *>(buffer_))(x, y).getColor().r_;
					case Optimization::Compressed: return (*static_cast<RgbCompressedImage_t *>(buffer_))(x, y).getColor().r_;
				}
			case ImageType::GrayWeight: return (*static_cast<Gray2DImageWeighed_t *>(buffer_))(x, y).getFloat();
			case ImageType::GrayAlpha: return (*static_cast<GrayAlpha2DImage_t *>(buffer_))(x, y).getFloat();
			case ImageType::GrayAlphaWeight: return (*static_cast<GrayAlpha2DImageWeighed_t *>(buffer_))(x, y).getFloat();
			case ImageType::Gray:
				switch(optimization_)
				{
					default:
					case Optimization::None: return (*static_cast<Gray2DImage_t *>(buffer_))(x, y).getFloat();
					case Optimization::Optimized:
					case Optimization::Compressed: return (*static_cast<GrayOptimizedImage_t *>(buffer_))(x, y).getColor().r_;
				}
			default: return 0.f;
		}
	}
	else return 0.f;
}

float Image::getWeight(int x, int y) const
{
	if(buffer_)
	{
		switch(type_)
		{
			case ImageType::ColorAlphaWeight: return (*static_cast<Rgba2DImageWeighed_t *>(buffer_))(x, y).getWeight();
			case ImageType::GrayWeight: return (*static_cast<Gray2DImageWeighed_t *>(buffer_))(x, y).getWeight();
			case ImageType::GrayAlphaWeight: return (*static_cast<GrayAlpha2DImageWeighed_t *>(buffer_))(x, y).getWeight();
			default: return 0.f;
		}
	}
	else return 0.f;
}

int Image::getInt(int x, int y) const
{
	if(buffer_)
	{
		switch(type_)
		{
			case ImageType::Gray:
				switch(optimization_)
				{
					case Optimization::Optimized:
					case Optimization::Compressed: return (*static_cast<GrayOptimizedImage_t *>(buffer_))(x, y).getInt();
					default: return 0;
				}
			default: return 0;
		}
	}
	else return 0;
}

void Image::setColor(int x, int y, const Rgba &col)
{
	if(buffer_)
	{
		switch(type_)
		{
			case ImageType::ColorAlphaWeight: (*static_cast<Rgba2DImageWeighed_t *>(buffer_))(x, y).setColor(col); break;
			case ImageType::ColorAlpha:
				switch(optimization_)
				{
					default:
					case Optimization::None: (*static_cast<Rgba2DImage_t *>(buffer_))(x, y).setColor(col); break;
					case Optimization::Optimized: (*static_cast<RgbaOptimizedImage_t *>(buffer_))(x, y).setColor(col); break;
					case Optimization::Compressed: (*static_cast<RgbaCompressedImage_t *>(buffer_))(x, y).setColor(col); break;
				}
				break;
			case ImageType::Color:
				switch(optimization_)
				{
					default:
					case Optimization::None: (*static_cast<Rgb2DImage_t *>(buffer_))(x, y) = col; break;
					case Optimization::Optimized: (*static_cast<RgbOptimizedImage_t *>(buffer_))(x, y).setColor(col); break;
					case Optimization::Compressed: (*static_cast<RgbCompressedImage_t *>(buffer_))(x, y).setColor(col); break;
				}
				break;
			case ImageType::GrayWeight: (*static_cast<Gray2DImageWeighed_t *>(buffer_))(x, y).setColor(col); break;
			case ImageType::GrayAlpha: (*static_cast<GrayAlpha2DImage_t  *>(buffer_))(x, y).setColor(col); break;
			case ImageType::GrayAlphaWeight: (*static_cast<GrayAlpha2DImageWeighed_t  *>(buffer_))(x, y).setColor(col); break;
			case ImageType::Gray:
				switch(optimization_)
				{
					default:
					case Optimization::None: (*static_cast<Gray2DImage_t *>(buffer_))(x, y).setColor(col); break;
					case Optimization::Optimized:
					case Optimization::Compressed: (*static_cast<GrayOptimizedImage_t *>(buffer_))(x, y).setColor(col); break;
				}
				break;
			default:
				break;
		}
	}
}

void Image::setFloat(int x, int y, float val)
{
	if(buffer_)
	{
		switch(type_)
		{
			case ImageType::ColorAlphaWeight: (*static_cast<Rgba2DImageWeighed_t *>(buffer_))(x, y).setColor(val); break;
			case ImageType::ColorAlpha:
				switch(optimization_)
				{
					default:
					case Optimization::None: (*static_cast<Rgba2DImage_t *>(buffer_))(x, y).setColor(val); break;
					case Optimization::Optimized: (*static_cast<RgbaOptimizedImage_t *>(buffer_))(x, y).setColor(val); break;
					case Optimization::Compressed: (*static_cast<RgbaCompressedImage_t *>(buffer_))(x, y).setColor(val); break;
				}
				break;
			case ImageType::Color:
				switch(optimization_)
				{
					default:
					case Optimization::None: (*static_cast<Rgb2DImage_t *>(buffer_))(x, y) = val; break;
					case Optimization::Optimized: (*static_cast<RgbOptimizedImage_t *>(buffer_))(x, y).setColor(val); break;
					case Optimization::Compressed: (*static_cast<RgbCompressedImage_t *>(buffer_))(x, y).setColor(val); break;
				}
				break;
			case ImageType::GrayWeight: (*static_cast<Gray2DImageWeighed_t *>(buffer_))(x, y).setFloat(val); break;
			case ImageType::GrayAlpha: (*static_cast<GrayAlpha2DImage_t  *>(buffer_))(x, y).setFloat(val); break;
			case ImageType::GrayAlphaWeight: (*static_cast<GrayAlpha2DImageWeighed_t  *>(buffer_))(x, y).setFloat(val); break;
			case ImageType::Gray:
				switch(optimization_)
				{
					default:
					case Optimization::None: (*static_cast<Gray2DImage_t *>(buffer_))(x, y).setFloat(val); break;
					case Optimization::Optimized:
					case Optimization::Compressed: (*static_cast<GrayOptimizedImage_t *>(buffer_))(x, y).setColor(val); break;
				}
				break;
			default:
				break;
		}
	}
}

void Image::setWeight(int x, int y, float val)
{
	if(buffer_)
	{
		switch(type_)
		{
			case ImageType::ColorAlphaWeight: (*static_cast<Rgba2DImageWeighed_t *>(buffer_))(x, y).setWeight(val); break;
			case ImageType::GrayWeight: (*static_cast<Gray2DImageWeighed_t *>(buffer_))(x, y).setWeight(val); break;
			case ImageType::GrayAlphaWeight: (*static_cast<GrayAlpha2DImageWeighed_t  *>(buffer_))(x, y).setWeight(val); break;
			default:
				break;
		}
	}
}

void Image::setInt(int x, int y, int val)
{
	if(buffer_)
	{
		switch(type_)
		{
			case ImageType::Gray:
				switch(optimization_)
				{
					case Optimization::Optimized:
					case Optimization::Compressed: (*static_cast<GrayOptimizedImage_t *>(buffer_))(x, y).setInt(val); break;
					default: break;
				}
				break;
			default:
				break;
		}
	}
}

Image Image::getDenoisedLdrBuffer(float h_col, float h_lum, float mix) const
{
	Image denoised_buffer = Image(width_, height_, type_, optimization_);

#ifdef HAVE_OPENCV
	cv::Mat a(height_, width_, CV_8UC3);
	cv::Mat b(height_, width_, CV_8UC3);
	cv::Mat_<cv::Vec3b> a_vec = a;
	cv::Mat_<cv::Vec3b> b_vec = b;

	for(int y = 0; y < height_; ++y)
	{
		for(int x = 0; x < width_; ++x)
		{
			Rgb color = getColor(x, y);
			color.clampRgb01();

			a_vec(y, x)[0] = (uint8_t)(color.getR() * 255);
			a_vec(y, x)[1] = (uint8_t)(color.getG() * 255);
			a_vec(y, x)[2] = (uint8_t)(color.getB() * 255);
		}
	}

	cv::fastNlMeansDenoisingColored(a, b, h_lum, h_col, 7, 21);

	for(int y = 0; y < height_; ++y)
	{
		for(int x = 0; x < width_; ++x)
		{
			Rgba col;
			col.r_ = (mix * (float)b_vec(y, x)[0] + (1.f - mix) * (float)a_vec(y, x)[0]) / 255.0;
			col.g_ = (mix * (float)b_vec(y, x)[1] + (1.f - mix) * (float)a_vec(y, x)[1]) / 255.0;
			col.b_ = (mix * (float)b_vec(y, x)[2] + (1.f - mix) * (float)a_vec(y, x)[2]) / 255.0;
			col.a_ = getColor(x, y).a_;
			denoised_buffer.setColor(x, y, col);
		}
	}
#else //HAVE_OPENCV
	//FIXME: Useless duplication work when OpenCV is not built in... avoid calling this function in the first place if OpenCV support not built.
	//This is kept here for interface compatibility when OpenCV not built in.
	for(int y = 0; y < height_; ++y)
	{
		for(int x = 0; x < width_; ++x)
		{
			denoised_buffer.setColor(x, y, getColor(x, y));
		}
	}
	Y_WARNING << "ImageHandler: built without OpenCV support, image cannot be de-noised." << YENDL;
#endif //HAVE_OPENCV
	return denoised_buffer;
}

END_YAFARAY
