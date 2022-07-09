#pragma once
/****************************************************************************
 *      background_constant.h: a background using a constant color
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

#ifndef YAFARAY_BACKGROUND_CONSTANT_H
#define YAFARAY_BACKGROUND_CONSTANT_H

#include <memory>
#include "background.h"
#include "color/color.h"

namespace yafaray {

class ConstantBackground final : public Background
{
	public:
		static const Background * factory(Logger &logger, Scene &scene, const std::string &name, const ParamMap &params);

	private:
		ConstantBackground(Logger &logger, Rgb col);
		Rgb eval(const Vec3f &dir, bool use_ibl_blur) const override;

		Rgb color_;
};

} //namespace yafaray

#endif // YAFARAY_BACKGROUND_CONSTANT_H