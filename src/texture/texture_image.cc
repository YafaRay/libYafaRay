/****************************************************************************
 *      imagetex.cc: a texture class for images
 *      This is part of the libYafaRay package
 *      Based on the original by: Mathias Wein; Copyright (C) 2006 Mathias Wein
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez (DarkTide)
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

#include "texture/texture_image.h"
#include "common/session.h"
#include "utility/util_string.h"
#include "common/param.h"
#include "scene/scene.h"

BEGIN_YAFARAY

float *ImageTexture::ewa_weight_lut_ = nullptr;

ImageTexture::ImageTexture(ImageHandler *ih, const InterpolationType &interpolation_type, float gamma, const ColorSpace &color_space):
		image_(ih), color_space_(color_space), gamma_(gamma), mirror_x_(false), mirror_y_(false)
{
	interpolation_type_ = interpolation_type;
}

ImageTexture::~ImageTexture()
{
	// Here we simply clear the pointer, yafaray's core will handle the memory cleanup
	image_ = nullptr;
}

void ImageTexture::resolution(int &x, int &y, int &z) const
{
	x = image_->getWidth();
	y = image_->getHeight();
	z = 0;
}

Rgba ImageTexture::interpolateImage(const Point3 &p, const MipMapParams *mipmap_params) const
{
	if(mipmap_params && mipmap_params->force_image_level_ > 0.f) return mipMapsTrilinearInterpolation(p, mipmap_params);

	Rgba interpolated_color(0.f);

	switch(interpolation_type_)
	{
		case InterpolationType::None: interpolated_color = noInterpolation(p); break;
		case InterpolationType::Bicubic: interpolated_color = bicubicInterpolation(p); break;
		case InterpolationType::Trilinear:
			if(mipmap_params) interpolated_color = mipMapsTrilinearInterpolation(p, mipmap_params);
			else interpolated_color = bilinearInterpolation(p);
			break;
		case InterpolationType::Ewa:
			if(mipmap_params) interpolated_color = mipMapsEwaInterpolation(p, ewa_max_anisotropy_, mipmap_params);
			else interpolated_color = bilinearInterpolation(p);
			break;
		case InterpolationType::Bilinear:
		default: interpolated_color = bilinearInterpolation(p); break;	//By default use Bilinear
	}

	return interpolated_color;
}

Rgba ImageTexture::getColor(const Point3 &p, const MipMapParams *mipmap_params) const
{
	Point3 p_1 = Point3(p.x_, -p.y_, p.z_);
	Rgba ret(0.f);

	bool outside = doMapping(p_1);

	if(outside) return ret;

	ret = interpolateImage(p_1, mipmap_params);

	return applyAdjustments(ret);
}

Rgba ImageTexture::getRawColor(const Point3 &p, const MipMapParams *mipmap_params) const
{
	// As from v3.2.0 all color buffers are already Linear RGB, if any part of the code requires the original "raw" color, a "de-linearization" (encoding again into the original color space) takes place in this function.

	// For example for Non-RGB / Stencil / Bump / Normal maps, etc, textures are typically already linear and the user should select "linearRGB" in the texture properties, but if the user (by mistake) keeps the default sRGB for them, for example, the default linearization would apply a sRGB to linearRGB conversion that messes up the texture values. This function will try to reconstruct the original texture values. In this case (if the user selected incorrectly sRGB for a normal map, for example), this function will prevent wrong results, but it will be slower and it could be slightly inaccurate as the interpolation will take place in the (incorrectly) linearized texels.

	//If the user correctly selected "linearRGB" for these kind of textures, the function below will not make any changes to the color and will keep the texture "as is" without any linearization nor de-linearization, which is the ideal case (fast and correct).

	//The user is responsible to select the correct textures color spaces, if the user does not do it, results may not be the expected. This function is only a coarse "fail safe"

	Rgba ret = getColor(p, mipmap_params);
	ret.colorSpaceFromLinearRgb(color_space_, gamma_);

	return ret;
}

Rgba ImageTexture::getColor(int x, int y, int z, const MipMapParams *mipmap_params) const
{
	int resx = image_->getWidth();
	int resy = image_->getHeight();

	y = resy - y; //on occasion change image storage from bottom to top...

	x = std::max(0, std::min(resx - 1, x));
	y = std::max(0, std::min(resy - 1, y));

	Rgba ret(0.f);

	if(mipmap_params && mipmap_params->force_image_level_ > 0.f) ret = image_->getPixel(x, y, (int) floorf(mipmap_params->force_image_level_ * image_->getHighestImgIndex()));
	else ret = image_->getPixel(x, y);

	return applyAdjustments(ret);
}

Rgba ImageTexture::getRawColor(int x, int y, int z, const MipMapParams *mipmap_params) const
{
	// As from v3.2.0 all color buffers are already Linear RGB, if any part of the code requires the original "raw" color, a "de-linearization" (encoding again into the original color space) takes place in this function.

	// For example for Non-RGB / Stencil / Bump / Normal maps, etc, textures are typically already linear and the user should select "linearRGB" in the texture properties, but if the user (by mistake) keeps the default sRGB for them, for example, the default linearization would apply a sRGB to linearRGB conversion that messes up the texture values. This function will try to reconstruct the original texture values. In this case (if the user selected incorrectly sRGB for a normal map, for example), this function will prevent wrong results, but it will be slower and it could be slightly inaccurate as the interpolation will take place in the (incorrectly) linearized texels.

	//If the user correctly selected "linearRGB" for these kind of textures, the function below will not make any changes to the color and will keep the texture "as is" without any linearization nor de-linearization, which is the ideal case (fast and correct).

	//The user is responsible to select the correct textures color spaces, if the user does not do it, results may not be the expected. This function is only a coarse "fail safe"

	Rgba ret = getColor(x, y, z, mipmap_params);
	ret.colorSpaceFromLinearRgb(color_space_, gamma_);

	return ret;
}

bool ImageTexture::doMapping(Point3 &texpt) const
{
	bool outside = false;

	texpt = 0.5f * texpt + 0.5f;
	// repeat, only valid for REPEAT clipmode
	if(tex_clip_mode_ == TexClipMode::Repeat)
	{

		if(xrepeat_ > 1) texpt.x_ *= (float)xrepeat_;
		if(yrepeat_ > 1) texpt.y_ *= (float)yrepeat_;

		if(mirror_x_ && int(ceilf(texpt.x_)) % 2 == 0) texpt.x_ = -texpt.x_;
		if(mirror_y_ && int(ceilf(texpt.y_)) % 2 == 0) texpt.y_ = -texpt.y_;

		if(texpt.x_ > 1.f) texpt.x_ -= int(texpt.x_);
		else if(texpt.x_ < 0.f) texpt.x_ += 1 - int(texpt.x_);

		if(texpt.y_ > 1.f) texpt.y_ -= int(texpt.y_);
		else if(texpt.y_ < 0.f) texpt.y_ += 1 - int(texpt.y_);
	}

	// crop
	if(cropx_) texpt.x_ = cropminx_ + texpt.x_ * (cropmaxx_ - cropminx_);
	if(cropy_) texpt.y_ = cropminy_ + texpt.y_ * (cropmaxy_ - cropminy_);

	// rot90
	if(rot_90_) std::swap(texpt.x_, texpt.y_);

	// clipping
	switch(tex_clip_mode_)
	{
		case TexClipMode::ClipCube:
		{
			if((texpt.x_ < 0) || (texpt.x_ > 1) || (texpt.y_ < 0) || (texpt.y_ > 1) || (texpt.z_ < -1) || (texpt.z_ > 1))
				outside = true;
			break;
		}
		case TexClipMode::Checker:
		{
			int xs = (int)floor(texpt.x_), ys = (int)floor(texpt.y_);
			texpt.x_ -= xs;
			texpt.y_ -= ys;
			if(!checker_odd_ && !((xs + ys) & 1))
			{
				outside = true;
				break;
			}
			if(!checker_even_ && ((xs + ys) & 1))
			{
				outside = true;
				break;
			}
			// scale around center, (0.5, 0.5)
			if(checker_dist_ < 1.0)
			{
				texpt.x_ = (texpt.x_ - 0.5) / (1.0 - checker_dist_) + 0.5;
				texpt.y_ = (texpt.y_ - 0.5) / (1.0 - checker_dist_) + 0.5;
			}
			// continue to TCL_CLIP
		}
		case TexClipMode::Clip:
		{
			if((texpt.x_ < 0) || (texpt.x_ > 1) || (texpt.y_ < 0) || (texpt.y_ > 1))
				outside = true;
			break;
		}
		case TexClipMode::Extend:
		{
			if(texpt.x_ > 0.99999f) texpt.x_ = 0.99999f; else if(texpt.x_ < 0) texpt.x_ = 0;
			if(texpt.y_ > 0.99999f) texpt.y_ = 0.99999f; else if(texpt.y_ < 0) texpt.y_ = 0;
			// no break, fall thru to TEX_REPEAT
		}
		default:
		case TexClipMode::Repeat: outside = false;
	}
	return outside;
}

void ImageTexture::setCrop(float minx, float miny, float maxx, float maxy)
{
	cropminx_ = minx, cropmaxx_ = maxx, cropminy_ = miny, cropmaxy_ = maxy;
	cropx_ = ((cropminx_ != 0.0) || (cropmaxx_ != 1.0));
	cropy_ = ((cropminy_ != 0.0) || (cropmaxy_ != 1.0));
}

void ImageTexture::findTextureInterpolationCoordinates(int &coord_0, int &coord_1, int &coord_2, int &coord_3, float &coord_decimal_part, float coord_float, int resolution, bool repeat, bool mirror) const
{
	if(repeat)
	{
		coord_1 = ((int)coord_float) % resolution;

		if(mirror)
		{

			if(coord_float < 0.f)
			{
				coord_0 = 1 % resolution;
				coord_2 = coord_1;
				coord_3 = coord_0;
				coord_decimal_part = -coord_float;
			}
			else if(coord_float >= resolution - 1.f)
			{
				coord_0 = (resolution + resolution - 1) % resolution;
				coord_2 = coord_1;
				coord_3 = coord_0;
				coord_decimal_part = coord_float - ((int)coord_float);
			}
			else
			{
				coord_0 = (resolution + coord_1 - 1) % resolution;
				coord_2 = coord_1 + 1;
				if(coord_2 >= resolution) coord_2 = (resolution + resolution - coord_2) % resolution;
				coord_3 = coord_1 + 2;
				if(coord_3 >= resolution) coord_3 = (resolution + resolution - coord_3) % resolution;
				coord_decimal_part = coord_float - ((int)coord_float);
			}
		}
		else
		{
			if(coord_float > 0.f)
			{
				coord_0 = (resolution + coord_1 - 1) % resolution;
				coord_2 = (coord_1 + 1) % resolution;
				coord_3 = (coord_1 + 2) % resolution;
				coord_decimal_part = coord_float - ((int)coord_float);
			}
			else
			{
				coord_0 = 1 % resolution;
				coord_2 = (resolution - 1) % resolution;
				coord_3 = (resolution - 2) % resolution;
				coord_decimal_part = -coord_float;
			}
		}
	}
	else
	{
		coord_1 = std::max(0, std::min(resolution - 1, ((int)coord_float)));

		if(coord_float > 0.f) coord_2 = std::min(resolution - 1, coord_1 + 1);
		else coord_2 = 0;

		coord_0 = std::max(0, coord_1 - 1);
		coord_3 = std::min(resolution - 1, coord_2 + 1);

		coord_decimal_part = coord_float - floor(coord_float);
	}
}

Rgba ImageTexture::noInterpolation(const Point3 &p, int mipmaplevel) const
{
	int resx = image_->getWidth(mipmaplevel);
	int resy = image_->getHeight(mipmaplevel);

	float xf = ((float)resx * (p.x_ - floor(p.x_)));
	float yf = ((float)resy * (p.y_ - floor(p.y_)));

	int x_0, x_1, x_2, x_3, y_0, y_1, y_2, y_3;
	float dx, dy;
	findTextureInterpolationCoordinates(x_0, x_1, x_2, x_3, dx, xf, resx, tex_clip_mode_ == TexClipMode::Repeat, mirror_x_);
	findTextureInterpolationCoordinates(y_0, y_1, y_2, y_3, dy, yf, resy, tex_clip_mode_ == TexClipMode::Repeat, mirror_y_);

	return image_->getPixel(x_1, y_1, mipmaplevel);
}

Rgba ImageTexture::bilinearInterpolation(const Point3 &p, int mipmaplevel) const
{
	int resx = image_->getWidth(mipmaplevel);
	int resy = image_->getHeight(mipmaplevel);

	float xf = ((float)resx * (p.x_ - floor(p.x_))) - 0.5f;
	float yf = ((float)resy * (p.y_ - floor(p.y_))) - 0.5f;

	int x_0, x_1, x_2, x_3, y_0, y_1, y_2, y_3;
	float dx, dy;
	findTextureInterpolationCoordinates(x_0, x_1, x_2, x_3, dx, xf, resx, tex_clip_mode_ == TexClipMode::Repeat, mirror_x_);
	findTextureInterpolationCoordinates(y_0, y_1, y_2, y_3, dy, yf, resy, tex_clip_mode_ == TexClipMode::Repeat, mirror_y_);

	Rgba c_11 = image_->getPixel(x_1, y_1, mipmaplevel);
	Rgba c_21 = image_->getPixel(x_2, y_1, mipmaplevel);
	Rgba c_12 = image_->getPixel(x_1, y_2, mipmaplevel);
	Rgba c_22 = image_->getPixel(x_2, y_2, mipmaplevel);

	float w_11 = (1 - dx) * (1 - dy);
	float w_12 = (1 - dx) * dy;
	float w_21 = dx * (1 - dy);
	float w_22 = dx * dy;

	return (w_11 * c_11) + (w_12 * c_12) + (w_21 * c_21) + (w_22 * c_22);
}

Rgba ImageTexture::bicubicInterpolation(const Point3 &p, int mipmaplevel) const
{
	int resx = image_->getWidth(mipmaplevel);
	int resy = image_->getHeight(mipmaplevel);

	float xf = ((float)resx * (p.x_ - floor(p.x_))) - 0.5f;
	float yf = ((float)resy * (p.y_ - floor(p.y_))) - 0.5f;

	int x_0, x_1, x_2, x_3, y_0, y_1, y_2, y_3;
	float dx, dy;
	findTextureInterpolationCoordinates(x_0, x_1, x_2, x_3, dx, xf, resx, tex_clip_mode_ == TexClipMode::Repeat, mirror_x_);
	findTextureInterpolationCoordinates(y_0, y_1, y_2, y_3, dy, yf, resy, tex_clip_mode_ == TexClipMode::Repeat, mirror_y_);

	Rgba c_00 = image_->getPixel(x_0, y_0, mipmaplevel);
	Rgba c_01 = image_->getPixel(x_0, y_1, mipmaplevel);
	Rgba c_02 = image_->getPixel(x_0, y_2, mipmaplevel);
	Rgba c_03 = image_->getPixel(x_0, y_3, mipmaplevel);

	Rgba c_10 = image_->getPixel(x_1, y_0, mipmaplevel);
	Rgba c_11 = image_->getPixel(x_1, y_1, mipmaplevel);
	Rgba c_12 = image_->getPixel(x_1, y_2, mipmaplevel);
	Rgba c_13 = image_->getPixel(x_1, y_3, mipmaplevel);

	Rgba c_20 = image_->getPixel(x_2, y_0, mipmaplevel);
	Rgba c_21 = image_->getPixel(x_2, y_1, mipmaplevel);
	Rgba c_22 = image_->getPixel(x_2, y_2, mipmaplevel);
	Rgba c_23 = image_->getPixel(x_2, y_3, mipmaplevel);

	Rgba c_30 = image_->getPixel(x_3, y_0, mipmaplevel);
	Rgba c_31 = image_->getPixel(x_3, y_1, mipmaplevel);
	Rgba c_32 = image_->getPixel(x_3, y_2, mipmaplevel);
	Rgba c_33 = image_->getPixel(x_3, y_3, mipmaplevel);

	Rgba cy_0 = cubicInterpolate__(c_00, c_10, c_20, c_30, dx);
	Rgba cy_1 = cubicInterpolate__(c_01, c_11, c_21, c_31, dx);
	Rgba cy_2 = cubicInterpolate__(c_02, c_12, c_22, c_32, dx);
	Rgba cy_3 = cubicInterpolate__(c_03, c_13, c_23, c_33, dx);

	return cubicInterpolate__(cy_0, cy_1, cy_2, cy_3, dy);
}

Rgba ImageTexture::mipMapsTrilinearInterpolation(const Point3 &p, const MipMapParams *mipmap_params) const
{
	float ds = std::max(fabsf(mipmap_params->ds_dx_), fabsf(mipmap_params->ds_dy_)) * image_->getWidth();
	float dt = std::max(fabsf(mipmap_params->dt_dx_), fabsf(mipmap_params->dt_dy_)) * image_->getHeight();
	float mipmaplevel = 0.5f * log2(ds * ds + dt * dt);

	if(mipmap_params->force_image_level_ > 0.f) mipmaplevel = mipmap_params->force_image_level_ * image_->getHighestImgIndex();

	mipmaplevel += trilinear_level_bias_;

	mipmaplevel = std::min(std::max(0.f, mipmaplevel), (float) image_->getHighestImgIndex());

	int mipmaplevel_a = (int) floor(mipmaplevel);
	int mipmaplevel_b = (int) ceil(mipmaplevel);
	float mipmaplevel_delta = mipmaplevel - (float) mipmaplevel_a;

	Rgba col = bilinearInterpolation(p, mipmaplevel_a);
	Rgba col_b = bilinearInterpolation(p, mipmaplevel_b);

	col.blend(col_b, mipmaplevel_delta);

	return col;
}

//All EWA interpolation/calculation code has been adapted from PBRT v2 (https://github.com/mmp/pbrt-v2). see LICENSES file

Rgba ImageTexture::mipMapsEwaInterpolation(const Point3 &p, float max_anisotropy, const MipMapParams *mipmap_params) const
{
	float ds_0 = fabsf(mipmap_params->ds_dx_);
	float ds_1 = fabsf(mipmap_params->ds_dy_);
	float dt_0 = fabsf(mipmap_params->dt_dx_);
	float dt_1 = fabsf(mipmap_params->dt_dy_);

	if((ds_0 * ds_0 + dt_0 * dt_0) < (ds_1 * ds_1 + dt_1 * dt_1))
	{
		std::swap(ds_0, ds_1);
		std::swap(dt_0, dt_1);
	}

	float major_length = sqrtf(ds_0 * ds_0 + dt_0 * dt_0);
	float minor_length = sqrtf(ds_1 * ds_1 + dt_1 * dt_1);

	if((minor_length * max_anisotropy < major_length) && (minor_length > 0.f))
	{
		float scale = major_length / (minor_length * max_anisotropy);
		ds_1 *= scale;
		dt_1 *= scale;
		minor_length *= scale;
	}

	if(minor_length <= 0.f) return bilinearInterpolation(p);

	float mipmaplevel = image_->getHighestImgIndex() - 1.f + log2(minor_length);

	mipmaplevel = std::min(std::max(0.f, mipmaplevel), (float) image_->getHighestImgIndex());


	int mipmaplevel_a = (int) floor(mipmaplevel);
	int mipmaplevel_b = (int) ceil(mipmaplevel);
	float mipmaplevel_delta = mipmaplevel - (float) mipmaplevel_a;

	Rgba col = ewaEllipticCalculation(p, ds_0, dt_0, ds_1, dt_1, mipmaplevel_a);
	Rgba col_b = ewaEllipticCalculation(p, ds_0, dt_0, ds_1, dt_1, mipmaplevel_b);

	col.blend(col_b, mipmaplevel_delta);

	return col;
}

inline int mod__(int a, int b)
{
	int n = int(a / b);
	a -= n * b;
	if(a < 0) a += b;
	return a;
}

Rgba ImageTexture::ewaEllipticCalculation(const Point3 &p, float d_s_0, float d_t_0, float d_s_1, float d_t_1, int mipmaplevel) const
{
	if(mipmaplevel >= image_->getHighestImgIndex())
	{
		int resx = image_->getWidth(mipmaplevel);
		int resy = image_->getHeight(mipmaplevel);

		return image_->getPixel(mod__(p.x_, resx), mod__(p.y_, resy), image_->getHighestImgIndex());
	}

	int resx = image_->getWidth(mipmaplevel);
	int resy = image_->getHeight(mipmaplevel);

	float xf = ((float)resx * (p.x_ - floor(p.x_))) - 0.5f;
	float yf = ((float)resy * (p.y_ - floor(p.y_))) - 0.5f;

	d_s_0 *= resx;
	d_s_1 *= resx;
	d_t_0 *= resy;
	d_t_1 *= resy;

	float a = d_t_0 * d_t_0 + d_t_1 * d_t_1 + 1;
	float b = -2.f * (d_s_0 * d_t_0 + d_s_1 * d_t_1);
	float c = d_s_0 * d_s_0 + d_s_1 * d_s_1 + 1;
	float inv_f = 1.f / (a * c - b * b * 0.25f);
	a *= inv_f;
	b *= inv_f;
	c *= inv_f;

	float det = -b * b + 4.f * a * c;
	float inv_det = 1.f / det;
	float u_sqrt = sqrtf(det * c);
	float v_sqrt = sqrtf(a * det);

	int s_0 = (int) ceilf(xf - 2.f * inv_det * u_sqrt);
	int s_1 = (int) floorf(xf + 2.f * inv_det * u_sqrt);
	int t_0 = (int) ceilf(yf - 2.f * inv_det * v_sqrt);
	int t_1 = (int) floorf(yf + 2.f * inv_det * v_sqrt);

	Rgba sum_col(0.f);

	float sum_wts = 0.f;
	for(int it = t_0; it <= t_1; ++it)
	{
		float tt = it - yf;
		for(int is = s_0; is <= s_1; ++is)
		{
			float ss = is - xf;

			float r_2 = a * ss * ss + b * ss * tt + c * tt * tt;
			if(r_2 < 1.f)
			{
				float weight = ewa_weight_lut_[std::min((int)floorf(r_2 * ewa_weight_lut_size_), ewa_weight_lut_size_ - 1)];
				int ismod = mod__(is, resx);
				int itmod = mod__(it, resy);

				sum_col += image_->getPixel(ismod, itmod, mipmaplevel) * weight;
				sum_wts += weight;
			}
		}
	}

	if(sum_wts > 0.f) sum_col = sum_col / sum_wts;
	else sum_col = Rgba(0.f);

	return sum_col;
}

void ImageTexture::generateEwaLookupTable()
{
	if(!ewa_weight_lut_)
	{
		Y_DEBUG << "** GENERATING EWA LOOKUP **" << YENDL;
		ewa_weight_lut_ = (float *) malloc(sizeof(float) * ewa_weight_lut_size_);
		for(int i = 0; i < ewa_weight_lut_size_; ++i)
		{
			float alpha = 2.f;
			float r_2 = float(i) / float(ewa_weight_lut_size_ - 1);
			ewa_weight_lut_[i] = expf(-alpha * r_2) - expf(-alpha);
		}
	}
}

ImageTexture::TexClipMode string2Cliptype__(const std::string &clipname)
{
	// default "repeat"
	ImageTexture::TexClipMode	tex_clipmode = ImageTexture::TexClipMode::Repeat;
	if(clipname.empty()) return tex_clipmode;
	if(clipname == "extend")		tex_clipmode = ImageTexture::TexClipMode::Extend;
	else if(clipname == "clip")		tex_clipmode = ImageTexture::TexClipMode::Clip;
	else if(clipname == "clipcube")	tex_clipmode = ImageTexture::TexClipMode::ClipCube;
	else if(clipname == "checker")	tex_clipmode = ImageTexture::TexClipMode::Checker;
	return tex_clipmode;
}

Texture *ImageTexture::factory(ParamMap &params, Scene &scene)
{
	std::string name;
	std::string intpstr;
	double gamma = 1.0;
	double expadj = 0.0;
	bool normalmap = false;
	std::string color_space_string = "Raw_Manual_Gamma";
	ColorSpace color_space = RawManualGamma;
	std::string texture_optimization_string = "optimized";
	TextureOptimization texture_optimization = TextureOptimization::Optimized;
	bool img_grayscale = false;
	ImageTexture *tex = nullptr;
	ImageHandler *ih = nullptr;
	params.getParam("interpolate", intpstr);
	params.getParam("color_space", color_space_string);
	params.getParam("gamma", gamma);
	params.getParam("exposure_adjust", expadj);
	params.getParam("normalmap", normalmap);
	params.getParam("filename", name);
	params.getParam("texture_optimization", texture_optimization_string);
	params.getParam("img_grayscale", img_grayscale);

	if(name.empty())
	{
		Y_ERROR << "ImageTexture: Required argument filename not found for image texture" << YENDL;
		return nullptr;
	}

	// interpolation type, bilinear default
	InterpolationType interpolation_type = InterpolationType::Bilinear;

	if(intpstr == "none") interpolation_type = InterpolationType::None;
	else if(intpstr == "bicubic") interpolation_type = InterpolationType::Bicubic;
	else if(intpstr == "mipmap_trilinear") interpolation_type = InterpolationType::Trilinear;
	else if(intpstr == "mipmap_ewa") interpolation_type = InterpolationType::Ewa;

	size_t l_dot = name.rfind(".") + 1;
	size_t l_slash = name.rfind("/") + 1;

	std::string ext = toLower__(name.substr(l_dot));

	ParamMap ihpm;
	ihpm["type"] = ext;
	ihpm["for_output"] = false;
	std::string ihname = "ih";
	ihname.append(toLower__(name.substr(l_slash, l_dot - l_slash - 1)));

	ih = scene.createImageHandler(ihname, ihpm);
	if(!ih)
	{
		Y_ERROR << "ImageTexture: Couldn't create image handler, dropping texture." << YENDL;
		return nullptr;
	}

	if(ih->isHdr())
	{
		if(color_space_string != "LinearRGB") Y_VERBOSE << "ImageTexture: The image is a HDR/EXR file: forcing linear RGB and ignoring selected color space '" << color_space_string << "' and the gamma setting." << YENDL;
		color_space = LinearRgb;
		if(texture_optimization_string != "none") Y_VERBOSE << "ImageTexture: The image is a HDR/EXR file: forcing texture optimization to 'none' and ignoring selected texture optimization '" << texture_optimization_string << "'" << YENDL;
		texture_optimization = TextureOptimization::None;	//FIXME DAVID: Maybe we should leave this to imageHandler factory code...
	}
	else
	{
		if(color_space_string == "sRGB") color_space = Srgb;
		else if(color_space_string == "XYZ") color_space = XyzD65;
		else if(color_space_string == "LinearRGB") color_space = LinearRgb;
		else if(color_space_string == "Raw_Manual_Gamma") color_space = RawManualGamma;
		else color_space = Srgb;

		if(texture_optimization_string == "none") texture_optimization = TextureOptimization::None;
		else if(texture_optimization_string == "optimized") texture_optimization = TextureOptimization::Optimized;
		else if(texture_optimization_string == "compressed") texture_optimization = TextureOptimization::Compressed;
		else texture_optimization = TextureOptimization::None;
	}

	ih->setColorSpace(color_space, gamma);
	ih->setTextureOptimization(texture_optimization);	//FIXME DAVID: Maybe we should leave this to imageHandler factory code...
	ih->setGrayScaleSetting(img_grayscale);

	if(!ih->loadFromFile(name))
	{
		Y_ERROR << "ImageTexture: Couldn't load image file, dropping texture." << YENDL;
		return nullptr;
	}

	tex = new ImageTexture(ih, interpolation_type, gamma, color_space);

	if(!tex)
	{
		Y_ERROR << "ImageTexture: Couldn't create image texture." << YENDL;
		return nullptr;
	}

	if(interpolation_type == InterpolationType::Trilinear || interpolation_type == InterpolationType::Ewa)
	{
		ih->generateMipMaps();
		if(!session__.getDifferentialRaysEnabled())
		{
			Y_VERBOSE << "At least one texture using mipmaps interpolation, enabling ray differentials." << YENDL;
			session__.setDifferentialRaysEnabled(true);	//If there is at least one texture using mipmaps, then enable differential rays in the rendering process.
		}

		/*//FIXME DAVID: TEST SAVING MIPMAPS. CAREFUL: IT COULD CAUSE CRASHES!
		for(int i=0; i<=ih->getHighestImgIndex(); ++i)
		{
			std::stringstream ss;
			ss << "//tmp//saved_mipmap_" << ihname << "__" << i;
			ih->saveToFile(ss.str(), i);
		}*/
	}

	// setup image
	bool rot_90 = false;
	bool even_tiles = false, odd_tiles = true;
	bool use_alpha = true, calc_alpha = false;
	int xrep = 1, yrep = 1;
	double minx = 0.0, miny = 0.0, maxx = 1.0, maxy = 1.0;
	double cdist = 0.0;
	std::string clipmode;
	bool mirror_x = false;
	bool mirror_y = false;
	float intensity = 1.f, contrast = 1.f, saturation = 1.f, hue = 0.f, factor_red = 1.f, factor_green = 1.f, factor_blue = 1.f;
	bool clamp = false;
	float trilinear_level_bias = 0.f;
	float ewa_max_anisotropy = 8.f;

	params.getParam("xrepeat", xrep);
	params.getParam("yrepeat", yrep);
	params.getParam("cropmin_x", minx);
	params.getParam("cropmin_y", miny);
	params.getParam("cropmax_x", maxx);
	params.getParam("cropmax_y", maxy);
	params.getParam("rot90", rot_90);
	params.getParam("clipping", clipmode);
	params.getParam("even_tiles", even_tiles);
	params.getParam("odd_tiles", odd_tiles);
	params.getParam("checker_dist", cdist);
	params.getParam("use_alpha", use_alpha);
	params.getParam("calc_alpha", calc_alpha);
	params.getParam("mirror_x", mirror_x);
	params.getParam("mirror_y", mirror_y);
	params.getParam("trilinear_level_bias", trilinear_level_bias);
	params.getParam("ewa_max_anisotropy", ewa_max_anisotropy);

	params.getParam("adj_mult_factor_red", factor_red);
	params.getParam("adj_mult_factor_green", factor_green);
	params.getParam("adj_mult_factor_blue", factor_blue);
	params.getParam("adj_intensity", intensity);
	params.getParam("adj_contrast", contrast);
	params.getParam("adj_saturation", saturation);
	params.getParam("adj_hue", hue);
	params.getParam("adj_clamp", clamp);

	tex->xrepeat_ = xrep;
	tex->yrepeat_ = yrep;
	tex->rot_90_ = rot_90;
	tex->setCrop(minx, miny, maxx, maxy);
	tex->calc_alpha_ = calc_alpha;
	tex->normalmap_ = normalmap;
	tex->tex_clip_mode_ = string2Cliptype__(clipmode);
	tex->checker_even_ = even_tiles;
	tex->checker_odd_ = odd_tiles;
	tex->checker_dist_ = cdist;
	tex->mirror_x_ = mirror_x;
	tex->mirror_y_ = mirror_y;

	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);

	tex->trilinear_level_bias_ = trilinear_level_bias;
	tex->ewa_max_anisotropy_ = ewa_max_anisotropy;

	if(interpolation_type == InterpolationType::Ewa) tex->generateEwaLookupTable();

	return tex;
}

END_YAFARAY
