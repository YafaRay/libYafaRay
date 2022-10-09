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

#ifndef LIBYAFARAY_MATERIAL_BSDF_H
#define LIBYAFARAY_MATERIAL_BSDF_H

namespace yafaray {

struct BsdfFlags : Enum<BsdfFlags, unsigned short>
{
	using Enum::Enum; using Enum::operator=;
	enum : ValueType_t
	{
		None		= 0,
		Specular	= 1 << 0,
		Glossy		= 1 << 1,
		Diffuse		= 1 << 2,
		Dispersive	= 1 << 3,
		Reflect		= 1 << 4,
		Transmit	= 1 << 5,
		Filter		= 1 << 6,
		Emit		= 1 << 7,
		Volumetric	= 1 << 8,
		DiffuseReflect = Diffuse | Reflect,
		SpecularReflect = Specular | Reflect,
		SpecularTransmit = Transmit | Filter,
		Translucency = Diffuse | Transmit,// translucency (diffuse transmitt)
		AllSpecular = Specular | Reflect | Transmit,
		AllGlossy = Glossy | Reflect | Transmit,
		All = Specular | Glossy | Diffuse | Dispersive | Reflect | Transmit | Filter
	};
	inline static const EnumMap<ValueType_t> map_{{
			{"None", None, ""},
			{"Specular", Specular, ""},
			{"Glossy", Glossy, ""},
			{"Diffuse", Diffuse, ""},
			{"Dispersive", Dispersive, ""},
			{"Reflect", Reflect, ""},
			{"Transmit", Transmit, ""},
			{"Filter", Filter, ""},
			{"Emit", Emit, ""},
			{"Volumetric", Volumetric, ""},
			{"DiffuseReflect", DiffuseReflect, ""},
			{"SpecularReflect", SpecularReflect, ""},
			{"SpecularTransmit", SpecularTransmit, ""},
			{"Translucency", Translucency, ""},
			{"AllSpecular", AllSpecular, ""},
			{"AllGlossy", AllGlossy, ""},
			{"All", All, ""},
		}};
};


struct DiffuseBrdf : public Enum<DiffuseBrdf>
{
	enum : ValueType_t { Lambertian, OrenNayar };
	inline static const EnumMap<ValueType_t> map_{{
			{"lambertian", Lambertian, ""},
			{"oren_nayar", OrenNayar, ""},
		}};
};

} //namespace yafaray

#endif //LIBYAFARAY_MATERIAL_BSDF_H
