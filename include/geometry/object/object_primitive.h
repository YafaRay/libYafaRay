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

#ifndef LIBYAFARAY_OBJECT_PRIMITIVE_H
#define LIBYAFARAY_OBJECT_PRIMITIVE_H

#include "object.h"

namespace yafaray {

/*! simple "container" to handle primitives as objects, for objects that
	consist of just one primitive like spheres etc. */
class PrimitiveObject final : public Object
{
		using ThisClassType_t = PrimitiveObject; using ParentClassType_t = Object;

	public:
		PrimitiveObject(ParamResult &param_result, const ParamMap &param_map, const Items <Object> &objects, const Items<Material> &materials, const Items<Light> &lights);
		[[nodiscard]] std::string exportToString(yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters) const override;
		[[nodiscard]] std::map<std::string, const ParamMeta *> getParamMetaMap() const override { return params_.getParamMetaMap(); }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		void setPrimitive(std::unique_ptr<const Primitive> primitive) { primitive_ = std::move(primitive); }

	protected:
 		inline static std::string getClassName() { return "PrimitiveObject"; }
		std::vector<const Primitive *> getPrimitives() const override { return {primitive_.get()}; }
		bool calculateObject(size_t material_id) override { return true; }

	private:
		[[nodiscard]] Type type() const override { return Type::Sphere; }
		std::unique_ptr<const Primitive> primitive_;
};

} //namespace yafaray

#endif //LIBYAFARAY_OBJECT_PRIMITIVE_H
