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
#include "common/string.h"
#include "common/param.h"
#include "scene/scene.h"
#include "math/interpolation.h"
#include "format/format.h"
#include "common/file.h"
#include "image/image_manipulation.h"

namespace yafaray {

const EwaWeightLut ImageTexture::ewa_weight_lut_;

ImageTexture::ImageTexture(Logger &logger, std::shared_ptr<Image> image) : Texture(logger)
{
	images_.emplace_back(std::move(image));
}

std::array<int, 3> ImageTexture::resolution() const
{
	return { images_[0]->getWidth(), images_[0]->getHeight(), 0 };
}

Rgba ImageTexture::interpolateImage(const Point3f &p, const MipMapParams *mipmap_params) const
{
	switch(interpolation_type_)
	{
		case InterpolationType::None: return noInterpolation(p);
		case InterpolationType::Bicubic: return bicubicInterpolation(p);
		case InterpolationType::Trilinear:
			if(mipmap_params) return mipMapsTrilinearInterpolation(p, mipmap_params);
			else return bilinearInterpolation(p);
		case InterpolationType::Ewa:
			if(mipmap_params) return mipMapsEwaInterpolation(p, ewa_max_anisotropy_, mipmap_params);
			else return bilinearInterpolation(p);
		default: //By default use Bilinear
		case InterpolationType::Bilinear: return bilinearInterpolation(p);
	}
}

Rgba ImageTexture::getColor(const Point3f &p, const MipMapParams *mipmap_params) const
{
	const auto [p_mapped, outside] = doMapping({{p[Axis::X], -p[Axis::Y], p[Axis::Z]}});
	if(outside) return Rgba{0.f};
	else return applyAdjustments(interpolateImage(p_mapped, mipmap_params));
}

Rgba ImageTexture::getRawColor(const Point3f &p, const MipMapParams *mipmap_params) const
{
	// As from v3.2.0 all image buffers are already Linear RGB, if any part of the code requires the original "raw" color, a "de-linearization" (encoding again into the original color space) takes place in this function.

	// For example for Non-RGB / Stencil / Bump / Normal maps, etc, textures are typically already linear and the user should select "linearRGB" in the texture properties, but if the user (by mistake) keeps the default sRGB for them, for example, the default linearization would apply a sRGB to linearRGB conversion that messes up the texture values. This function will try to reconstruct the original texture values. In this case (if the user selected incorrectly sRGB for a normal map, for example), this function will prevent wrong results, but it will be slower and it could be slightly inaccurate as the interpolation will take place in the (incorrectly) linearized texels.

	//If the user correctly selected "linearRGB" for these kind of textures, the function below will not make any changes to the color and will keep the texture "as is" without any linearization nor de-linearization, which is the ideal case (fast and correct).

	//The user is responsible to select the correct textures color spaces, if the user does not do it, results may not be the expected. This function is only a coarse "fail safe"

	Rgba ret = getColor(p, mipmap_params);
	ret.colorSpaceFromLinearRgb(original_image_file_color_space_, original_image_file_gamma_);
	return ret;
}

std::pair<Point3f, bool> ImageTexture::doMapping(const Point3f &tex_point) const
{
	bool outside = false;
	Point3f mapped{0.5f * tex_point + Vec3f{0.5f} }; //FIXME DAVID: does the Vec3 portion make sense?
	// repeat, only valid for REPEAT clipmode
	if(tex_clip_mode_ == ClipMode::Repeat)
	{
		if(xrepeat_ > 1) mapped[Axis::X] *= static_cast<float>(xrepeat_);
		if(yrepeat_ > 1) mapped[Axis::Y] *= static_cast<float>(yrepeat_);
		if(mirror_x_ && static_cast<int>(ceilf(mapped[Axis::X])) % 2 == 0) mapped[Axis::X] = -mapped[Axis::X];
		if(mirror_y_ && static_cast<int>(ceilf(mapped[Axis::Y])) % 2 == 0) mapped[Axis::Y] = -mapped[Axis::Y];
		if(mapped[Axis::X] > 1.f) mapped[Axis::X] -= std::trunc(mapped[Axis::X]);
		else if(mapped[Axis::X] < 0.f)
			mapped[Axis::X] += 1.f - std::trunc(mapped[Axis::X]);
		if(mapped[Axis::Y] > 1.f) mapped[Axis::Y] -= std::trunc(mapped[Axis::Y]);
		else if(mapped[Axis::Y] < 0.f)
			mapped[Axis::Y] += 1.f - std::trunc(mapped[Axis::Y]);
	}
	if(cropx_) mapped[Axis::X] = cropminx_ + mapped[Axis::X] * (cropmaxx_ - cropminx_);
	if(cropy_) mapped[Axis::Y] = cropminy_ + mapped[Axis::Y] * (cropmaxy_ - cropminy_);
	if(rot_90_) std::swap(mapped[Axis::X], mapped[Axis::Y]);

	switch(tex_clip_mode_)
	{
		case ClipMode::ClipCube:
		{
			if(mapped[Axis::X] < 0.f || mapped[Axis::X] > 1.f || mapped[Axis::Y] < 0.f || mapped[Axis::Y] > 1.f || mapped[Axis::Z] < -1.f || mapped[Axis::Z] > 1.f)
				outside = true;
			break;
		}
		case ClipMode::Checker:
		{
			const float xs = std::floor(mapped[Axis::X]);
			const float ys = std::floor(mapped[Axis::Y]);
			mapped[Axis::X] -= xs;
			mapped[Axis::Y] -= ys;
			if(!checker_odd_ && !(static_cast<int>(xs + ys) & 1))
			{
				outside = true;
				break;
			}
			if(!checker_even_ && (static_cast<int>(xs + ys) & 1))
			{
				outside = true;
				break;
			}
			// scale around center, (0.5, 0.5)
			if(checker_dist_ < 1.f)
			{
				mapped[Axis::X] = (mapped[Axis::X] - 0.5f) / (1.f - checker_dist_) + 0.5f;
				mapped[Axis::Y] = (mapped[Axis::Y] - 0.5f) / (1.f - checker_dist_) + 0.5f;
			}
			// continue to TCL_CLIP
		}
		case ClipMode::Clip:
		{
			if(mapped[Axis::X] < 0.f || mapped[Axis::X] > 1.f || mapped[Axis::Y] < 0.f || mapped[Axis::Y] > 1.f)
				outside = true;
			break;
		}
		case ClipMode::Extend:
		{
			if(mapped[Axis::X] > 0.99999f) mapped[Axis::X] = 0.99999f;
			else if(mapped[Axis::X] < 0.f)
				mapped[Axis::X] = 0.f;
			if(mapped[Axis::Y] > 0.99999f) mapped[Axis::Y] = 0.99999f;
			else if(mapped[Axis::Y] < 0.f)
				mapped[Axis::Y] = 0.f;
			// no break, fall thru to TEX_REPEAT
		}
		default:
		case ClipMode::Repeat: outside = false; break;
	}
	return {std::move(mapped), outside };
}

void ImageTexture::setCrop(float minx, float miny, float maxx, float maxy)
{
	cropminx_ = minx, cropmaxx_ = maxx, cropminy_ = miny, cropmaxy_ = maxy;
	cropx_ = ((cropminx_ != 0.0) || (cropmaxx_ != 1.0));
	cropy_ = ((cropminy_ != 0.0) || (cropmaxy_ != 1.0));
}

void ImageTexture::findTextureInterpolationCoordinates(int &coord_0, int &coord_1, int &coord_2, int &coord_3, float &coord_decimal_part, float coord_float, int resolution, bool repeat, bool mirror)
{
	if(repeat)
	{
		coord_1 = (static_cast<int>(coord_float)) % resolution;
		if(mirror)
		{
			if(coord_float < 0.f)
			{
				coord_0 = 1 % resolution;
				coord_2 = coord_1;
				coord_3 = coord_0;
				coord_decimal_part = -coord_float;
			}
			else if(coord_float >= (resolution - 1))
			{
				coord_0 = (2 * resolution - 1) % resolution;
				coord_2 = coord_1;
				coord_3 = coord_0;
				coord_decimal_part = coord_float - (static_cast<int>(coord_float));
			}
			else
			{
				coord_0 = (resolution + coord_1 - 1) % resolution;
				coord_2 = coord_1 + 1;
				if(coord_2 >= resolution) coord_2 = (2 * resolution - coord_2) % resolution;
				coord_3 = coord_1 + 2;
				if(coord_3 >= resolution) coord_3 = (2 * resolution - coord_3) % resolution;
				coord_decimal_part = coord_float - (static_cast<int>(coord_float));
			}
		}
		else
		{
			if(coord_float > 0.f)
			{
				coord_0 = (resolution + coord_1 - 1) % resolution;
				coord_2 = (coord_1 + 1) % resolution;
				coord_3 = (coord_1 + 2) % resolution;
				coord_decimal_part = coord_float - (static_cast<int>(coord_float));
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
		coord_1 = std::max(0, std::min(resolution - 1, (static_cast<int>(coord_float))));
		if(coord_float > 0.f) coord_2 = std::min(resolution - 1, coord_1 + 1);
		else coord_2 = 0;
		coord_0 = std::max(0, coord_1 - 1);
		coord_3 = std::min(resolution - 1, coord_2 + 1);
		coord_decimal_part = coord_float - std::floor(coord_float);
	}
}

Rgba ImageTexture::noInterpolation(const Point3f &p, int mipmap_level) const
{
	const int resx = images_.at(mipmap_level)->getWidth();
	const int resy = images_.at(mipmap_level)->getHeight();

	const float xf = (static_cast<float>(resx) * (p[Axis::X] - std::floor(p[Axis::X])));
	const float yf = (static_cast<float>(resy) * (p[Axis::Y] - std::floor(p[Axis::Y])));

	int x_0, x_1, x_2, x_3, y_0, y_1, y_2, y_3;
	float dx, dy;
	findTextureInterpolationCoordinates(x_0, x_1, x_2, x_3, dx, xf, resx, tex_clip_mode_ == ClipMode::Repeat, mirror_x_);
	findTextureInterpolationCoordinates(y_0, y_1, y_2, y_3, dy, yf, resy, tex_clip_mode_ == ClipMode::Repeat, mirror_y_);
	return images_.at(mipmap_level)->getColor({{x_1, y_1}});
}

Rgba ImageTexture::bilinearInterpolation(const Point3f &p, int mipmap_level) const
{
	const int resx = images_.at(mipmap_level)->getWidth();
	const int resy = images_.at(mipmap_level)->getHeight();

	const float xf = (static_cast<float>(resx) * (p[Axis::X] - std::floor(p[Axis::X]))) - 0.5f;
	const float yf = (static_cast<float>(resy) * (p[Axis::Y] - std::floor(p[Axis::Y]))) - 0.5f;

	int x_0, x_1, x_2, x_3, y_0, y_1, y_2, y_3;
	float dx, dy;
	findTextureInterpolationCoordinates(x_0, x_1, x_2, x_3, dx, xf, resx, tex_clip_mode_ == ClipMode::Repeat, mirror_x_);
	findTextureInterpolationCoordinates(y_0, y_1, y_2, y_3, dy, yf, resy, tex_clip_mode_ == ClipMode::Repeat, mirror_y_);

	const Rgba c_11 = images_.at(mipmap_level)->getColor({{x_1, y_1}});
	const Rgba c_21 = images_.at(mipmap_level)->getColor({{x_2, y_1}});
	const Rgba c_12 = images_.at(mipmap_level)->getColor({{x_1, y_2}});
	const Rgba c_22 = images_.at(mipmap_level)->getColor({{x_2, y_2}});

	const float w_11 = (1 - dx) * (1 - dy);
	const float w_12 = (1 - dx) * dy;
	const float w_21 = dx * (1 - dy);
	const float w_22 = dx * dy;

	return (w_11 * c_11) + (w_12 * c_12) + (w_21 * c_21) + (w_22 * c_22);
}

Rgba ImageTexture::bicubicInterpolation(const Point3f &p, int mipmap_level) const
{
	const int resx = images_.at(mipmap_level)->getWidth();
	const int resy = images_.at(mipmap_level)->getHeight();

	const float xf = (static_cast<float>(resx) * (p[Axis::X] - std::floor(p[Axis::X]))) - 0.5f;
	const float yf = (static_cast<float>(resy) * (p[Axis::Y] - std::floor(p[Axis::Y]))) - 0.5f;

	int x_0, x_1, x_2, x_3, y_0, y_1, y_2, y_3;
	float dx, dy;
	findTextureInterpolationCoordinates(x_0, x_1, x_2, x_3, dx, xf, resx, tex_clip_mode_ == ClipMode::Repeat, mirror_x_);
	findTextureInterpolationCoordinates(y_0, y_1, y_2, y_3, dy, yf, resy, tex_clip_mode_ == ClipMode::Repeat, mirror_y_);

	const Rgba c_00 = images_.at(mipmap_level)->getColor({{x_0, y_0}});
	const Rgba c_01 = images_.at(mipmap_level)->getColor({{x_0, y_1}});
	const Rgba c_02 = images_.at(mipmap_level)->getColor({{x_0, y_2}});
	const Rgba c_03 = images_.at(mipmap_level)->getColor({{x_0, y_3}});

	const Rgba c_10 = images_.at(mipmap_level)->getColor({{x_1, y_0}});
	const Rgba c_11 = images_.at(mipmap_level)->getColor({{x_1, y_1}});
	const Rgba c_12 = images_.at(mipmap_level)->getColor({{x_1, y_2}});
	const Rgba c_13 = images_.at(mipmap_level)->getColor({{x_1, y_3}});

	const Rgba c_20 = images_.at(mipmap_level)->getColor({{x_2, y_0}});
	const Rgba c_21 = images_.at(mipmap_level)->getColor({{x_2, y_1}});
	const Rgba c_22 = images_.at(mipmap_level)->getColor({{x_2, y_2}});
	const Rgba c_23 = images_.at(mipmap_level)->getColor({{x_2, y_3}});

	const Rgba c_30 = images_.at(mipmap_level)->getColor({{x_3, y_0}});
	const Rgba c_31 = images_.at(mipmap_level)->getColor({{x_3, y_1}});
	const Rgba c_32 = images_.at(mipmap_level)->getColor({{x_3, y_2}});
	const Rgba c_33 = images_.at(mipmap_level)->getColor({{x_3, y_3}});

	const Rgba cy_0 = math::cubicInterpolate(c_00, c_10, c_20, c_30, dx);
	const Rgba cy_1 = math::cubicInterpolate(c_01, c_11, c_21, c_31, dx);
	const Rgba cy_2 = math::cubicInterpolate(c_02, c_12, c_22, c_32, dx);
	const Rgba cy_3 = math::cubicInterpolate(c_03, c_13, c_23, c_33, dx);

	return math::cubicInterpolate(cy_0, cy_1, cy_2, cy_3, dy);
}

Rgba ImageTexture::mipMapsTrilinearInterpolation(const Point3f &p, const MipMapParams *mipmap_params) const
{
	const float ds = std::max(std::abs(mipmap_params->ds_dx_), std::abs(mipmap_params->ds_dy_)) * images_.at(0)->getWidth();
	const float dt = std::max(std::abs(mipmap_params->dt_dx_), std::abs(mipmap_params->dt_dy_)) * images_.at(0)->getHeight();
	float mipmap_level = 0.5f * math::log2(ds * ds + dt * dt);

	if(mipmap_params->force_image_level_ > 0.f) mipmap_level = mipmap_params->force_image_level_ * static_cast<float>(images_.size() - 1);

	mipmap_level += trilinear_level_bias_;

	mipmap_level = std::min(std::max(0.f, mipmap_level), static_cast<float>(images_.size() - 1));

	const int mipmap_level_a = static_cast<int>(std::floor(mipmap_level));
	const int mipmap_level_b = static_cast<int>(std::ceil(mipmap_level));
	const float mipmap_level_delta = mipmap_level - static_cast<float>(mipmap_level_a);

	Rgba col = bilinearInterpolation(p, mipmap_level_a);
	const Rgba col_b = bilinearInterpolation(p, mipmap_level_b);

	col.blend(col_b, mipmap_level_delta);
	return col;
}

//All EWA interpolation/calculation code has been adapted from PBRT v2 (https://github.com/mmp/pbrt-v2). see LICENSES file

Rgba ImageTexture::mipMapsEwaInterpolation(const Point3f &p, float max_anisotropy, const MipMapParams *mipmap_params) const
{
	float ds_0 = std::abs(mipmap_params->ds_dx_);
	float ds_1 = std::abs(mipmap_params->ds_dy_);
	float dt_0 = std::abs(mipmap_params->dt_dx_);
	float dt_1 = std::abs(mipmap_params->dt_dy_);

	if((ds_0 * ds_0 + dt_0 * dt_0) < (ds_1 * ds_1 + dt_1 * dt_1))
	{
		std::swap(ds_0, ds_1);
		std::swap(dt_0, dt_1);
	}

	const float major_length = sqrtf(ds_0 * ds_0 + dt_0 * dt_0);
	float minor_length = sqrtf(ds_1 * ds_1 + dt_1 * dt_1);

	if((minor_length * max_anisotropy < major_length) && (minor_length > 0.f))
	{
		const float scale = major_length / (minor_length * max_anisotropy);
		ds_1 *= scale;
		dt_1 *= scale;
		minor_length *= scale;
	}

	if(minor_length <= 0.f) return bilinearInterpolation(p);

	float mipmap_level = static_cast<float>(images_.size() - 1) - 1.f + math::log2(minor_length);
	mipmap_level = std::min(std::max(0.f, mipmap_level), static_cast<float>(images_.size() - 1));

	const int mipmap_level_a = static_cast<int>(std::floor(mipmap_level));
	const int mipmap_level_b = static_cast<int>(std::ceil(mipmap_level));
	const float mipmap_level_delta = mipmap_level - static_cast<float>(mipmap_level_a);

	Rgba col = ewaEllipticCalculation(p, ds_0, dt_0, ds_1, dt_1, mipmap_level_a);
	const Rgba col_b = ewaEllipticCalculation(p, ds_0, dt_0, ds_1, dt_1, mipmap_level_b);

	col.blend(col_b, mipmap_level_delta);

	return col;
}

Rgba ImageTexture::ewaEllipticCalculation(const Point3f &p, float ds_0, float dt_0, float ds_1, float dt_1, int mipmap_level) const
{
	if(mipmap_level >= static_cast<float>(images_.size() - 1))
	{
		const int resx = images_.at(0)->getWidth();
		const int resy = images_.at(0)->getHeight();
		return images_.at(images_.size() - 1)->getColor({{math::mod(static_cast<int>(p[Axis::X]), resx), math::mod(static_cast<int>(p[Axis::Y]), resy)}});
	}

	const int resx = images_.at(mipmap_level)->getWidth();
	const int resy = images_.at(mipmap_level)->getHeight();

	const float xf = (static_cast<float>(resx) * (p[Axis::X] - std::floor(p[Axis::X]))) - 0.5f;
	const float yf = (static_cast<float>(resy) * (p[Axis::Y] - std::floor(p[Axis::Y]))) - 0.5f;

	ds_0 *= resx;
	ds_1 *= resx;
	dt_0 *= resy;
	dt_1 *= resy;

	float a = dt_0 * dt_0 + dt_1 * dt_1 + 1;
	float b = -2.f * (ds_0 * dt_0 + ds_1 * dt_1);
	float c = ds_0 * ds_0 + ds_1 * ds_1 + 1;
	const float inv_f = 1.f / (a * c - b * b * 0.25f);
	a *= inv_f;
	b *= inv_f;
	c *= inv_f;

	const float det = -b * b + 4.f * a * c;
	const float inv_det = 1.f / det;
	const float u_sqrt = sqrtf(det * c);
	const float v_sqrt = sqrtf(a * det);

	const int s_0 = static_cast<int>(ceilf(xf - 2.f * inv_det * u_sqrt));
	const int s_1 = static_cast<int>(floorf(xf + 2.f * inv_det * u_sqrt));
	const int t_0 = static_cast<int>(ceilf(yf - 2.f * inv_det * v_sqrt));
	const int t_1 = static_cast<int>(floorf(yf + 2.f * inv_det * v_sqrt));

	Rgba sum_col(0.f);

	constexpr int ewa_weight_lut_size = EwaWeightLut::size();
	float sum_wts = 0.f;
	for(int it = t_0; it <= t_1; ++it)
	{
		const float tt = it - yf;
		for(int is = s_0; is <= s_1; ++is)
		{
			const float ss = is - xf;
			const float r_2 = a * ss * ss + b * ss * tt + c * tt * tt;
			if(r_2 < 1.f)
			{
				const float weight = ewa_weight_lut_.get(std::min(static_cast<int>(floorf(r_2 * ewa_weight_lut_size)), ewa_weight_lut_size - 1));
				const int ismod = math::mod(is, resx);
				const int itmod = math::mod(it, resy);
				sum_col += images_.at(mipmap_level)->getColor({{ismod, itmod}}) * weight;
				sum_wts += weight;
			}
		}
	}
	if(sum_wts > 0.f) sum_col = sum_col / sum_wts;
	else sum_col = Rgba(0.f);
	return sum_col;
}

EwaWeightLut::EwaWeightLut() noexcept
{
	for(int i = 0; i < num_items_; ++i)
	{
		constexpr float alpha = 2.f;
		const float r_2 = static_cast<float>(i) / static_cast<float>(num_items_ - 1);
		items_[i] = std::exp(-alpha * r_2) - std::exp(-alpha);
	}
}

void ImageTexture::generateMipMaps()
{
	image_manipulation::generateMipMaps(logger_, images_);
}

ImageTexture::ClipMode ImageTexture::string2Cliptype(const std::string &clipname)
{
	// default "repeat"
	ImageTexture::ClipMode	tex_clipmode = ImageTexture::ClipMode::Repeat;
	if(clipname.empty()) return tex_clipmode;
	if(clipname == "extend")		tex_clipmode = ImageTexture::ClipMode::Extend;
	else if(clipname == "clip")		tex_clipmode = ImageTexture::ClipMode::Clip;
	else if(clipname == "clipcube")	tex_clipmode = ImageTexture::ClipMode::ClipCube;
	else if(clipname == "checker")	tex_clipmode = ImageTexture::ClipMode::Checker;
	return tex_clipmode;
}

Texture * ImageTexture::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params)
{
	std::string image_name;
	std::string interpolation_type_str;
	double gamma = 1.0;
	double expadj = 0.0;
	bool normalmap = false;
	params.getParam("interpolate", interpolation_type_str);
	params.getParam("gamma", gamma);
	params.getParam("exposure_adjust", expadj);
	params.getParam("normalmap", normalmap);
	params.getParam("image_name", image_name);

	const InterpolationType interpolation_type = getInterpolationTypeFromName(interpolation_type_str);

	if(image_name.empty())
	{
		logger.logError("ImageTexture: Required argument image_name not found for image texture");
		return nullptr;
	}

	std::shared_ptr<Image> image = scene.getImage(image_name);
	if(!image)
	{
		logger.logError("ImageTexture: Couldn't load image file, dropping texture.");
		return nullptr;
	}

	auto tex = new ImageTexture(logger, image);
	if(!tex) //FIXME: this will never be true, replace by exception handling??
	{
		logger.logError("ImageTexture: Couldn't create image texture.");
		return nullptr;
	}

	tex->original_image_file_color_space_ = image->getColorSpace();
	tex->original_image_file_gamma_ = image->getGamma();

	tex->interpolation_type_ = interpolation_type;
	if(interpolation_type == InterpolationType::Trilinear || interpolation_type == InterpolationType::Ewa)
	{
		tex->generateMipMaps();

		/*//FIXME DAVID: TEST SAVING MIPMAPS. CAREFUL: IT COULD CAUSE CRASHES!
		for(int i=0; i<=format->getHighestImgIndex(); ++i)
		{
			std::stringstream ss;
			ss << "//tmp//saved_mipmap_" << ihname << "_global" << i;
			format->saveToFile(ss.str(), i);
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
	tex->tex_clip_mode_ = string2Cliptype(clipmode);
	tex->checker_even_ = even_tiles;
	tex->checker_odd_ = odd_tiles;
	tex->checker_dist_ = cdist;
	tex->mirror_x_ = mirror_x;
	tex->mirror_y_ = mirror_y;

	tex->setAdjustments(intensity, contrast, saturation, hue, clamp, factor_red, factor_green, factor_blue);

	tex->trilinear_level_bias_ = trilinear_level_bias;
	tex->ewa_max_anisotropy_ = ewa_max_anisotropy;

	return tex;
}

} //namespace yafaray
