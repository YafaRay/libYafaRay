/****************************************************************************
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#include "scene/scene_yafaray.h"
#include "common/logger.h"
#include "geometry/triangle.h"
#include "accelerator/accelerator_kdtree.h"
#include "common/param.h"
#include "light/light.h"
#include "material/material.h"
#include "integrator/integrator.h"
#include "texture/texture.h"
#include "background/background.h"
#include "camera/camera.h"
#include "shader/shader_node.h"
#include "render/imagefilm.h"
#include "volume/volume.h"
#include "geometry/primitive_basic.h"
#include "output/output.h"
#include "render/render_data.h"

BEGIN_YAFARAY

#define Y_VERBOSE_SCENE Y_VERBOSE << "Scene (YafaRay): "
#define Y_ERROR_SCENE Y_ERROR << "Scene (YafaRay): "
#define Y_WARN_SCENE Y_WARNING << "Scene (YafaRay): "

#define WARN_EXIST Y_WARN_SCENE << "Sorry, " << pname << " \"" << name << "\" already exists!" << YENDL

#define ERR_NO_TYPE Y_ERROR_SCENE << pname << " type not specified for \"" << name << "\" node!" << YENDL
#define ERR_ON_CREATE(t) Y_ERROR_SCENE << "No " << pname << " could be constructed '" << t << "'!" << YENDL

#define INFO_VERBOSE_SUCCESS(name, t) Y_VERBOSE_SCENE << "Added " << pname << " '"<< name << "' (" << t << ")!" << YENDL
#define INFO_VERBOSE_SUCCESS_DISABLED(name, t) Y_VERBOSE_SCENE << "Added " << pname << " '"<< name << "' (" << t << ")! [DISABLED]" << YENDL

// Object flags
// Lower order byte indicates type
constexpr unsigned int trim__ = 0x0000;
constexpr unsigned int vtrim__ = 0x0001;
constexpr unsigned int mtrim__ = 0x0002;
// Higher order byte indicates options
constexpr unsigned int invisiblem__ = 0x0100;
constexpr unsigned int basemesh__ = 0x0200;


Scene *YafaRayScene::factory(ParamMap &params)
{
	Scene *scene = new YafaRayScene();
	return scene;
}

YafaRayScene::YafaRayScene()
{
	geometry_creation_state_.cur_obj_ = nullptr;
}

YafaRayScene::~YafaRayScene()
{
	clearGeometry();
}

void YafaRayScene::clearGeometry()
{
	if(tree_) { delete tree_; tree_ = nullptr; }
	if(vtree_) { delete vtree_; vtree_ = nullptr; }
	for(auto &m : meshes_)
	{
		if(m.second.type_ == trim__) { delete m.second.obj_; m.second.obj_ = nullptr; }
		else { delete m.second.mobj_; m.second.mobj_ = nullptr; }
	}
	meshes_.clear();
	freeMap(objects_);
	objects_.clear();
}

bool YafaRayScene::startCurveMesh(const std::string &name, int vertices, int obj_pass_index)
{
	if(creation_state_.stack_.front() != CreationState::Geometry) return false;
	int ptype = 0 & 0xFF;

	ObjData &n_obj = meshes_.find(name)->second;

	//TODO: switch?
	// Allocate triangles to render the curve
	n_obj.obj_ = new TriangleObject(2 * (vertices - 1), true, false);
	n_obj.obj_->setObjectIndex(obj_pass_index);
	n_obj.type_ = ptype;
	creation_state_.stack_.push_front(CreationState::Object);
	creation_state_.changes_ |= CreationState::Flags::CGeom;
	geometry_creation_state_.orco_ = false;
	geometry_creation_state_.cur_obj_ = &n_obj;

	n_obj.obj_->points_.reserve(2 * vertices);
	return true;
}

bool YafaRayScene::endCurveMesh(const Material *mat, float strand_start, float strand_end, float strand_shape)
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;

	// TODO: Check if we have at least 2 vertex...
	// TODO: math optimizations

	// extrude vertices and create faces
	std::vector<Point3> &points = geometry_creation_state_.cur_obj_->obj_->points_;
	float r;	//current radius
	int i;
	Point3 o, a, b;
	Vec3 N(0), u(0), v(0);
	int n = points.size();
	// Vertex extruding
	for(i = 0; i < n; i++)
	{
		o = points[i];
		if(strand_shape < 0)
		{
			r = strand_start + math::pow((float)i / (n - 1), 1 + strand_shape) * (strand_end - strand_start);
		}
		else
		{
			r = strand_start + (1 - math::pow(((float)(n - i - 1)) / (n - 1), 1 - strand_shape)) * (strand_end - strand_start);
		}
		// Last point keep previous tangent plane
		if(i < n - 1)
		{
			N = points[i + 1] - points[i];
			N.normalize();
			Vec3::createCs(N, u, v);
		}
		// TODO: thikness?
		a = o - (0.5 * r * v) - 1.5 * r / math::sqrt(3.f) * u;
		b = o - (0.5 * r * v) + 1.5 * r / math::sqrt(3.f) * u;

		geometry_creation_state_.cur_obj_->obj_->points_.push_back(a);
		geometry_creation_state_.cur_obj_->obj_->points_.push_back(b);
	}

	// Face fill
	Triangle tri;
	int a_1, a_2, a_3, b_1, b_2, b_3;
	float su, sv;
	int iu, iv;
	for(i = 0; i < n - 1; i++)
	{
		// 1D particles UV mapping
		su = (float)i / (n - 1);
		sv = su + 1. / (n - 1);
		iu = addUv(su, su);
		iv = addUv(sv, sv);
		a_1 = i;
		a_2 = 2 * i + n;
		a_3 = a_2 + 1;
		b_1 = i + 1;
		b_2 = a_2 + 2;
		b_3 = b_2 + 1;
		// Close bottom
		if(i == 0)
		{
			tri = Triangle(a_1, a_3, a_2, geometry_creation_state_.cur_obj_->obj_);
			tri.setMaterial(mat);
			geometry_creation_state_.cur_tri_ = geometry_creation_state_.cur_obj_->obj_->addTriangle(tri);
			geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
			geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
			geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		}

		// Fill
		tri = Triangle(a_1, b_2, b_1, geometry_creation_state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		geometry_creation_state_.cur_tri_ = geometry_creation_state_.cur_obj_->obj_->addTriangle(tri);
		// StrandUV
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

		tri = Triangle(a_1, a_2, b_2, geometry_creation_state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		geometry_creation_state_.cur_tri_ = geometry_creation_state_.cur_obj_->obj_->addTriangle(tri);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

		tri = Triangle(a_2, b_3, b_2, geometry_creation_state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		geometry_creation_state_.cur_tri_ = geometry_creation_state_.cur_obj_->obj_->addTriangle(tri);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

		tri = Triangle(a_2, a_3, b_3, geometry_creation_state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		geometry_creation_state_.cur_tri_ = geometry_creation_state_.cur_obj_->obj_->addTriangle(tri);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

		tri = Triangle(b_3, a_3, a_1, geometry_creation_state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		geometry_creation_state_.cur_tri_ = geometry_creation_state_.cur_obj_->obj_->addTriangle(tri);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);

		tri = Triangle(b_3, a_1, b_1, geometry_creation_state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		geometry_creation_state_.cur_tri_ = geometry_creation_state_.cur_obj_->obj_->addTriangle(tri);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

	}
	// Close top
	tri = Triangle(i, 2 * i + n, 2 * i + n + 1, geometry_creation_state_.cur_obj_->obj_);
	tri.setMaterial(mat);
	geometry_creation_state_.cur_tri_ = geometry_creation_state_.cur_obj_->obj_->addTriangle(tri);
	geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
	geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
	geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

	geometry_creation_state_.cur_obj_->obj_->finish();

	creation_state_.stack_.pop_front();
	return true;
}

bool YafaRayScene::startTriMesh(const std::string &name, int vertices, int triangles, bool has_orco, bool has_uv, int type, int obj_pass_index)
{
	if(creation_state_.stack_.front() != CreationState::Geometry) return false;
	int ptype = type & 0xFF;
	if(ptype != trim__ && type != vtrim__ && type != mtrim__) return false;

	auto &n_obj = meshes_[name];

	switch(ptype)
	{
		case trim__: n_obj.obj_ = new TriangleObject(triangles, has_uv, has_orco);
			n_obj.obj_->setVisibility(!(type & invisiblem__));
			n_obj.obj_->useAsBaseObject((type & basemesh__));
			n_obj.obj_->setObjectIndex(obj_pass_index);
			break;
		case vtrim__:
		case mtrim__: n_obj.mobj_ = new MeshObject(triangles, has_uv, has_orco);
			n_obj.mobj_->setVisibility(!(type & invisiblem__));
			n_obj.obj_->setObjectIndex(obj_pass_index);
			break;
		default: return false;
	}
	n_obj.type_ = ptype;
	creation_state_.stack_.push_front(CreationState::Object);
	creation_state_.changes_ |= CreationState::Flags::CGeom;
	geometry_creation_state_.orco_ = has_orco;
	geometry_creation_state_.cur_obj_ = &n_obj;

	return true;
}

bool YafaRayScene::endTriMesh()
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;

	if(geometry_creation_state_.cur_obj_->type_ == trim__)
	{
		if(geometry_creation_state_.cur_obj_->obj_->has_uv_)
		{
			if(geometry_creation_state_.cur_obj_->obj_->uv_offsets_.size() != 3 * geometry_creation_state_.cur_obj_->obj_->triangles_.size())
			{
				Y_ERROR << "Scene: UV-offsets mismatch!" << YENDL;
				return false;
			}
		}

		//calculate geometric normals of tris
		geometry_creation_state_.cur_obj_->obj_->finish();
	}
	else
	{
		geometry_creation_state_.cur_obj_->mobj_->finish();
	}

	creation_state_.stack_.pop_front();
	return true;
}

void prepareEdges__(int vertex_common_index, int vertex_1_index, int vertex_2_index, const std::vector<Point3> &vertices, Vec3 &edge_1, Vec3 &edge_2)
{
	edge_1 = vertices[vertex_1_index] - vertices[vertex_common_index];
	edge_2 = vertices[vertex_2_index] - vertices[vertex_common_index];
}

bool YafaRayScene::smoothMesh(const std::string &name, float angle)
{
	if(creation_state_.stack_.front() != CreationState::Geometry) return false;
	ObjData *odat;
	if(!name.empty())
	{
		auto it = meshes_.find(name);
		if(it == meshes_.end()) return false;
		odat = &(it->second);
	}
	else
	{
		odat = geometry_creation_state_.cur_obj_;
		if(!odat) return false;
	}

	if(odat->obj_->normals_exported_ && odat->obj_->points_.size() == odat->obj_->normals_.size())
	{
		odat->obj_->is_smooth_ = true;
		return true;
	}

	// cannot smooth other mesh types yet...
	if(odat->type_ > 0) return false;
	std::vector<Vec3> &normals = odat->obj_->normals_;
	std::vector<Triangle> &triangles = odat->obj_->triangles_;
	size_t points = odat->obj_->points_.size();
	std::vector<Triangle>::iterator tri;
	const std::vector<Point3> &vertices = odat->obj_->points_;

	normals.reserve(points);
	normals.resize(points, {0, 0, 0});

	if(angle >= 180)
	{
		for(tri = triangles.begin(); tri != triangles.end(); ++tri)
		{

			const Vec3 n = tri->getNormal();
			Vec3 e_1, e_2;

			prepareEdges__(tri->pa_, tri->pb_, tri->pc_, vertices, e_1, e_2);
			float alpha = e_1.sinFromVectors(e_2);

			normals[tri->pa_] += n * alpha;

			prepareEdges__(tri->pb_, tri->pa_, tri->pc_, vertices, e_1, e_2);
			alpha = e_1.sinFromVectors(e_2);

			normals[tri->pb_] += n * alpha;

			prepareEdges__(tri->pc_, tri->pa_, tri->pb_, vertices, e_1, e_2);
			alpha = e_1.sinFromVectors(e_2);

			normals[tri->pc_] += n * alpha;

			tri->setNormals(tri->pa_, tri->pb_, tri->pc_);
		}

		for(size_t idx = 0; idx < normals.size(); ++idx) normals[idx].normalize();

	}
	else if(angle > 0.1) // angle dependant smoothing
	{
		float thresh = math::cos(math::degToRad(angle));
		std::vector<Vec3> vnormals;
		std::vector<int> vn_index;
		// create list of faces that include given vertex
		std::vector<std::vector<Triangle *>> vface(points);
		std::vector<std::vector<float>> alphas(points);
		for(tri = triangles.begin(); tri != triangles.end(); ++tri)
		{
			Vec3 e_1, e_2;

			prepareEdges__(tri->pa_, tri->pb_, tri->pc_, vertices, e_1, e_2);
			alphas[tri->pa_].push_back(e_1.sinFromVectors(e_2));
			vface[tri->pa_].push_back(&(*tri));

			prepareEdges__(tri->pb_, tri->pa_, tri->pc_, vertices, e_1, e_2);
			alphas[tri->pb_].push_back(e_1.sinFromVectors(e_2));
			vface[tri->pb_].push_back(&(*tri));

			prepareEdges__(tri->pc_, tri->pa_, tri->pb_, vertices, e_1, e_2);
			alphas[tri->pc_].push_back(e_1.sinFromVectors(e_2));
			vface[tri->pc_].push_back(&(*tri));
		}
		for(int i = 0; i < (int)vface.size(); ++i)
		{
			std::vector<Triangle *> &tris = vface[i];
			int j = 0;
			for(auto fi = tris.begin(); fi != tris.end(); ++fi)
			{
				Triangle *f = *fi;
				bool smooth = false;
				// calculate vertex normal for face
				Vec3 vnorm, fnorm;

				fnorm = f->getNormal();
				vnorm = fnorm * alphas[i][j];
				int k = 0;
				for(auto f_2 = tris.begin(); f_2 != tris.end(); ++f_2)
				{
					if(**fi == **f_2)
					{
						k++;
						continue;
					}
					Vec3 f_2_norm = (*f_2)->getNormal();
					if((fnorm * f_2_norm) > thresh)
					{
						smooth = true;
						vnorm += f_2_norm * alphas[i][k];
					}
					k++;
				}
				int n_idx = -1;
				if(smooth)
				{
					vnorm.normalize();
					//search for existing normal
					for(unsigned int l = 0; l < vnormals.size(); ++l)
					{
						if(vnorm * vnormals[l] > 0.999)
						{
							n_idx = vn_index[l];
							break;
						}
					}
					// create new if none found
					if(n_idx == -1)
					{
						n_idx = normals.size();
						vnormals.push_back(vnorm);
						vn_index.push_back(n_idx);
						normals.push_back(vnorm);
					}
				}
				// set vertex normal to idx
				if(f->pa_ == i) f->na_ = n_idx;
				else if(f->pb_ == i) f->nb_ = n_idx;
				else if(f->pc_ == i) f->nc_ = n_idx;
				else
				{
					Y_ERROR << "Scene: Mesh smoothing error!" << YENDL;
					return false;
				}
				j++;
			}
			vnormals.clear();
			vn_index.clear();
		}
	}

	odat->obj_->is_smooth_ = true;

	return true;
}

int YafaRayScene::addVertex(const Point3 &p)
{
	if(creation_state_.stack_.front() != CreationState::Object) return -1;
	geometry_creation_state_.cur_obj_->obj_->points_.push_back(p);
	if(geometry_creation_state_.cur_obj_->type_ == mtrim__)
	{
		std::vector<Point3> &points = geometry_creation_state_.cur_obj_->mobj_->points_;
		int n = points.size();
		if(n % 3 == 0)
		{
			//convert point 2 to quadratic bezier control point
			points[n - 2] = 2.f * points[n - 2] - 0.5f * (points[n - 3] + points[n - 1]);
		}
		return (n - 1) / 3;
	}

	geometry_creation_state_.cur_obj_->last_vert_id_ = geometry_creation_state_.cur_obj_->obj_->points_.size() - 1;

	return geometry_creation_state_.cur_obj_->last_vert_id_;
}

int YafaRayScene::addVertex(const Point3 &p, const Point3 &orco)
{
	if(creation_state_.stack_.front() != CreationState::Object) return -1;

	switch(geometry_creation_state_.cur_obj_->type_)
	{
		case trim__:
			geometry_creation_state_.cur_obj_->obj_->points_.push_back(p);
			geometry_creation_state_.cur_obj_->obj_->points_.push_back(orco);
			geometry_creation_state_.cur_obj_->last_vert_id_ = (geometry_creation_state_.cur_obj_->obj_->points_.size() - 1) / 2;
			break;

		case vtrim__:
			geometry_creation_state_.cur_obj_->mobj_->points_.push_back(p);
			geometry_creation_state_.cur_obj_->mobj_->points_.push_back(orco);
			geometry_creation_state_.cur_obj_->last_vert_id_ = (geometry_creation_state_.cur_obj_->mobj_->points_.size() - 1) / 2;
			break;

		case mtrim__:
			return addVertex(p);
	}

	return geometry_creation_state_.cur_obj_->last_vert_id_;
}

void YafaRayScene::addNormal(const Vec3 &n)
{
	if(mode_ != 0)
	{
		Y_WARNING << "Normal exporting is only supported for triangle mode" << YENDL;
		return;
	}
	if(geometry_creation_state_.cur_obj_->obj_->points_.size() > geometry_creation_state_.cur_obj_->last_vert_id_ && geometry_creation_state_.cur_obj_->obj_->points_.size() > geometry_creation_state_.cur_obj_->obj_->normals_.size())
	{
		if(geometry_creation_state_.cur_obj_->obj_->normals_.size() < geometry_creation_state_.cur_obj_->obj_->points_.size())
			geometry_creation_state_.cur_obj_->obj_->normals_.resize(geometry_creation_state_.cur_obj_->obj_->points_.size());

		geometry_creation_state_.cur_obj_->obj_->normals_[geometry_creation_state_.cur_obj_->last_vert_id_] = n;
		geometry_creation_state_.cur_obj_->obj_->normals_exported_ = true;
	}
}

bool YafaRayScene::addTriangle(int a, int b, int c, const Material *mat)
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	if(geometry_creation_state_.cur_obj_->type_ == mtrim__)
	{
		BsTriangle tri(3 * a, 3 * b, 3 * c, geometry_creation_state_.cur_obj_->mobj_);
		tri.setMaterial(mat);
		geometry_creation_state_.cur_obj_->mobj_->addBsTriangle(tri);
	}
	else if(geometry_creation_state_.cur_obj_->type_ == vtrim__)
	{
		if(geometry_creation_state_.orco_) a *= 2, b *= 2, c *= 2;
		VTriangle tri(a, b, c, geometry_creation_state_.cur_obj_->mobj_);
		tri.setMaterial(mat);
		geometry_creation_state_.cur_obj_->mobj_->addTriangle(tri);
	}
	else
	{
		if(geometry_creation_state_.orco_) a *= 2, b *= 2, c *= 2;
		Triangle tri(a, b, c, geometry_creation_state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		if(geometry_creation_state_.cur_obj_->obj_->normals_exported_)
		{
			if(geometry_creation_state_.orco_)
			{
				// Since the vertex indexes are duplicated with orco
				// we divide by 2: a / 2 == a >> 1 since is an integer division
				tri.na_ = a >> 1;
				tri.nb_ = b >> 1;
				tri.nc_ = c >> 1;
			}
			else
			{
				tri.na_ = a;
				tri.nb_ = b;
				tri.nc_ = c;
			}
		}
		geometry_creation_state_.cur_tri_ = geometry_creation_state_.cur_obj_->obj_->addTriangle(tri);
	}
	return true;
}

bool YafaRayScene::addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const Material *mat)
{
	if(!addTriangle(a, b, c, mat)) return false;

	if(geometry_creation_state_.cur_obj_->type_ == trim__)
	{
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(uv_a);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(uv_b);
		geometry_creation_state_.cur_obj_->obj_->uv_offsets_.push_back(uv_c);
	}
	else
	{
		geometry_creation_state_.cur_obj_->mobj_->uv_offsets_.push_back(uv_a);
		geometry_creation_state_.cur_obj_->mobj_->uv_offsets_.push_back(uv_b);
		geometry_creation_state_.cur_obj_->mobj_->uv_offsets_.push_back(uv_c);
	}

	return true;
}

int YafaRayScene::addUv(float u, float v)
{
	if(creation_state_.stack_.front() != CreationState::Object) return false;
	if(geometry_creation_state_.cur_obj_->type_ == trim__)
	{
		geometry_creation_state_.cur_obj_->obj_->uv_values_.push_back(Uv(u, v));
		return (int)geometry_creation_state_.cur_obj_->obj_->uv_values_.size() - 1;
	}
	else
	{
		geometry_creation_state_.cur_obj_->mobj_->uv_values_.push_back(Uv(u, v));
		return (int)geometry_creation_state_.cur_obj_->mobj_->uv_values_.size() - 1;
	}
}

ObjectGeometric *YafaRayScene::createObject(const std::string &name, ParamMap &params)
{
       std::string pname = "Object";
       if(objects_.find(name) != objects_.end())
       {
               WARN_EXIST; return nullptr;
       }
       std::string type;
       if(! params.getParam("type", type))
       {
               ERR_NO_TYPE; return nullptr;
       }
       ObjectGeometric *object = ObjectGeometric::factory(params, *this);
       if(object)
       {
               objects_[name] = object;
               INFO_VERBOSE_SUCCESS(name, type);
               return object;
       }
       ERR_ON_CREATE(type);
       return nullptr;
}


TriangleObject *YafaRayScene::getMesh(const std::string &name) const
{
	auto i = meshes_.find(name);
	return (i == meshes_.end()) ? 0 : i->second.obj_;
}

ObjectGeometric *YafaRayScene::getObject(const std::string &name) const
{
	auto i = meshes_.find(name);
	if(i != meshes_.end())
	{
		if(i->second.type_ == trim__) return i->second.obj_;
		else return i->second.mobj_;
	}
	else
	{
		auto oi = objects_.find(name);
		if(oi != objects_.end()) return oi->second;
	}
	return nullptr;
}

bool YafaRayScene::updateGeometry()
{
	if(tree_) delete tree_;
	if(vtree_) delete vtree_;
	tree_ = nullptr, vtree_ = nullptr;
	int nprims = 0;
	if(mode_ == 0)
	{
		for(const auto &m : meshes_)
		{
			if(!m.second.obj_->isVisible()) continue;
			if(m.second.obj_->isBaseObject()) continue;

			if(m.second.type_ == trim__) nprims += m.second.obj_->numPrimitives();
		}
		if(nprims > 0)
		{
			const Triangle **tris = new const Triangle *[nprims];
			const Triangle **insert = tris;
			for(const auto &m : meshes_)
			{
				if(!m.second.obj_->isVisible()) continue;
				if(m.second.obj_->isBaseObject()) continue;

				if(m.second.type_ == trim__) insert += m.second.obj_->getPrimitives(insert);
			}

			ParamMap params;
			params["type"] = std::string("kdtree"); //Do not remove the std::string(), entering directly a string literal can be confused with bool until C++17 new string literals
			params["num_primitives"] = nprims;
			params["depth"] = -1;
			params["leaf_size"] = 1;
			params["cost_ratio"] = 0.8f;
			params["empty_bonus"] = 0.33f;

			tree_ = Accelerator<Triangle>::factory(tris, params);

			delete [] tris;
			scene_bound_ = tree_->getBound();
			Y_VERBOSE << "Scene: New scene bound is:" <<
					  "(" << scene_bound_.a_.x_ << ", " << scene_bound_.a_.y_ << ", " << scene_bound_.a_.z_ << "), (" <<
					  scene_bound_.g_.x_ << ", " << scene_bound_.g_.y_ << ", " << scene_bound_.g_.z_ << ")" << YENDL;

			if(shadow_bias_auto_) shadow_bias_ = YAF_SHADOW_BIAS;
			if(ray_min_dist_auto_) ray_min_dist_ = MIN_RAYDIST;

			Y_INFO << "Scene: total scene dimensions: X=" << scene_bound_.longX() << ", Y=" << scene_bound_.longY() << ", Z=" << scene_bound_.longZ() << ", volume=" << scene_bound_.vol() << ", Shadow Bias=" << shadow_bias_ << (shadow_bias_auto_ ? " (auto)" : "") << ", Ray Min Dist=" << ray_min_dist_ << (ray_min_dist_auto_ ? " (auto)" : "") << YENDL;
		}
		else Y_WARNING << "Scene: Scene is empty..." << YENDL;
	}
	else
	{
		for(const auto &m : meshes_)
		{
			if(m.second.type_ != trim__) nprims += m.second.mobj_->numPrimitives();
		}
		// include all non-mesh objects; eventually make a common map...
		for(const auto &o : objects_)
		{
			nprims += o.second->numPrimitives();
		}
		if(nprims > 0)
		{
			const Primitive **tris = new const Primitive *[nprims];
			const Primitive **insert = tris;
			for(const auto &m : meshes_)
			{
				if(m.second.type_ != trim__) insert += m.second.mobj_->getPrimitives(insert);
			}
			for(const auto &o : objects_)
			{
				insert += o.second->getPrimitives(insert);
			}

			ParamMap params;
			params["type"] = std::string("kdtree"); //Do not remove the std::string(), entering directly a string literal can be confused with bool until C++17 new string literals
			params["num_primitives"] = nprims;
			params["depth"] = -1;
			params["leaf_size"] = 1;
			params["cost_ratio"] = 0.8f;
			params["empty_bonus"] = 0.33f;

			vtree_ = Accelerator<Primitive>::factory(tris, params);

			delete [] tris;
			scene_bound_ = vtree_->getBound();
			Y_VERBOSE << "Scene: New scene bound is:" << YENDL <<
					  "(" << scene_bound_.a_.x_ << ", " << scene_bound_.a_.y_ << ", " << scene_bound_.a_.z_ << "), (" <<
					  scene_bound_.g_.x_ << ", " << scene_bound_.g_.y_ << ", " << scene_bound_.g_.z_ << ")" << YENDL;

			if(shadow_bias_auto_) shadow_bias_ = YAF_SHADOW_BIAS;
			if(ray_min_dist_auto_) ray_min_dist_ = MIN_RAYDIST;

			Y_INFO << "Scene: total scene dimensions: X=" << scene_bound_.longX() << ", Y=" << scene_bound_.longY() << ", Z=" << scene_bound_.longZ() << ", volume=" << scene_bound_.vol() << ", Shadow Bias=" << shadow_bias_ << (shadow_bias_auto_ ? " (auto)" : "") << ", Ray Min Dist=" << ray_min_dist_ << (ray_min_dist_auto_ ? " (auto)" : "") << YENDL;

		}
		else Y_ERROR << "Scene: Scene is empty..." << YENDL;
	}
	return true;
}

bool YafaRayScene::intersect(const Ray &ray, SurfacePoint &sp) const
{
	float dis, z;
	IntersectData data;
	if(ray.tmax_ < 0) dis = std::numeric_limits<float>::infinity();
	else dis = ray.tmax_;
	// intersect with tree:
	if(mode_ == 0)
	{
		if(!tree_) return false;
		Triangle *hitt = nullptr;
		if(!tree_->intersect(ray, dis, &hitt, z, data)) { return false; }
		Point3 h = ray.from_ + z * ray.dir_;
		hitt->getSurface(sp, h, data);
		sp.origin_ = hitt;
		sp.data_ = data;
		sp.ray_ = nullptr;
	}
	else
	{
		if(!vtree_) return false;
		Primitive *hitprim = nullptr;
		if(!vtree_->intersect(ray, dis, &hitprim, z, data)) { return false; }
		Point3 h = ray.from_ + z * ray.dir_;
		hitprim->getSurface(sp, h, data);
		sp.origin_ = hitprim;
		sp.data_ = data;
		sp.ray_ = nullptr;
	}
	ray.tmax_ = z;
	return true;
}

bool YafaRayScene::intersect(const DiffRay &ray, SurfacePoint &sp) const
{
	float dis, z;
	IntersectData data;
	if(ray.tmax_ < 0) dis = std::numeric_limits<float>::infinity();
	else dis = ray.tmax_;
	// intersect with tree:
	if(mode_ == 0)
	{
		if(!tree_) return false;
		Triangle *hitt = nullptr;
		if(!tree_->intersect(ray, dis, &hitt, z, data)) { return false; }
		Point3 h = ray.from_ + z * ray.dir_;
		hitt->getSurface(sp, h, data);
		sp.origin_ = hitt;
		sp.data_ = data;
		sp.ray_ = &ray;
	}
	else
	{
		if(!vtree_) return false;
		Primitive *hitprim = nullptr;
		if(!vtree_->intersect(ray, dis, &hitprim, z, data)) { return false; }
		Point3 h = ray.from_ + z * ray.dir_;
		hitprim->getSurface(sp, h, data);
		sp.origin_ = hitprim;
		sp.data_ = data;
		sp.ray_ = &ray;
	}
	ray.tmax_ = z;
	return true;
}

bool YafaRayScene::isShadowed(RenderData &render_data, const Ray &ray, float &obj_index, float &mat_index) const
{

	Ray sray(ray);
	sray.from_ += sray.dir_ * sray.tmin_;
	sray.time_ = render_data.time_;
	float dis;
	if(ray.tmax_ < 0) dis = std::numeric_limits<float>::infinity();
	else  dis = sray.tmax_ - 2 * sray.tmin_;
	if(mode_ == 0)
	{
		Triangle *hitt = nullptr;
		if(!tree_) return false;
		bool shadowed = tree_->intersectS(sray, dis, &hitt, shadow_bias_);
		if(hitt)
		{
			if(hitt->getMesh()) obj_index = hitt->getMesh()->getAbsObjectIndex();	//Object index of the object casting the shadow
			if(hitt->getMaterial()) mat_index = hitt->getMaterial()->getAbsMaterialIndex();	//Material index of the object casting the shadow
		}
		return shadowed;
	}
	else
	{
		Primitive *hitt = nullptr;
		if(!vtree_) return false;
		bool shadowed = vtree_->intersectS(sray, dis, &hitt, shadow_bias_);
		if(hitt)
		{
			if(hitt->getMaterial()) mat_index = hitt->getMaterial()->getAbsMaterialIndex();	//Material index of the object casting the shadow
		}
		return shadowed;
	}
}

bool YafaRayScene::isShadowed(RenderData &render_data, const Ray &ray, int max_depth, Rgb &filt, float &obj_index, float &mat_index) const
{
	Ray sray(ray);
	sray.from_ += sray.dir_ * sray.tmin_;
	float dis;
	if(ray.tmax_ < 0) dis = std::numeric_limits<float>::infinity();
	else  dis = sray.tmax_ - 2 * sray.tmin_;
	filt = Rgb(1.0);
	void *odat = render_data.arena_;
	alignas (16) unsigned char userdata[Integrator::getUserDataSize()];
	render_data.arena_ = static_cast<void *>(userdata);
	bool isect = false;
	if(mode_ == 0)
	{
		Triangle *hitt = nullptr;
		if(tree_)
		{
			isect = tree_->intersectTs(render_data, sray, max_depth, dis, &hitt, filt, shadow_bias_);
			if(hitt)
			{
				if(hitt->getMesh()) obj_index = hitt->getMesh()->getAbsObjectIndex();	//Object index of the object casting the shadow
				if(hitt->getMaterial()) mat_index = hitt->getMaterial()->getAbsMaterialIndex();	//Material index of the object casting the shadow
			}
		}
	}
	else
	{
		Primitive *hitt = nullptr;
		if(vtree_)
		{
			isect = vtree_->intersectTs(render_data, sray, max_depth, dis, &hitt, filt, shadow_bias_);
			if(hitt)
			{
				if(hitt->getMaterial()) mat_index = hitt->getMaterial()->getAbsMaterialIndex();	//Material index of the object casting the shadow
			}
		}
	}
	render_data.arena_ = odat;
	return isect;
}


bool YafaRayScene::addInstance(const std::string &base_object_name, const Matrix4 &obj_to_world)
{
	if(mode_ != 0) return false;

	if(meshes_.find(base_object_name) == meshes_.end())
	{
		Y_ERROR << "Base mesh for instance doesn't exist " << base_object_name << YENDL;
		return false;
	}

	int id = getNextFreeId();

	if(id > 0)
	{
		const std::string instance_name = base_object_name + "-" + std::to_string(id);
		ObjData &od = meshes_[instance_name];
		ObjData &base = meshes_[base_object_name];

		od.obj_ = new TriangleObjectInstance(base.obj_, obj_to_world);

		return true;
	}
	else
	{
		return false;
	}
}

END_YAFARAY
