#pragma once
/****************************************************************************
 *      curveUtils.h: Curve interpolation utils
 *      This is part of the libYafaRay package
 *      Copyright (C) 2009 Rodrigo Placencia
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

#ifndef YAFARAY_INTERPOLATION_CURVE_H
#define YAFARAY_INTERPOLATION_CURVE_H

#include "common/yafaray_common.h"

BEGIN_YAFARAY

//////////////////////////////////////////////////////////////////////////////
// irregularCurve declaration & definition
//////////////////////////////////////////////////////////////////////////////

class IrregularCurve final
{
	public:
		IrregularCurve(const float *datay, const float *datax, int n) noexcept;
		IrregularCurve(const float *datay, int n) noexcept;
		float getSample(float wl) const noexcept;
		float operator()(float x) const noexcept {return getSample(x);};
		void addSample(float data) noexcept;

	private:
		std::unique_ptr<float[]> c_1_, c_2_;
		int size_;
		int index_;
};

IrregularCurve::IrregularCurve(const float *datay, const float *datax, int n) noexcept : size_(n), index_(0)
{
	c_1_ = std::unique_ptr<float[]>(new float[n]);
	c_2_ = std::unique_ptr<float[]>(new float[n]);
	for(int i = 0; i < n; i++)
	{
		c_1_[i] = datax[i];
		c_2_[i] = datay[i];
	}
}

IrregularCurve::IrregularCurve(const float *datay, int n) noexcept : size_(n), index_(0)
{
	c_1_ = std::unique_ptr<float[]>(new float[n]);
	c_2_ = std::unique_ptr<float[]>(new float[n]);
	for(int i = 0; i < n; i++) c_2_[i] = datay[i];
}

float IrregularCurve::getSample(float x) const noexcept
{
	if(x < c_1_[0] || x > c_1_[size_ - 1]) return 0.0;
	int zero = 0;

	for(int i = 0; i < size_; i++)
	{
		if(c_1_[i] == x) return c_2_[i];
		else if(c_1_[i] <= x && c_1_[i + 1] > x)
		{
			zero = i;
			break;
		}
	}

	float y = x - c_1_[zero];
	y *= (c_2_[zero + 1] - c_2_[zero]) / (c_1_[zero + 1] - c_1_[zero]);
	y += c_2_[zero];
	return y;
}

void IrregularCurve::addSample(float data) noexcept
{
	if(index_ < size_) c_1_[index_++] = data;
}

//////////////////////////////////////////////////////////////////////////////
// regularCurve declaration & definition
//////////////////////////////////////////////////////////////////////////////

class RegularCurve final
{
	public:
		RegularCurve(const float *data, float begin_r, float end_r, int n) noexcept;
		RegularCurve(float begin_r, float end_r, int n) noexcept;
		float getSample(float x) const noexcept;
		float operator()(float x) const noexcept {return getSample(x);};
		void addSample(float data) noexcept;

	private:
		std::vector<float> c_;
		float end_r_;
		float begin_r_;
		float step_ = 0.f;
		size_t index_ = 0;
};

RegularCurve::RegularCurve(const float *data, float begin_r, float end_r, int n) noexcept : end_r_(begin_r), begin_r_(end_r)
{
	c_ = std::vector<float>(n);
	for(int i = 0; i < n; i++) c_[i] = data[i];
	step_ = n / (begin_r_ - end_r_);
}

RegularCurve::RegularCurve(float begin_r, float end_r, int n) noexcept : end_r_(begin_r), begin_r_(end_r)
{
	c_ = std::vector<float>(n);
	step_ = n / (begin_r_ - end_r_);
}

float RegularCurve::getSample(float x) const noexcept
{
	if(x < end_r_ || x > begin_r_) return 0.0;

	const float med = (x - end_r_) * step_;
	const int y_0 = static_cast<int>(floor(med));
	const int y_1 = static_cast<int>(ceil(med));

	if(y_0 == y_1 || y_1 >= static_cast<int>(c_.size())) return c_[y_0];

	const float x_0 = (y_0 / step_) + end_r_;
	const float x_1 = (y_1 / step_) + end_r_;

	float y = x - x_0;
	y *= c_[y_1] - c_[y_0] / (x_1 - x_0);
	y += c_[y_0];

	return y;
}

void RegularCurve::addSample(float data) noexcept
{
	if(index_ < c_.size()) c_[index_++] = data;
}

END_YAFARAY
#endif
