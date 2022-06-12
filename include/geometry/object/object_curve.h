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

#ifndef YAFARAY_OBJECT_CURVE_H
#define YAFARAY_OBJECT_CURVE_H

#include "common/logger.h"
#include "object_mesh.h"

BEGIN_YAFARAY

template <typename T> struct Uv;
class FacePrimitive;
class Material;

class CurveObject final : public MeshObject
{
	public:
		static Object *factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		CurveObject(int num_vertices, float strand_start, float strand_end, float strand_shape, bool has_uv, bool has_orco, bool motion_blur_bezier, float time_range_start, float time_range_end);
		bool calculateObject(const std::unique_ptr<const Material> *material) override;

	private:
		float strand_start_ = 0.01f;
		float strand_end_ = 0.01f;
		float strand_shape_ = 0.f;
};

END_YAFARAY

#endif //YAFARAY_OBJECT_CURVE_H
