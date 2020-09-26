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

BEGIN_YAFARAY
// fast incremental Halton sequence generator
// calculation of value must be double prec.
class Halton
{
	public:
		Halton() = default;
		Halton(int base)
		{
			setBase(base);
		}

		void setBase(int base)
		{
			base_ = base;
			inv_base_ = 1.0 / (double) base;
			value_ = 0;
		}

		void reset()
		{
			value_ = 0.0;
		}

		inline void setStart(unsigned int i)
		{
			double factor = inv_base_;
			value_ = 0.0;
			while(i > 0)
			{
				value_ += (double)(i % base_) * factor;
				i /= base_;
				factor *= inv_base_;
			}
		}

		inline float getNext()
		{
			const double r = 0.9999999999 - value_;
			if(inv_base_ < r)
			{
				value_ += inv_base_;
			}
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
			return std::max(0.f, std::min(1.f, (float)value_));
		}

	private:
		unsigned int base_;
		double inv_base_;
		double value_;
};

END_YAFARAY

#endif    //YAFARAY_HALTON_H
