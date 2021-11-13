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

#include "common/yafaray_common.h"
#include <stack>

BEGIN_YAFARAY

class RandomGenerator;
class Camera;
class PhotonMap;
class Logger;

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
};

class RenderData final
{
	public:
		RenderData() = default;
		bool chromatic_ = true; //!< indicates wether the full spectrum is calculated (true) or only a single wavelength (false).
//		bool lights_geometry_material_emit_ = false; //!< indicate that emission of materials assiciated to lights shall be included, for correctly visible lights etc.
		float wavelength_ = 0.f; //!< the (normalized) wavelength being used when chromatic is false. The range is defined going from 400nm (0.0) to 700nm (1.0), although the widest range humans can perceive is ofteb given 380-780nm.
};

END_YAFARAY

#endif //YAFARAY_RENDER_DATA_H
