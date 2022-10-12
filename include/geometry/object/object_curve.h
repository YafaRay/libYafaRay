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

#ifndef LIBYAFARAY_OBJECT_CURVE_H
#define LIBYAFARAY_OBJECT_CURVE_H

#include "common/logger.h"
#include "object_mesh.h"

namespace yafaray {

template <typename T> struct Uv;
class FacePrimitive;
class Material;

class CurveObject final : public MeshObject
{
		using ThisClassType_t = CurveObject; using ParentClassType_t = MeshObject;

	public:
		inline static std::string getClassName() { return "CurveObject"; }
		static std::pair<std::unique_ptr<Object>, ParamError> factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;
		CurveObject(ParamError &param_error, const ParamMap &param_map);

	private:
		[[nodiscard]] Type type() const override { return Type::Curve; }
		const struct Params
		{
			PARAM_INIT_PARENT(ParentClassType_t);
			PARAM_DECL(float , strand_start_, 0.01f, "strand_start", "");
			PARAM_DECL(float , strand_end_, 0.01f, "strand_end", "");
			PARAM_DECL(float , strand_shape_, 0.f, "strand_shape", "");
		} params_;
		bool calculateObject(const std::unique_ptr<const Material> *material) override;
		virtual int calculateNumFaces() const override { return 2 * (MeshObject::params_.num_vertices_ - 1); }
};

} //namespace yafaray

#endif //LIBYAFARAY_OBJECT_CURVE_H
