#pragma once
/****************************************************************************
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

#ifndef YAFARAY_BACKGROUND_H
#define YAFARAY_BACKGROUND_H

#include <memory>
#include "common/yafaray_common.h"

BEGIN_YAFARAY

class ParamMap;
class Scene;
class RenderData;
class Light;
class Rgb;
class Ray;
class Logger;

class Background
{
	public:
		static std::unique_ptr<Background> factory(Logger &logger, ParamMap &params, Scene &scene);
		Background(Logger &logger) : logger_(logger) { }
		virtual ~Background() = default;
		//! get the background color for a given ray
		virtual Rgb operator()(const Ray &ray, bool from_postprocessed = false) const = 0;
		virtual Rgb eval(const Ray &ray, bool from_postprocessed = false) const = 0;
		/*! get the light source representing background lighting.
			\return the light source that reproduces background lighting, or nullptr if background
					shall only be sampled from BSDFs
		*/
		bool hasIbl() const { return with_ibl_; }
		bool shootsCaustic() const { return shoot_caustic_; }

	protected:
		bool with_ibl_ = false;
		bool shoot_caustic_ = false;
		Logger &logger_;
};

END_YAFARAY

#endif // YAFARAY_BACKGROUND_H
