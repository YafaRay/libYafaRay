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

#include "common/yafaray_common.h"
#include "color/color.h"
#include <memory>

BEGIN_YAFARAY

class ParamMap;
class Scene;
class Light;
class Rgb;
class Logger;
class Vec3;

class Background
{
	public:
		static const Background * factory(Logger &logger, Scene &scene, const std::string &name, const ParamMap &params);
		explicit Background(Logger &logger) : logger_(logger) { }
		virtual ~Background() = default;
		Rgb operator()(const Vec3 &dir) const { return operator()(dir, false); }
		virtual Rgb operator()(const Vec3 &dir, bool use_ibl_blur) const { return eval(dir, use_ibl_blur); }
		Rgb eval(const Vec3 &dir) const { return eval(dir, false); }
		virtual Rgb eval(const Vec3 &dir, bool use_ibl_blur) const = 0;

	protected:
		Logger &logger_;
};

END_YAFARAY

#endif // YAFARAY_BACKGROUND_H
