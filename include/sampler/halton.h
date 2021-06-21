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

#ifndef YAFARAY_HALTON_H
#define YAFARAY_HALTON_H

#include "common/yafaray_common.h"
#include <algorithm>

BEGIN_YAFARAY

// fast incremental Halton sequence generator
// calculation of value must be double prec.
class Halton final
{
	public:
		Halton(int base) { setBase(base); }
		Halton(int base, unsigned int start) : Halton(base) { setStart(start); }
		void setStart(unsigned int start);
		void reset() { value_ = 0.0; }
		float getNext();
		static double lowDiscrepancySampling(int dim, unsigned int n);

	private:
		void setBase(int base);
		unsigned int base_;
		double inv_base_;
		double value_;
};

inline void Halton::setBase(int base)
{
	base_ = base;
	inv_base_ = 1.0 / static_cast<double>(base);
	value_ = 0.0;
}

inline void Halton::setStart(unsigned int start)
{
	double factor = inv_base_;
	value_ = 0.0;
	while(start > 0)
	{
		value_ += static_cast<double>(start % base_) * factor;
		start /= base_;
		factor *= inv_base_;
	}
}

inline float Halton::getNext()
{
	const double r = 0.9999999999 - value_;
	if(inv_base_ < r) value_ += inv_base_;
	else
	{
		double hh = 0.0, h = inv_base_;
		while(h >= r)
		{
			hh = h;
			h *= inv_base_;
		}
		value_ += hh + h - 1.0;
	}
	return std::max(0.f, std::min(1.f, static_cast<float>(value_)));
}

END_YAFARAY

#endif    //YAFARAY_HALTON_H
