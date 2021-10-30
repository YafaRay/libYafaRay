#pragma once
/****************************************************************************
 *      background_texture.h: a background using the texture class
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
 */

#ifndef YAFARAY_BACKGROUND_TEXTURE_H
#define YAFARAY_BACKGROUND_TEXTURE_H

#include <memory>
#include "background.h"
#include "color/color.h"

BEGIN_YAFARAY

class Scene;
class ParamMap;
class Texture;

class TextureBackground final : public Background
{
	public:
		static std::unique_ptr<Background> factory(Logger &logger, ParamMap &params, Scene &scene);

	private:
		enum Projection { Spherical = 0, Angular };
		TextureBackground(Logger &logger, const Texture *texture, Projection proj, float bpower, float rot, bool ibl, float ibl_blur, bool with_caustic);
		virtual Rgb operator()(const Ray &ray, bool use_ibl_blur = false) const override;
		virtual Rgb eval(const Ray &ray, bool use_ibl_blur = false) const override;

		const Texture *tex_;
		Projection project_;
		float power_;
		float rotation_;
		float sin_r_, cos_r_;
		float ibl_blur_mipmap_level_; //Calculated based on the IBL_Blur parameter. As mipmap levels have half size each, this parameter is not linear
};

END_YAFARAY

#endif //YAFARAY_BACKGROUND_TEXTURE_H