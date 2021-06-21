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
#include "common/memory.h"

BEGIN_YAFARAY

class Random;
class Camera;
class PhotonMap;
class Logger;

class RenderData final
{
	public:
		RenderData() = default;
		RenderData(Random *rand) : prng_(rand) { }
		RenderData(const RenderData &r) = delete;
		void setDefaults();

		int raylevel_ = 0;
		float depth_ = 0;
		int current_pass_ = 0;
		int pixel_sample_ = 0; //!< number of samples inside this pixels so far
		int ray_division_ = 1; //!< keep track of trajectory splitting
		int ray_offset_ = 0; //!< keep track of trajectory splitting
		float dc_1_ = 0.f, dc_2_ = 0.f; //!< used to decorrelate samples from trajectory splitting
		int pixel_number_ = 0;
		int thread_id_ = 0; //!< identify the current render thread; shall range from 0 to scene_t::getNumThreads() - 1
		unsigned int sampling_offs_ = 0; //!< a "noise-like" pixel offset you may use to decorelate sampling of adjacent pixel.
		bool chromatic_ = true; //!< indicates wether the full spectrum is calculated (true) or only a single wavelength (false).
		bool lights_geometry_material_emit_ = false; //!< indicate that emission of materials assiciated to lights shall be included, for correctly visible lights etc.
		float wavelength_ = 0.f; //!< the (normalized) wavelength being used when chromatic is false. The range is defined going from 400nm (0.0) to 700nm (1.0), although the widest range humans can perceive is ofteb given 380-780nm.
		float time_ = 0.f; //!< the current (normalized) frame time
		const Camera *cam_ = nullptr;
		Random *prng_ = nullptr; //!< a pseudorandom number generator
		mutable void *arena_ = nullptr; //!< a fixed amount of memory where materials may keep data to avoid recalculations...really need better memory management :(
};

inline void RenderData::setDefaults()
{
	raylevel_ = 0;
	chromatic_ = true;
	ray_division_ = 1;
	ray_offset_ = 0;
	dc_1_ = dc_2_ = 0.f;
}

END_YAFARAY

#endif //YAFARAY_RENDER_DATA_H
