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

#ifndef LIBYAFARAY_MATERIAL_SAMPLE_H
#define LIBYAFARAY_MATERIAL_SAMPLE_H

namespace yafaray {

struct Sample
{
	Sample(float s_1, float s_2, BsdfFlags sflags = BsdfFlags::All, bool revrs = false):
	s_1_(s_1), s_2_(s_2), pdf_(0.f), flags_(sflags), sampled_flags_(BsdfFlags::None), reverse_(revrs) {}
	float s_1_, s_2_;
	float pdf_;
	BsdfFlags flags_, sampled_flags_;
	bool reverse_; //!< if true, the sample method shall return the probability/color for swapped incoming/outgoing dir
	float pdf_back_;
	Rgb col_back_;
};

} //namespace yafaray

#endif //LIBYAFARAY_MATERIAL_SAMPLE_H
