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

#ifndef YAFARAY_AA_NOISE_PARAMS_H
#define YAFARAY_AA_NOISE_PARAMS_H

#include "common/yafaray_common.h"

BEGIN_YAFARAY

struct AaNoiseParams
{
	enum class DarkDetectionType : unsigned char { None, Linear, Curve };

	int samples_ = 1;
	int passes_ = 1;
	int inc_samples_ = 1; //!< sample count for additional passes
	float threshold_ = 0.05f;
	float resampled_floor_ = 0.f; //!< minimum amount of resampled pixels (% of the total pixels) below which we will automatically decrease the threshold value for the next pass
	float sample_multiplier_factor_ = 1.f;
	float light_sample_multiplier_factor_ = 1.f;
	float indirect_sample_multiplier_factor_ = 1.f;
	bool detect_color_noise_ = false;
	DarkDetectionType dark_detection_type_ = DarkDetectionType::None;
	float dark_threshold_factor_ = 0.f;
	int variance_edge_size_ = 10;
	int variance_pixels_ = 0;
	float clamp_samples_ = 0.f;
	float clamp_indirect_ = 0.f;
};

END_YAFARAY

#endif //YAFARAY_AA_NOISE_PARAMS_H
