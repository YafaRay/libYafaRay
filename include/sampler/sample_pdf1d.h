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
		explicit Pdf1D(const std::vector<float> &function) : function_(function) { init(); }
		explicit Pdf1D(std::vector<float> &&function) : function_(std::move(function)) { init(); }
		size_t size() const { return function_.size(); }
		float invSize() const { return inv_size_; }
		float integral() const { return integral_; }
		float invIntegral() const { return inv_integral_; }
		float function(size_t index) const { return function_[index]; }
		float cdf(size_t index) const { return cdf_[index]; }
		float sample(float u, float &pdf) const;
		// take a discrete sample.
		// determines an index in the array from which the CDF was taked from, rather than a sample in [0;1]
		int dSample(float u, float &pdf) const;
		// Distribution1D Data

	private:
		void init();
		static std::pair<float, std::vector<float>> cumulateStep1DDf(const std::vector<float> &function);
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
	std::vector<float> cdf(n_steps);
	const double delta = 1.0 / static_cast<double>(n_steps);
	double c = 0.0;
	for(size_t i = 0; i < n_steps; ++i)
	{
		c += static_cast<double>(function[i]) * delta;
		cdf[i] = static_cast<float>(c);
	}
	const float integral = static_cast<float>(c);// * delta;
	for(auto &cdf_entry : cdf) cdf_entry /= integral;
	return {integral, cdf};
}

inline int Pdf1D::dSample(float u, float &pdf) const
{
	int index;
	if(u <= 0.f)
		index = 0;
	else if(u >= 1.f)
		index = cdf_.size() - 1;
	else
	{
		const auto &it = std::lower_bound(cdf_.begin(), cdf_.end(), u);
		index = static_cast<int>(it - cdf_.begin());
	}
	pdf = function_[index] * inv_integral_;
	return index;
}

inline float Pdf1D::sample(float u, float &pdf) const
{
	if(u <= 0.f)
	{
		constexpr int index = 0;
		pdf = function_[index] * inv_integral_;
		return static_cast<float>(index);
	}
	else if(u >= 1.f)
	{
		const int index = cdf_.size() - 1;
		pdf = function_[index] * inv_integral_;
		return static_cast<float>(index) + 1.f;
	}
	else
	{
		const auto &it = std::lower_bound(cdf_.begin(), cdf_.end(), u);
		const int index = static_cast<int>(it - cdf_.begin());
		pdf = function_[index] * inv_integral_;
		// Return offset along current cdf segment
		const float delta = index > 0 ? (u - cdf_[index - 1]) / (cdf_[index] - cdf_[index - 1]) : u / cdf_[index];
		return static_cast<float>(index) + delta;
	}
}

END_YAFARAY

#endif //YAFARAY_SAMPLE_PDF1D_H
