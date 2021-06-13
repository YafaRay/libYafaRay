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

#ifndef YAFARAY_SAMPLE_PDF1D_H
#define YAFARAY_SAMPLE_PDF1D_H

#include "public_api/yafaray_conf.h"
#include "sampler/sample.h"
#include "common/logger.h"
#include <cstring>

BEGIN_YAFARAY

/*! class that holds a 1D probability distribution function (pdf) and is also able to
	take samples from it. In order to do this the cumulative distribution function (cdf)
	is also calculated on construction.
*/

class Pdf1D final
{
	public:
		static void inline cumulateStep1DDf(const float *f, int n_steps, float *integral, float *cdf);
		Pdf1D() = default;
		Pdf1D(float *f, int n);
		float sample(Logger &logger, float u, float *pdf) const;
		// take a discrete sample.
		// determines an index in the array from which the CDF was taked from, rather than a sample in [0;1]
		int dSample(Logger &logger, float u, float *pdf) const;
		// Distribution1D Data

		std::unique_ptr<float[]> func_, cdf_;
		float integral_, inv_integral_, inv_count_;
		int count_;
};

inline Pdf1D::Pdf1D(float *f, int n)
{
	func_ = std::unique_ptr<float[]>(new float[n]);
	cdf_ = std::unique_ptr<float[]>(new float[n + 1]);
	count_ = n;
	memcpy(func_.get(), f, n * sizeof(float));
	cumulateStep1DDf(func_.get(), n, &integral_, cdf_.get());
	inv_integral_ = 1.f / integral_;
	inv_count_ = 1.f / count_;
}

inline void Pdf1D::cumulateStep1DDf(const float *f, int n_steps, float *integral, float *cdf)
{
	double c = 0.0;
	const double delta = 1.0 / static_cast<double>(n_steps);
	cdf[0] = 0.0;
	for(int i = 1; i < n_steps + 1; ++i)
	{
		c += static_cast<double>(f[i - 1]) * delta;
		cdf[i] = static_cast<float>(c);
	}
	*integral = static_cast<float>(c);// * delta;
	for(int i = 1; i < n_steps + 1; ++i)
		cdf[i] /= *integral;
}

inline float Pdf1D::sample(Logger &logger, float u, float *pdf) const
{
	// Find surrounding cdf segments
	const float *ptr = std::lower_bound(cdf_.get(), cdf_.get() + count_ + 1, u);
	int index = static_cast<int>(ptr - cdf_.get() - 1);
	if(index < 0) //Hopefully this should no longer be necessary from now on, as a minimum value slightly over 0.f has been set to the scrHalton function to avoid ptr and cdf to coincide (which caused index = -1)
	{
		logger.logError("Index out of bounds in pdf1D_t::Sample: index, u, ptr, cdf = ", index, ", ", u, ", ", ptr, ", ", cdf_.get());
		index = 0;
	}
	// Return offset along current cdf segment
	const float delta = (u - cdf_[index]) / (cdf_[index + 1] - cdf_[index]);
	if(pdf) *pdf = func_[index] * inv_integral_;
	return index + delta;
}

inline int Pdf1D::dSample(Logger &logger, float u, float *pdf) const
{
	if(u == 0.f)
	{
		*pdf = func_[0] * inv_integral_;
		return 0;
	}
	const float *ptr = std::lower_bound(cdf_.get(), cdf_.get() + count_ + 1, u);
	int index = static_cast<int>(ptr - cdf_.get() - 1);
	if(index < 0)
	{
		logger.logError("Index out of bounds in pdf1D_t::Sample: index, u, ptr, cdf = ", index, ", ", u, ", ", ptr, ", ", cdf_.get());
		index = 0;
	}
	if(pdf) *pdf = func_[index] * inv_integral_;
	return index;
}

END_YAFARAY

#endif //YAFARAY_SAMPLE_PDF1D_H
