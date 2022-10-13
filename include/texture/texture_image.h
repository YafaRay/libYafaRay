#pragma once
/****************************************************************************
 *
 *      imagetex.h: Image specific functions
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#ifndef LIBYAFARAY_TEXTURE_IMAGE_H
#define LIBYAFARAY_TEXTURE_IMAGE_H

#include "texture/texture.h"
#include "image/image.h"

namespace yafaray {

class ImageTexture final : public Texture
{
		using ThisClassType_t = ImageTexture; using ParentClassType_t = Texture;

	public:
		inline static std::string getClassName() { return "ImageTexture"; }
		static std::pair<std::unique_ptr<Texture>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		ImageTexture(Logger &logger, ParamError &param_error, const ParamMap &param_map, const Image *image);

	private:
		struct ClipMode : public Enum<ClipMode>
		{
			enum : ValueType_t { Extend, Clip, ClipCube, Repeat, Checker };
			inline static const EnumMap<ValueType_t> map_{{
					{"extend", Extend, ""},
					{"clip", Clip, ""},
					{"clipcube", ClipCube, ""},
					{"repeat", Repeat, ""},
					{"checker", Checker, ""},
				}};
		};
		[[nodiscard]] Type type() const override { return Type::Image; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_ENUM_DECL(ClipMode, clip_mode_, ClipMode::Repeat, "clipping", "Clip mode");
			PARAM_DECL(std::string , image_name_, false, "image_name", "");
			PARAM_DECL(float , exposure_adjust_, 0.f, "exposure_adjust", "");
			PARAM_DECL(bool , normal_map_, false, "normalmap", "");
			PARAM_DECL(int , xrepeat_, 1, "xrepeat", "");
			PARAM_DECL(int , yrepeat_, 1, "yrepeat", "");
			PARAM_DECL(float , cropmin_x_, 0.f, "cropmin_x", "");
			PARAM_DECL(float , cropmin_y_, 0.f, "cropmin_y", "");
			PARAM_DECL(float , cropmax_x_, 1.f, "cropmax_x", "");
			PARAM_DECL(float , cropmax_y_, 1.f, "cropmax_y", "");
			PARAM_DECL(bool , rot_90_, false, "rot90", "");
			PARAM_DECL(bool , even_tiles_, false, "even_tiles", "");
			PARAM_DECL(bool , odd_tiles_, true, "odd_tiles", "");
			PARAM_DECL(float , checker_dist_, 0.f, "checker_dist", "");
			PARAM_DECL(bool , use_alpha_, true, "use_alpha", "");
			PARAM_DECL(bool , calc_alpha_, false, "calc_alpha", "");
			PARAM_DECL(bool , mirror_x_, false, "mirror_x", "");
			PARAM_DECL(bool , mirror_y_, false, "mirror_y", "");
			PARAM_DECL(float , trilinear_level_bias_, 0.f, "trilinear_level_bias", "Manually specified delta to be added/subtracted from the calculated mipmap level. Negative values will choose higher resolution mipmaps than calculated, reducing the blurry artifacts at the cost of increasing texture noise. Positive values will choose lower resolution mipmaps than calculated. Default (and recommended) is 0.0 to use the calculated mipmaps as-is.");
			PARAM_DECL(float , ewa_max_anisotropy_, 8.f, "ewa_max_anisotropy", "Maximum anisotropy allowed for mipmap EWA algorithm. Higher values give better quality in textures seen from an angle, but render will be slower. Lower values will give more speed but lower quality in textures seen in an angle.");
		} params_;
		bool discrete() const override { return true; }
		bool isThreeD() const override { return false; }
		bool isNormalmap() const override { return params_.normal_map_; }
		Rgba getColor(const Point3f &p, const MipMapParams *mipmap_params) const override;
		Rgba getRawColor(const Point3f &p, const MipMapParams *mipmap_params) const override;
		std::array<int, 3> resolution() const override;
		void generateMipMaps() override;
		Rgba noInterpolation(const Point3f &p, int mipmap_level = 0) const;
		Rgba bilinearInterpolation(const Point3f &p, int mipmap_level = 0) const;
		Rgba bicubicInterpolation(const Point3f &p, int mipmap_level = 0) const;
		Rgba mipMapsTrilinearInterpolation(const Point3f &p, const MipMapParams *mipmap_params) const;
		Rgba mipMapsEwaInterpolation(const Point3f &p, float max_anisotropy, const MipMapParams *mipmap_params) const;
		Rgba ewaEllipticCalculation(const Point3f &p, float ds_0, float dt_0, float ds_1, float dt_1, int mipmap_level = 0) const;
		std::pair<Point3f, bool> doMapping(const Point3f &tex_point) const;
		Rgba interpolateImage(const Point3f &p, const MipMapParams *mipmap_params) const;
		static void findTextureInterpolationCoordinates(int &coord_0, int &coord_1, int &coord_2, int &coord_3, float &coord_decimal_part, float coord_float, int resolution, bool repeat, bool mirror);
		const Image *getImageFromMipMapLevel(int mipmap_level = 0) const;

		//bool grayscale_ = false;	//!< Converts the information loaded from the texture RGB to grayscale to reduce memory usage for bump or mask textures, for example. Alpha is ignored in this case. //TODO: to implement at some point
		const bool crop_x_{(params_.cropmin_x_ != 0.f) || (params_.cropmax_x_ != 1.f)};
		const bool crop_y_{(params_.cropmin_y_ != 0.f) || (params_.cropmax_y_ != 1.f)};
		const Image *image_ = nullptr;
		std::vector<std::unique_ptr<const Image>> mipmaps_;
		float original_image_file_gamma_ = 1.f;
		ColorSpace original_image_file_color_space_ = ColorSpace::RawManualGamma;
		class EwaWeightLut;
		static const EwaWeightLut ewa_weight_lut_;
};

class ImageTexture::EwaWeightLut final
{
	public:
		EwaWeightLut() noexcept;
		float get(int index) const { return items_[index]; };
		static constexpr inline int size() { return num_items_; }

	private:
		static constexpr inline int num_items_ = 128;
		std::array<float, num_items_> items_;
};

} //namespace yafaray

#endif // LIBYAFARAY_TEXTURE_IMAGE_H
