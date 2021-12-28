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

#ifndef YAFARAY_OBJECT_PRIMITIVE_H
#define YAFARAY_OBJECT_PRIMITIVE_H

#include "object_basic.h"

BEGIN_YAFARAY

/*! simple "container" to handle primitives as objects, for objects that
	consist of just one primitive like spheres etc. */
class PrimitiveObject : public ObjectBasic
{
	public:
		void setPrimitive(const Primitive *primitive) { primitive_ = primitive; }
		virtual int numPrimitives() const override { return 1; }
		virtual const std::vector<const Primitive *> getPrimitives() const override { return {primitive_}; }
		virtual bool calculateObject(const std::unique_ptr<Material> *material) override { return true; }

	private:
		const Primitive *primitive_ = nullptr;
};

END_YAFARAY

#endif //YAFARAY_OBJECT_PRIMITIVE_H
