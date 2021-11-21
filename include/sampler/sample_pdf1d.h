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

#include "common/yafaray_common.h"
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
		Pdf1D(const std::vector<float> &function) : function_(function) { init(); }
		Pdf1D(std::vector<float> &&function) : function_(std::move(function)) { init(); }
		size_t size() const { return function_.size(); }
		float invSize() const { return inv_size_; }
		float integral() const { return integral_; }
		float invIntegral() const { return inv_integral_; }
		float function(size_t index) const { return function_[index]; }
		float cdf(size_t index) const { return cdf_[index]; }
		float sample(Logger &logger, float u, float &pdf) const;
		// take a discrete sample.
		// determines an index in the array from which the CDF was taked from, rather than a sample in [0;1]
		int dSample(Logger &logger, float u, float &pdf) const;
		// Distribution1D Data

	private:
		void init();
		std::pair<float, std::vector<float>> cumulateStep1DDf(const std::vector<float> &function);
		const std::vector<float> function_;
		std::vector<float> cdf_;
		float integral_, inv_integral_, inv_size_;
};

inline void Pdf1D::init()
{
	std::tie(integral_, cdf_) = cumulateStep1DDf(function_);
	inv_integral_ = 1.f / integral_;
	inv_size_ = 1.f / size();
}

inline std::pair<float, std::vector<float>> Pdf1D::cumulateStep1DDf(const std::vector<float> &function)
{
	const size_t n_steps = function.size();
	std::vector<float> cdf(n_steps + 1);
	const double delta = 1.0 / static_cast<double>(n_steps);
	cdf[0] = 0.f;
	double c = 0.0;
	for(size_t i = 1; i < n_steps + 1; ++i)
	{
		c += static_cast<double>(function[i - 1]) * delta;
		cdf[i] = static_cast<float>(c);
	}
	const float integral = static_cast<float>(c);// * delta;
	for(size_t i = 1; i < n_steps + 1; ++i) cdf[i] /= integral;
	return {integral, cdf};
}

inline float Pdf1D::sample(Logger &logger, float u, float &pdf) const
{
	// Find surrounding cdf segments
	const auto it = std::lower_bound(cdf_.begin(), cdf_.end(), u);
	int index = static_cast<int>(it - cdf_.begin() - 1);
	if(index < 0) //Hopefully this should no longer be necessary from now on, as a minimum value slightly over 0.f has been set to the scrHalton function to avoid ptr and cdf to coincide (which caused index = -1)
	{
		logger.logError("Index out of bounds in pdf1D_t::Sample: index, u, cdf = ", index, ", ", u, ", ", cdf_.data());
		index = 0; //FIXME!
	}
	// Return offset along current cdf segment
	const float delta = (u - cdf_[index]) / (cdf_[index + 1] - cdf_[index]);
	pdf = function_[index] * inv_integral_;
	return index + delta;
}

inline int Pdf1D::dSample(Logger &logger, float u, float &pdf) const
{
	if(u == 0.f)
	{
		pdf = function_[0] * inv_integral_;
		return 0;
	}
	const auto it = std::lower_bound(cdf_.begin(), cdf_.end(), u);
	int index = static_cast<int>(it - cdf_.begin() - 1);
	if(index < 0)
	{
		logger.logError("Index out of bounds in pdf1D_t::Sample: index, u, cdf = ", index, ", ", u, ", ", cdf_.data());
		index = 0; //FIXME!
	}
	pdf = function_[index] * inv_integral_;
	return index;
}

END_YAFARAY

#endif //YAFARAY_SAMPLE_PDF1D_H
