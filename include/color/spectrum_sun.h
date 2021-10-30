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

#ifndef YAFARAY_SPECTRUM_SUN_H
#define YAFARAY_SPECTRUM_SUN_H

#include "common/yafaray_common.h"
#include "color/spectrum.h"
#include <algorithm>
#include <vector>

BEGIN_YAFARAY

namespace spectrum_sun
{

// k_o Spectrum table from pg 127, MI.
static constexpr std::array<float, 64> k_o_wavelengths
{
	300, 305, 310, 315, 320,
	325, 330, 335, 340, 345,
	350, 355,

	445, 450, 455, 460, 465,
	470, 475, 480, 485, 490,
	495,

	500, 505, 510, 515, 520,
	525, 530, 535, 540, 545,
	550, 555, 560, 565, 570,
	575, 580, 585, 590, 595,

	600, 605, 610, 620, 630,
	640, 650, 660, 670, 680,
	690,

	700, 710, 720, 730, 740,
	750, 760, 770, 780, 790,
};

static constexpr std::array<float, 64> k_o_amplitudes
{
	10.0, 4.8, 2.7, 1.35, .8,
	.380, .160, .075, .04, .019,
	.007, .0,

	.003, .003, .004, .006, .008,
	.009, .012, .014,	.017, .021,
	.025,

	.03, .035, .04, .045, .048,
	.057, .063, .07, .075, .08,
	.085, .095, .103, .110, .12,
	.122, .12, .118, .115, .12,

	.125, .130, .12, .105, .09,
	.079,	.067, .057, .048, .036,
	.028,

	.023, .018, .014, .011, .010,
	.009,	.007, .004, .0, .0
};


// k_g Spectrum table from pg 130, MI.
static constexpr std::array<float, 4> k_g_wavelengths
{
	759, 760, 770, 771
};

static constexpr std::array<float, 4> k_g_amplitudes
{
	0, 3.0, 0.210, 0
};

// k_wa Spectrum table from pg 130, MI.
static constexpr std::array<float, 13> k_wa_wavelengths
{
	689, 690, 700, 710, 720, 730, 740,
	750, 760, 770, 780, 790, 800
};

static constexpr std::array<float, 13> k_wa_amplitudes
{
	0,
	0.160e-1,
	0.240e-1,
	0.125e-1,
	0.100e+1,
	0.870,
	0.610e-1,
	0.100e-2,
	0.100e-4,
	0.100e-4,
	0.600e-3,
	0.175e-1,
	0.360e-1
};

// 380-750 by 10nm
static constexpr std::array<float, 38> sol_amplitudes
{
	165.5, 162.3, 211.2, 258.8, 258.2,
	242.3, 267.6, 296.6, 305.4, 300.6,
	306.6, 288.3, 287.1, 278.2, 271.0,
	272.3, 263.6, 255.0, 250.6, 253.1,
	253.5, 251.3, 246.3, 241.7, 236.8,
	232.1, 228.2, 223.4, 219.7, 215.3,
	211.0, 207.3, 202.4, 198.7, 194.3,
	190.7, 186.3, 182.6
};

} //namespace spectrum_sun

struct IrregularSpectrum final
{
	IrregularSpectrum(const float *amps, const float *wl, int n)
	{
		for(int i = 0; i < n; ++i) { wavelen_.push_back(wl[i]); amplitude_.push_back(amps[i]); }
	}
	float sample(float wl);
	std::vector<float> wavelen_;
	std::vector<float> amplitude_;
};

inline float IrregularSpectrum::sample(float wl)
{
	const auto i = lower_bound(wavelen_.begin(), wavelen_.end(), wl);
	if(i == wavelen_.begin() || i == wavelen_.end()) return 0.f;
	int index = (i - wavelen_.begin()) - 1;
	float delta = (wl - wavelen_[index]) / (wavelen_[index + 1] - wavelen_[index]);
	return (1.f - delta) * amplitude_[index] + delta * amplitude_[index + 1];
}

END_YAFARAY

#endif // YAFARAY_SPECTRUM_SUN_H