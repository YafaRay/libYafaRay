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

#ifndef YAFARAY_RENDER_DATA_H
#define YAFARAY_RENDER_DATA_H

namespace yafaray {

struct RayDivision final
{
	int division_ = 1; //!< keep track of trajectory splitting
	int offset_ = 0; //!< keep track of trajectory splitting
	float decorrelation_1_ = 0.f; //!< used to decorrelate samples from trajectory splitting*/
	float decorrelation_2_ = 0.f; //!< used to decorrelate samples from trajectory splitting*/
};

struct PixelSamplingData final
{
	int sample_ = 0; //!< number of samples inside this pixels so far
	int number_ = 0;
	unsigned int offset_ = 0; //!< a "noise-like" pixel offset you may use to decorelate sampling of adjacent pixel.
	float time_ = 0.f;
};

} //namespace yafaray

#endif //YAFARAY_RENDER_DATA_H
