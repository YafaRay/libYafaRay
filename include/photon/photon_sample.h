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

#ifndef LIBYAFARAY_PHOTON_SAMPLE_H
#define LIBYAFARAY_PHOTON_SAMPLE_H

#include "material/sample.h"

namespace yafaray {

struct PSample final : Sample
{
	PSample(float s_1, float s_2, float s_3, BsdfFlags sflags, const Rgb &l_col, const Rgb &transm = Rgb(1.f)):
	Sample(s_1, s_2, sflags), s_3_(s_3), lcol_(l_col), alpha_(transm) {}
	float s_3_;
	const Rgb lcol_; //!< the photon color from last scattering
	const Rgb alpha_; //!< the filter color between last scattering and this hit (not pre-applied to lcol!)
	Rgb color_; //!< the new color after scattering, i.e. what will be lcol for next scatter.
};

} //namespace yafaray

#endif //LIBYAFARAY_PHOTON_SAMPLE_H
