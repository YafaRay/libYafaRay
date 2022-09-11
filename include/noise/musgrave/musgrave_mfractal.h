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

#ifndef LIBYAFARAY_MUSGRAVE_MFRACTAL_H
#define LIBYAFARAY_MUSGRAVE_MFRACTAL_H

#include "noise/musgrave.h"

namespace yafaray {

class NoiseGenerator;

class MFractalMusgrave final : public Musgrave
{
	public:
		MFractalMusgrave(float h, float lacu, float octs, const NoiseGenerator *n_gen)
				: h_(h), lacunarity_(lacu), octaves_(octs), n_gen_(n_gen) {}

	private:
		float operator()(const Point3f &pt) const override;

		float h_, lacunarity_, octaves_;
		const NoiseGenerator *n_gen_;
};

} //namespace yafaray

#endif //LIBYAFARAY_MUSGRAVE_MFRACTAL_H
