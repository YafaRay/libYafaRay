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

#include "geometry/object/object_basic.h"
#include "geometry/primitive/primitive_sphere.h"
#include "common/param.h"

BEGIN_YAFARAY

unsigned int ObjectBasic::object_index_auto_ = 0;
unsigned int ObjectBasic::highest_object_index_ = 1;

ObjectBasic::ObjectBasic()
{
	object_index_auto_++;
	srand(object_index_auto_);
	float r, g, b;
	do
	{
		r = static_cast<float>(rand() % 8) / 8.f;
		g = static_cast<float>(rand() % 8) / 8.f;
		b = static_cast<float>(rand() % 8) / 8.f;
	}
	while(r + g + b < 0.5f);
	object_index_auto_color_ = Rgb(r, g, b);
}

void ObjectBasic::setObjectIndex(unsigned int new_obj_index)
{
	object_index_ = new_obj_index;
	if(highest_object_index_ < object_index_) highest_object_index_ = object_index_;
}

END_YAFARAY
