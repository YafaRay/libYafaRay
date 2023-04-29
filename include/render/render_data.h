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

#ifndef LIBYAFARAY_RENDER_DATA_H
#define LIBYAFARAY_RENDER_DATA_H

namespace yafaray {

struct RayDivision final
{
	int division_{1}; //!< keep track of trajectory splitting
	int offset_{0}; //!< keep track of trajectory splitting
	float decorrelation_1_{0.f}; //!< used to decorrelate samples from trajectory splitting*/
	float decorrelation_2_{0.f}; //!< used to decorrelate samples from trajectory splitting*/
};

struct PixelSamplingData final
{
	PixelSamplingData(int thread_id, int number, unsigned int offset, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier) :
	thread_id_{thread_id}, number_{number}, offset_{offset},
	aa_light_sample_multiplier_{aa_light_sample_multiplier},
	aa_indirect_sample_multiplier_{aa_indirect_sample_multiplier} { }
	int thread_id_{0};
	int sample_{0}; //!< number of samples inside this pixels so far
	int number_{0};
	unsigned int offset_{0}; //!< a "noise-like" pixel offset you may use to decorelate sampling of adjacent pixel.
	float aa_light_sample_multiplier_{1.f};
	float aa_indirect_sample_multiplier_{1.f};
	float time_{0.f};
};

} //namespace yafaray

#endif //LIBYAFARAY_RENDER_DATA_H
