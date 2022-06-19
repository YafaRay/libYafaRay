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

#include "color/spectrum.h"
#include <algorithm>
#include <vector>

namespace yafaray::spectrum_sun
{

// k_o Spectrum table from pg 127, MI.
static const std::vector<std::pair<float, float>> k_o_wavelength_amplitudes
{
	{300.f, 10.f}, {305.f, 4.8f}, {310.f, 2.7f}, {315.f, 1.35f}, {320.f, 0.8f}, {325.f, 0.38f}, {330.f, 0.16f}, {335.f, 0.075f}, {340.f, 0.04f}, {345.f, 0.019f}, {350.f, 0.007f}, {355.f, 0.f}, {445.f, 0.003f}, {450.f, 0.003f}, {455.f, 0.004f}, {460.f, 0.006f}, {465.f, 0.008f}, {470.f, 0.009f}, {475.f, 0.012f}, {480.f, 0.014f}, {485.f, 0.017f}, {490.f, 0.021f}, {495.f, 0.025f}, {500.f, 0.03f}, {505.f, 0.035f}, {510.f, 0.04f}, {515.f, 0.045f}, {520.f, 0.048f}, {525.f, 0.057f}, {530.f, 0.063f}, {535.f, 0.07f}, {540.f, 0.075f}, {545.f, 0.08f}, {550.f, 0.085f}, {555.f, 0.095f}, {560.f, 0.103f}, {565.f, 0.11f}, {570.f, 0.12f}, {575.f, 0.122f}, {580.f, 0.12f}, {585.f, 0.118f}, {590.f, 0.115f}, {595.f, 0.12f}, {600.f, 0.125f}, {605.f, 0.13f}, {610.f, 0.12f}, {620.f, 0.105f}, {630.f, 0.09f}, {640.f, 0.079f}, {650.f, 0.067f}, {660.f, 0.057f}, {670.f, 0.048f}, {680.f, 0.036f}, {690.f, 0.028f}, {700.f, 0.023f}, {710.f, 0.018f}, {720.f, 0.014f}, {730.f, 0.011f}, {740.f, 0.01f}, {750.f, 0.009f}, {760.f, 0.007f}, {770.f, 0.004f}, {780.f, 0.f}, {790.f, 0.f}
};

// k_g Spectrum table from pg 130, MI.
static const std::vector<std::pair<float, float>> k_g_wavelength_amplitudes
{
	{759.f, 0.f}, {760.f, 3.f}, {770.f, 0.21f}, {771.f, 0.f}
};

// k_wa Spectrum table from pg 130, MI.
static const std::vector<std::pair<float, float>> k_wa_wavelength_amplitudes
{
	{689.f, 0.f}, {690.f, 0.016f}, {700.f, 0.024f}, {710.f, 0.0125f}, {720.f, 1.f}, {730.f, 0.87f}, {740.f, 0.061f}, {750.f, 0.001f}, {760.f, 1e-05f}, {770.f, 1e-05f}, {780.f, 0.0006f}, {790.f, 0.0175f}, {800.f, 0.036f}
};

// 380-750 by 10nm
static constexpr inline std::array<float, 38> sol_amplitudes
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

} //namespace yafaray::spectrum_sun

#endif // YAFARAY_SPECTRUM_SUN_H