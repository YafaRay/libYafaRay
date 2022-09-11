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

#ifndef LIBYAFARAY_MIPMAP_PARAMS_H
#define LIBYAFARAY_MIPMAP_PARAMS_H

namespace yafaray {

class MipMapParams final
{
	public:
		explicit MipMapParams(float force_image_level) : force_image_level_(force_image_level) { }
		MipMapParams(float dsdx, float dtdx, float dsdy, float dtdy) : ds_dx_(dsdx), dt_dx_(dtdx), ds_dy_(dsdy), dt_dy_(dtdy) { }

		float force_image_level_ = 0.f;
		float ds_dx_ = 0.f;
		float dt_dx_ = 0.f;
		float ds_dy_ = 0.f;
		float dt_dy_ = 0.f;
};

} //namespace yafaray

#endif //LIBYAFARAY_MIPMAP_PARAMS_H
