#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      Monte Carlo & Quasi Monte Carlo stuff
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

#ifndef YAFARAY_MCQMC_H
#define YAFARAY_MCQMC_H

BEGIN_YAFRAY
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
			double r = 0.9999999999 - value_;
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


// fast base-2 van der Corput, Sobel, and Larcher & Pillichshammer sequences,
// all from "Efficient Multidimensional Sampling" by Alexander Keller
static constexpr double mult_ratio__ = 0.00000000023283064365386962890625;

inline float riVdC__(unsigned int bits, unsigned int r = 0)
{
	bits = (bits << 16) | (bits >> 16);
	bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
	bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
	bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
	bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);
	return std::max(0.f, std::min(1.f, (float)((double)(bits ^ r) * mult_ratio__)));
}

inline float riS__(unsigned int i, unsigned int r = 0)
{
	for(unsigned int v = 1 << 31; i; i >>= 1, v ^= v >> 1)
		if(i & 1) r ^= v;
	return std::max(0.f, std::min(1.f, ((float)((double) r * mult_ratio__))));
}

inline float riLp__(unsigned int i, unsigned int r = 0)
{
	for(unsigned int v = 1 << 31; i; i >>= 1, v |= v >> 1)
		if(i & 1) r ^= v;
	return std::max(0.f, std::min(1.f, ((float)((double)r * mult_ratio__))));
}


inline int nextPrime__(int last_prime)
{
	int new_prime = last_prime + (last_prime & 1) + 1;
	for(;;)
	{
		int dv = 3;  bool ispr = true;
		while((ispr) && (dv * dv <= new_prime))
		{
			ispr = ((new_prime % dv) != 0);
			dv += 2;
		}
		if(ispr) break;
		new_prime += 2;
	}
	return new_prime;
}

/* The fnv - Fowler/Noll/Vo- hash code
   unrolled for the special case of hashing 32bit unsigned integers
   very easy but fast
   more details on http://www.isthe.com/chongo/tech/comp/fnv/
*/

union Fnv32
{
	unsigned int in_;
	unsigned char out_[4];
};

inline unsigned int fnv32ABuf__(unsigned int value)
{
	constexpr unsigned int fnv_1_32_init = 0x811c9dc5;
	constexpr unsigned int fnv_32_prime = 0x01000193;
	unsigned int hash = fnv_1_32_init;
	Fnv32 val;
	val.in_ = value;

	for(int i = 0; i < 4; i++)
	{
		hash ^= val.out_[i];
		hash *= fnv_32_prime;
	}

	return hash;
}

/* multiply-with-carry generator x(n) = a*x(n-1) + carry mod 2^32.
   period = (a*2^31)-1 */

/* Choose a value for a from this list
   1791398085 1929682203 1683268614 1965537969 1675393560
   1967773755 1517746329 1447497129 1655692410 1606218150
   2051013963 1075433238 1557985959 1781943330 1893513180
   1631296680 2131995753 2083801278 1873196400 1554115554
*/
class Random
{
	public:
		Random() = default;
		Random(unsigned int seed): c_(seed) { }
		double operator()()
		{
			const unsigned int xh = x_ >> 16, xl = x_ & 65535;
			x_ = x_ * y_a_ + c_;
			c_ = xh * y_ah_ + ((xh * y_al_) >> 16) + ((xl * y_ah_) >> 16);
			if(xl * y_al_ >= ~c_ + 1) c_++;
			return (double)x_ * mult_ratio__;
		}
	protected:
		unsigned int x_ = 30903, c_ = 0;
		static constexpr unsigned int y_a_ = 1791398085;
		static constexpr unsigned int y_ah_ = (y_a_ >> 16);
		static constexpr unsigned int y_al_ = y_a_ & 65535;

};

END_YAFRAY

#endif    //YAFARAY_MCQMC_H
