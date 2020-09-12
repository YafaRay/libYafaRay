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

#include "object_geom/object_geom_mesh.h"
#include "object_geom/primitive_basic.h"
#include "common/triangle.h"
#include "common/param.h"
#include <cstdlib>

BEGIN_YAFARAY

float ObjectGeometric::highest_object_index_ = 1.f;	//Initially this class shared variable will be 1.f
unsigned int ObjectGeometric::object_index_auto_ = 0;	//Initially this class shared variable will be 0

ObjectGeometric *ObjectGeometric::factory(ParamMap &params, Scene &scene)
{
	std::string type;
	params.getParam("type", type);
	if(type == "sphere") return sphereFactory__(params, scene);
	else return nullptr;
}

ObjectGeometric::ObjectGeometric() : light_(nullptr), visible_(true), is_base_mesh_(false), object_index_(0.f)
{
	object_index_auto_++;
	srand(object_index_auto_);
	float r, g, b;
	do
	{
		r = (float)(rand() % 8) / 8.f;
		g = (float)(rand() % 8) / 8.f;
		b = (float)(rand() % 8) / 8.f;
	}
	while(r + g + b < 0.5f);
	object_index_auto_color_ = Rgb(r, g, b);
	object_index_auto_number_ = object_index_auto_;
}
void ObjectGeometric::setObjectIndex(const float &new_obj_index) {
	object_index_ = new_obj_index;
	if(highest_object_index_ < object_index_) highest_object_index_ = object_index_;
}

TriangleObject::TriangleObject(int ntris, bool has_uv, bool has_orco):
		has_orco_(has_orco), has_uv_(has_uv), is_smooth_(false), normals_exported_(false)
{
	triangles_.reserve(ntris);
	if(has_uv)
	{
		uv_offsets_.reserve(ntris);
	}

	if(has_orco)
	{
		points_.reserve(2 * 3 * ntris);
	}
	else
	{
		points_.reserve(3 * ntris);
	}
}

int TriangleObject::getPrimitives(const Triangle **prims) const
{
	for(unsigned int i = 0; i < triangles_.size(); ++i)
	{
		prims[i] = &(triangles_[i]);
	}
	return triangles_.size();
}

Triangle *TriangleObject::addTriangle(const Triangle &t)
{
	triangles_.push_back(t);
	triangles_.back().self_index_ = triangles_.size() - 1;
	return &(triangles_.back());
}

void TriangleObject::finish()
{
	for(auto i = triangles_.begin(); i != triangles_.end(); ++i)
	{
		i->recNormal();
	}
}

// triangleObjectInstance_t Methods

TriangleObjectInstance::TriangleObjectInstance(TriangleObject *base, Matrix4 obj_2_world)
{
	obj_to_world_ = obj_2_world;
	m_base_ = base;
	has_orco_ = m_base_->has_orco_;
	has_uv_ = m_base_->has_uv_;
	is_smooth_ = m_base_->is_smooth_;
	normals_exported_ = m_base_->normals_exported_;
	visible_ = true;
	is_base_mesh_ = false;

	triangles_.reserve(m_base_->triangles_.size());

	for(size_t i = 0; i < m_base_->triangles_.size(); i++)
	{
		triangles_.push_back(TriangleInstance(&m_base_->triangles_[i], this));
	}
}

int TriangleObjectInstance::getPrimitives(const Triangle **prims) const
{
	for(size_t i = 0; i < triangles_.size(); i++)
	{
		prims[i] = &triangles_[i];
	}
	return triangles_.size();
}

void TriangleObjectInstance::finish()
{
	// Empty
}

/*===================
	meshObject_t methods
=====================================*/

MeshObject::MeshObject(int ntris, bool has_uv, bool has_orco):
		has_orco_(has_orco), has_uv_(has_uv), is_smooth_(false), light_(nullptr)
{
	//triangles.reserve(ntris);
	if(has_uv)
	{
		uv_offsets_.reserve(ntris);
	}
}

int MeshObject::getPrimitives(const Primitive **prims) const
{
	int n = 0;
	for(unsigned int i = 0; i < triangles_.size(); ++i, ++n)
	{
		prims[n] = &(triangles_[i]);
	}
	for(unsigned int i = 0; i < s_triangles_.size(); ++i, ++n)
	{
		prims[n] = &(s_triangles_[i]);
	}
	return n;
}

Primitive *MeshObject::addTriangle(const VTriangle &t)
{
	triangles_.push_back(t);
	return &(triangles_.back());
}

Primitive *MeshObject::addBsTriangle(const BsTriangle &t)
{
	s_triangles_.push_back(t);
	return &(triangles_.back());
}

void MeshObject::finish()
{
	for(auto i = triangles_.begin(); i != triangles_.end(); ++i)
	{
		i->recNormal();
	}
}

END_YAFARAY
