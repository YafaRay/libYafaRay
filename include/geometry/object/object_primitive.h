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

#include "object_base.h"

namespace yafaray {

/*! simple "container" to handle primitives as objects, for objects that
	consist of just one primitive like spheres etc. */
class PrimitiveObject final : public ObjectBase
{
		using ThisClassType_t = PrimitiveObject; using ParentClassType_t = ObjectBase;

	public:
		PrimitiveObject(ParamError &param_error, const ParamMap &param_map) : ObjectBase{param_error, param_map} { }
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		void setPrimitive(const Primitive *primitive) { primitive_ = primitive; }

	protected:
 		inline static std::string getClassName() { return "PrimitiveObject"; }
		int numPrimitives() const override { return 1; }
		std::vector<const Primitive *> getPrimitives() const override { return {primitive_}; }
		bool calculateObject(const std::unique_ptr<const Material> *material) override { return true; }

	private:
		[[nodiscard]] Type type() const override { return Type::Sphere; }
		const Primitive *primitive_ = nullptr;
};

} //namespace yafaray

#endif //LIBYAFARAY_OBJECT_PRIMITIVE_H
