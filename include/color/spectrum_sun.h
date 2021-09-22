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


// k_o Spectrum table from pg 127, MI.
static constexpr float k_o_wavelengths_global[64] =
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




static constexpr float k_o_amplitudes_global[64] =
{
	10.0,  4.8,  2.7,   1.35,  .8,
	.380,  .160,  .075,  .04,  .019,
	.007,  .0,

	.003,  .003,  .004,  .006,  .008,
	.009,  .012,  .014,	.017,  .021,
	.025,

	.03,  .035,  .04,  .045,  .048,
	.057,  .063, .07,  .075,  .08,
	.085,  .095, .103, .110,  .12,
	.122,  .12,  .118,  .115,  .12,

	.125,  .130,  .12,  .105,  .09,
	.079,	.067, .057,  .048,  .036,
	.028,

	.023,  .018,  .014,  .011, .010,
	.009,	.007,  .004,  .0,  .0
};


// k_g Spectrum table from pg 130, MI.
static constexpr float k_g_wavelengths_global[4] =
{
	759,  760,  770,  771
};

static constexpr float k_g_amplitudes_global[4] =
{
	0,  3.0,  0.210,  0
};

// k_wa Spectrum table from pg 130, MI.
static constexpr float k_wa_wavelengths_global[13] =
{
	689,  690,  700,  710,  720,  730,  740,
	750,  760,  770,  780,  790,  800
};

static constexpr float k_wa_amplitudes_global[13] =
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
static constexpr float sol_amplitudes_global[38] =
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
	auto i = lower_bound(wavelen_.begin(), wavelen_.end(), wl);
	if(i == wavelen_.begin() || i == wavelen_.end()) return 0.f;
	int index = (i - wavelen_.begin()) - 1;
	float delta = (wl - wavelen_[index]) / (wavelen_[index + 1] - wavelen_[index]);
	return (1.f - delta) * amplitude_[index] + delta * amplitude_[index + 1];
}

inline Rgb computeAttenuatedSunlight_global(float theta, int turbidity)
{
	IrregularSpectrum k_o_curve(k_o_amplitudes_global, k_o_wavelengths_global, 64);
	IrregularSpectrum k_g_curve(k_g_amplitudes_global, k_g_wavelengths_global, 4);
	IrregularSpectrum k_wa_curve(k_wa_amplitudes_global, k_wa_wavelengths_global, 13);
	//RiRegularSpectralCurve   solCurve(solAmplitudes, 380, 750, 38);  // every 10 nm  IN WRONG UNITS
	// Need a factor of 100 (done below)
	float data[38];  // (750 - 380) / 10  + 1

	float beta = 0.04608365822050f * turbidity - 0.04586025928522f;
	float tau_r, tau_a, tau_o, tau_g, tau_wa;
	const float alpha = 1.3f, l_ozone = .35f, w = 2.0f;

	Rgb sun_xyz(0.f);
	float m = 1.f / (math::cos(theta) + 0.000940f * math::pow(1.6386f - theta, -1.253f)); // Relative Optical Mass

	int i;
	float lambda;
	for(i = 0, lambda = 380.f; i < 38; i++, lambda += 10.f)
	{
		const float u_l = lambda * 0.001f;
		// Rayleigh Scattering
		// Results agree with the graph (pg 115, MI) */
		tau_r = math::exp(-m * 0.008735f * math::pow(u_l, -4.08f));
		// Aerosal (water + dust) attenuation
		// beta - amount of aerosols present
		// alpha - ratio of small to large particle sizes. (0:4,usually 1.3)
		// Results agree with the graph (pg 121, MI)
		tau_a = math::exp(-m * beta * math::pow(u_l, -alpha));  // lambda should be in um
		// Attenuation due to ozone absorption
		// lOzone - amount of ozone in cm(NTP)
		// Results agree with the graph (pg 128, MI)
		tau_o = math::exp(-m * k_o_curve.sample(lambda) * l_ozone);
		// Attenuation due to mixed gases absorption
		// Results agree with the graph (pg 131, MI)
		tau_g = math::exp(-1.41f * k_g_curve.sample(lambda) * m / math::pow(1.f + 118.93f * k_g_curve.sample(lambda) * m, 0.45f));
		// Attenuation due to water vapor absorbtion
		// w - precipitable water vapor in centimeters (standard = 2)
		// Results agree with the graph (pg 132, MI)
		tau_wa = math::exp(-0.2385f * k_wa_curve.sample(lambda) * w * m /
						math::pow(1.f + 20.07f * k_wa_curve.sample(lambda) * w * m, 0.45f));

		data[i] = 100.f * sol_amplitudes_global[i] * tau_r * tau_a * tau_o * tau_g * tau_wa; // 100 comes from solCurve being
		// in wrong units.
		sun_xyz += spectrum::wl2Xyz(lambda) * data[i];
	}
	sun_xyz *= 0.02631578947368421053f;
	Rgb sun_col;
	sun_col.set((3.240479f * sun_xyz.r_ - 1.537150f * sun_xyz.g_ - 0.498535f * sun_xyz.b_),
	            (-0.969256f * sun_xyz.r_ + 1.875992f * sun_xyz.g_ + 0.041556f * sun_xyz.b_),
	            (0.055648f * sun_xyz.r_ - 0.204043f * sun_xyz.g_ + 1.057311f * sun_xyz.b_));
	return sun_col;
}

END_YAFARAY

#endif // YAFARAY_SPECTRUM_SUN_H