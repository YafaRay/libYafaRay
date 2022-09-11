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

#ifndef YAFARAY_NOISE_GENERATOR_H
#define YAFARAY_NOISE_GENERATOR_H

#include "param/class_meta.h"

namespace yafaray {

class NoiseGenerator
{
	public:
		struct NoiseType : public Enum<NoiseType>
		{
			enum : decltype(type()) { Blender, PerlinStandard, PerlinImproved, Cell, VoronoiF1, VoronoiF2, VoronoiF3, VoronoiF4, VoronoiF2F1, VoronoiCrackle };
			inline static const EnumMap<decltype(type())> map_{{
					{"blender", Blender, ""},
					{"stdperlin", PerlinStandard, ""},
					{"newperlin", PerlinImproved, ""},
					{"cellnoise", Cell, ""},
					{"voronoi_f1", VoronoiF1, ""},
					{"voronoi_f2", VoronoiF2, ""},
					{"voronoi_f3", VoronoiF3, ""},
					{"voronoi_f4", VoronoiF4, ""},
					{"voronoi_f2f1", VoronoiF2F1, ""},
					{"voronoi_crackle", VoronoiCrackle, ""},
				}};
		};
		NoiseGenerator() = default;
		virtual ~NoiseGenerator() = default;
		static std::unique_ptr<NoiseGenerator> newNoise(const NoiseType &noise_type);
		virtual float operator()(const Point3f &pt) const = 0;
		virtual Point3f offset(const Point3f &pt) const { return pt; } // offset only added by blendernoise
		static float turbulence(const NoiseGenerator *ngen, const Point3f &pt, int oct, float size, bool hard); // basic turbulence, half amplitude, double frequency defaults. returns value in range (0,1)
		static Rgba cellNoiseColor(const Point3f &pt); // noise cell color (used with voronoi)
		static float getSignedNoise(const NoiseGenerator *n_gen, const Point3f &pt) { return 2.f * (*n_gen)(pt) - 1.f; }

	protected:
		static Point3f hashPnt(const Point3i &point);
		static const std::array<unsigned char, 512> hash_;
		static const std::array<float, 768> hashpntf_;
};

} //namespace yafaray

#endif  //YAFARAY_NOISE_GENERATOR_H
