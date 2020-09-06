/****************************************************************************
 *      scene.cc: scene_t controls the rendering of a scene
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

#include "common/scene.h"
#include "common/environment.h"
#include "common/logging.h"
#include "object_geom/object_geom.h"
#include "material/material.h"
#include "light/light.h"
#include "integrator/integrator.h"
#include "common/imagefilm.h"
#include "common/sysinfo.h"
#include "common/triangle.h"
#include "common/kdtree_generic.h"
#include <iostream>
#include <limits>
#include <sstream>

BEGIN_YAFARAY

Scene::Scene(const RenderEnvironment *render_environment): vol_integrator_(nullptr), camera_(nullptr), image_film_(nullptr), tree_(nullptr), vtree_(nullptr), background_(nullptr), surf_integrator_(nullptr), nthreads_(1), nthreads_photons_(1), mode_(1), signals_(0), env_(render_environment)
{
	state_.changes_ = CAll;
	state_.stack_.push_front(Ready);
	state_.next_free_id_ = std::numeric_limits<int>::max();
	state_.cur_obj_ = nullptr;
}

Scene::Scene(const Scene &s)
{
	Y_ERROR << "Scene: You may NOT use the copy constructor!" << YENDL;
}

Scene::~Scene()
{
	if(tree_) delete tree_;
	if(vtree_) delete vtree_;
	for(auto i = meshes_.begin(); i != meshes_.end(); ++i)
	{
		if(i->second.type_ == trim__)
			delete i->second.obj_;
		else
			delete i->second.mobj_;
	}
}

void Scene::abort()
{
	sig_mutex_.lock();
	signals_ |= Y_SIG_ABORT;
	sig_mutex_.unlock();
}

int Scene::getSignals() const
{
	int sig;
	sig_mutex_.lock();
	sig = signals_;
	sig_mutex_.unlock();
	return sig;
}

bool Scene::startGeometry()
{
	if(state_.stack_.front() != Ready) return false;
	state_.stack_.push_front(Geometry);
	return true;
}

bool Scene::endGeometry()
{
	if(state_.stack_.front() != Geometry) return false;
	// in case objects share arrays, so they all need to be updated
	// after each object change, uncomment the below block again:
	// don't forget to update the mesh object iterators!
	/*	for(auto i=meshes.begin();
			 i!=meshes.end(); ++i)
		{
			objData_t &dat = (*i).second;
			dat.obj->setContext(dat.points.begin(), dat.normals.begin() );
		}*/
	state_.stack_.pop_front();
	return true;
}

bool Scene::startCurveMesh(ObjId_t id, int vertices, int obj_pass_index)
{
	if(state_.stack_.front() != Geometry) return false;
	int ptype = 0 & 0xFF;

	ObjData &n_obj = meshes_[id];

	//TODO: switch?
	// Allocate triangles to render the curve
	n_obj.obj_ = new TriangleObject(2 * (vertices - 1), true, false);
	n_obj.obj_->setObjectIndex(obj_pass_index);
	n_obj.type_ = ptype;
	state_.stack_.push_front(Object);
	state_.changes_ |= CGeom;
	state_.orco_ = false;
	state_.cur_obj_ = &n_obj;

	n_obj.obj_->points_.reserve(2 * vertices);
	return true;
}

bool Scene::endCurveMesh(const Material *mat, float strand_start, float strand_end, float strand_shape)
{
	if(state_.stack_.front() != Object) return false;

	// TODO: Check if we have at least 2 vertex...
	// TODO: math optimizations

	// extrude vertices and create faces
	std::vector<Point3> &points = state_.cur_obj_->obj_->points_;
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
			r = strand_start + pow((float)i / (n - 1), 1 + strand_shape) * (strand_end - strand_start);
		}
		else
		{
			r = strand_start + (1 - pow(((float)(n - i - 1)) / (n - 1), 1 - strand_shape)) * (strand_end - strand_start);
		}
		// Last point keep previous tangent plane
		if(i < n - 1)
		{
			N = points[i + 1] - points[i];
			N.normalize();
			createCs__(N, u, v);
		}
		// TODO: thikness?
		a = o - (0.5 * r * v) - 1.5 * r / sqrt(3.f) * u;
		b = o - (0.5 * r * v) + 1.5 * r / sqrt(3.f) * u;

		state_.cur_obj_->obj_->points_.push_back(a);
		state_.cur_obj_->obj_->points_.push_back(b);
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
			tri = Triangle(a_1, a_3, a_2, state_.cur_obj_->obj_);
			tri.setMaterial(mat);
			state_.cur_tri_ = state_.cur_obj_->obj_->addTriangle(tri);
			state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
			state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
			state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		}

		// Fill
		tri = Triangle(a_1, b_2, b_1, state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		state_.cur_tri_ = state_.cur_obj_->obj_->addTriangle(tri);
		// StrandUV
		state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

		tri = Triangle(a_1, a_2, b_2, state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		state_.cur_tri_ = state_.cur_obj_->obj_->addTriangle(tri);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

		tri = Triangle(a_2, b_3, b_2, state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		state_.cur_tri_ = state_.cur_obj_->obj_->addTriangle(tri);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

		tri = Triangle(a_2, a_3, b_3, state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		state_.cur_tri_ = state_.cur_obj_->obj_->addTriangle(tri);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

		tri = Triangle(b_3, a_3, a_1, state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		state_.cur_tri_ = state_.cur_obj_->obj_->addTriangle(tri);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iu);

		tri = Triangle(b_3, a_1, b_1, state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		state_.cur_tri_ = state_.cur_obj_->obj_->addTriangle(tri);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iu);
		state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

	}
	// Close top
	tri = Triangle(i, 2 * i + n, 2 * i + n + 1, state_.cur_obj_->obj_);
	tri.setMaterial(mat);
	state_.cur_tri_ = state_.cur_obj_->obj_->addTriangle(tri);
	state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
	state_.cur_obj_->obj_->uv_offsets_.push_back(iv);
	state_.cur_obj_->obj_->uv_offsets_.push_back(iv);

	state_.cur_obj_->obj_->finish();

	state_.stack_.pop_front();
	return true;
}

bool Scene::startTriMesh(ObjId_t id, int vertices, int triangles, bool has_orco, bool has_uv, int type, int obj_pass_index)
{
	if(state_.stack_.front() != Geometry) return false;
	int ptype = type & 0xFF;
	if(ptype != trim__ && type != vtrim__ && type != mtrim__) return false;

	ObjData &n_obj = meshes_[id];
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
	state_.stack_.push_front(Object);
	state_.changes_ |= CGeom;
	state_.orco_ = has_orco;
	state_.cur_obj_ = &n_obj;

	return true;
}

bool Scene::endTriMesh()
{
	if(state_.stack_.front() != Object) return false;

	if(state_.cur_obj_->type_ == trim__)
	{
		if(state_.cur_obj_->obj_->has_uv_)
		{
			if(state_.cur_obj_->obj_->uv_offsets_.size() != 3 * state_.cur_obj_->obj_->triangles_.size())
			{
				Y_ERROR << "Scene: UV-offsets mismatch!" << YENDL;
				return false;
			}
		}

		//calculate geometric normals of tris
		state_.cur_obj_->obj_->finish();
	}
	else
	{
		state_.cur_obj_->mobj_->finish();
	}

	state_.stack_.pop_front();
	return true;
}

void Scene::setNumThreads(int threads)
{
	nthreads_ = threads;

	if(nthreads_ == -1) //Automatic detection of number of threads supported by this system, taken from Blender. (DT)
	{
		Y_VERBOSE << "Automatic Detection of Threads: Active." << YENDL;
		const SysInfo sys_info;
		nthreads_ = sys_info.getNumSystemThreads();
		Y_VERBOSE << "Number of Threads supported: [" << nthreads_ << "]." << YENDL;
	}
	else
	{
		Y_VERBOSE << "Automatic Detection of Threads: Inactive." << YENDL;
	}

	Y_PARAMS << "Using [" << nthreads_ << "] Threads." << YENDL;

	std::stringstream set;
	set << "CPU threads=" << nthreads_ << std::endl;

	logger__.appendRenderSettings(set.str());
}

void Scene::setNumThreadsPhotons(int threads_photons)
{
	nthreads_photons_ = threads_photons;

	if(nthreads_photons_ == -1) //Automatic detection of number of threads supported by this system, taken from Blender. (DT)
	{
		Y_VERBOSE << "Automatic Detection of Threads for Photon Mapping: Active." << YENDL;
		const SysInfo sys_info;
		nthreads_photons_ = sys_info.getNumSystemThreads();
		Y_VERBOSE << "Number of Threads supported for Photon Mapping: [" << nthreads_photons_ << "]." << YENDL;
	}
	else
	{
		Y_VERBOSE << "Automatic Detection of Threads for Photon Mapping: Inactive." << YENDL;
	}

	Y_PARAMS << "Using for Photon Mapping [" << nthreads_photons_ << "] Threads." << YENDL;
}

#define PREPARE_EDGES(q, v1, v2) e1 = vertices[v1] - vertices[q]; \
			e2 = vertices[v2] - vertices[q];

bool Scene::smoothMesh(ObjId_t id, float angle)
{
	if(state_.stack_.front() != Geometry) return false;
	ObjData *odat;
	if(id)
	{
		auto it = meshes_.find(id);
		if(it == meshes_.end()) return false;
		odat = &(it->second);
	}
	else
	{
		odat = state_.cur_obj_;
		if(!odat) return false;
	}

	if(odat->obj_->normals_exported_ && odat->obj_->points_.size() == odat->obj_->normals_.size())
	{
		odat->obj_->is_smooth_ = true;
		return true;
	}

	// cannot smooth other mesh types yet...
	if(odat->type_ > 0) return false;
	unsigned int idx = 0;
	std::vector<Normal> &normals = odat->obj_->normals_;
	std::vector<Triangle> &triangles = odat->obj_->triangles_;
	size_t points = odat->obj_->points_.size();
	std::vector<Triangle>::iterator tri;
	std::vector<Point3> const &vertices = odat->obj_->points_;

	normals.reserve(points);
	normals.resize(points, Normal(0, 0, 0));

	if(angle >= 180)
	{
		for(tri = triangles.begin(); tri != triangles.end(); ++tri)
		{

			Vec3 n = tri->getNormal();
			Vec3 e1, e2;
			float alpha = 0;

			PREPARE_EDGES(tri->pa_, tri->pb_, tri->pc_)
			alpha = e1.sinFromVectors(e2);

			normals[tri->pa_] += n * alpha;

			PREPARE_EDGES(tri->pb_, tri->pa_, tri->pc_)
			alpha = e1.sinFromVectors(e2);

			normals[tri->pb_] += n * alpha;

			PREPARE_EDGES(tri->pc_, tri->pa_, tri->pb_)
			alpha = e1.sinFromVectors(e2);

			normals[tri->pc_] += n * alpha;

			tri->setNormals(tri->pa_, tri->pb_, tri->pc_);
		}

		for(idx = 0; idx < normals.size(); ++idx) normals[idx].normalize();

	}
	else if(angle > 0.1) // angle dependant smoothing
	{
		float thresh = fCos__(degToRad__(angle));
		std::vector<Vec3> vnormals;
		std::vector<int> vn_index;
		// create list of faces that include given vertex
		std::vector<std::vector<Triangle *>> vface(points);
		std::vector<std::vector<float>> alphas(points);
		for(tri = triangles.begin(); tri != triangles.end(); ++tri)
		{
			Vec3 e1, e2;

			PREPARE_EDGES(tri->pa_, tri->pb_, tri->pc_)
			alphas[tri->pa_].push_back(e1.sinFromVectors(e2));
			vface[tri->pa_].push_back(&(*tri));

			PREPARE_EDGES(tri->pb_, tri->pa_, tri->pc_)
			alphas[tri->pb_].push_back(e1.sinFromVectors(e2));
			vface[tri->pb_].push_back(&(*tri));

			PREPARE_EDGES(tri->pc_, tri->pa_, tri->pb_)
			alphas[tri->pc_].push_back(e1.sinFromVectors(e2));
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
						normals.push_back(Normal(vnorm));
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

int Scene::addVertex(const Point3 &p)
{
	if(state_.stack_.front() != Object) return -1;
	state_.cur_obj_->obj_->points_.push_back(p);
	if(state_.cur_obj_->type_ == mtrim__)
	{
		std::vector<Point3> &points = state_.cur_obj_->mobj_->points_;
		int n = points.size();
		if(n % 3 == 0)
		{
			//convert point 2 to quadratic bezier control point
			points[n - 2] = 2.f * points[n - 2] - 0.5f * (points[n - 3] + points[n - 1]);
		}
		return (n - 1) / 3;
	}

	state_.cur_obj_->last_vert_id_ = state_.cur_obj_->obj_->points_.size() - 1;

	return state_.cur_obj_->last_vert_id_;
}

int Scene::addVertex(const Point3 &p, const Point3 &orco)
{
	if(state_.stack_.front() != Object) return -1;

	switch(state_.cur_obj_->type_)
	{
		case trim__:
			state_.cur_obj_->obj_->points_.push_back(p);
			state_.cur_obj_->obj_->points_.push_back(orco);
			state_.cur_obj_->last_vert_id_ = (state_.cur_obj_->obj_->points_.size() - 1) / 2;
			break;

		case vtrim__:
			state_.cur_obj_->mobj_->points_.push_back(p);
			state_.cur_obj_->mobj_->points_.push_back(orco);
			state_.cur_obj_->last_vert_id_ = (state_.cur_obj_->mobj_->points_.size() - 1) / 2;
			break;

		case mtrim__:
			return addVertex(p);
	}

	return state_.cur_obj_->last_vert_id_;
}

void Scene::addNormal(const Normal &n)
{
	if(mode_ != 0)
	{
		Y_WARNING << "Normal exporting is only supported for triangle mode" << YENDL;
		return;
	}
	if(state_.cur_obj_->obj_->points_.size() > state_.cur_obj_->last_vert_id_ && state_.cur_obj_->obj_->points_.size() > state_.cur_obj_->obj_->normals_.size())
	{
		if(state_.cur_obj_->obj_->normals_.size() < state_.cur_obj_->obj_->points_.size())
			state_.cur_obj_->obj_->normals_.resize(state_.cur_obj_->obj_->points_.size());

		state_.cur_obj_->obj_->normals_[state_.cur_obj_->last_vert_id_] = n;
		state_.cur_obj_->obj_->normals_exported_ = true;
	}
}

bool Scene::addTriangle(int a, int b, int c, const Material *mat)
{
	if(state_.stack_.front() != Object) return false;
	if(state_.cur_obj_->type_ == mtrim__)
	{
		BsTriangle tri(3 * a, 3 * b, 3 * c, state_.cur_obj_->mobj_);
		tri.setMaterial(mat);
		state_.cur_obj_->mobj_->addBsTriangle(tri);
	}
	else if(state_.cur_obj_->type_ == vtrim__)
	{
		if(state_.orco_) a *= 2, b *= 2, c *= 2;
		VTriangle tri(a, b, c, state_.cur_obj_->mobj_);
		tri.setMaterial(mat);
		state_.cur_obj_->mobj_->addTriangle(tri);
	}
	else
	{
		if(state_.orco_) a *= 2, b *= 2, c *= 2;
		Triangle tri(a, b, c, state_.cur_obj_->obj_);
		tri.setMaterial(mat);
		if(state_.cur_obj_->obj_->normals_exported_)
		{
			if(state_.orco_)
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
		state_.cur_tri_ = state_.cur_obj_->obj_->addTriangle(tri);
	}
	return true;
}

bool Scene::addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const Material *mat)
{
	if(!addTriangle(a, b, c, mat)) return false;

	if(state_.cur_obj_->type_ == trim__)
	{
		state_.cur_obj_->obj_->uv_offsets_.push_back(uv_a);
		state_.cur_obj_->obj_->uv_offsets_.push_back(uv_b);
		state_.cur_obj_->obj_->uv_offsets_.push_back(uv_c);
	}
	else
	{
		state_.cur_obj_->mobj_->uv_offsets_.push_back(uv_a);
		state_.cur_obj_->mobj_->uv_offsets_.push_back(uv_b);
		state_.cur_obj_->mobj_->uv_offsets_.push_back(uv_c);
	}

	return true;
}

int Scene::addUv(float u, float v)
{
	if(state_.stack_.front() != Object) return false;
	if(state_.cur_obj_->type_ == trim__)
	{
		state_.cur_obj_->obj_->uv_values_.push_back(Uv(u, v));
		return (int)state_.cur_obj_->obj_->uv_values_.size() - 1;
	}
	else
	{
		state_.cur_obj_->mobj_->uv_values_.push_back(Uv(u, v));
		return (int)state_.cur_obj_->mobj_->uv_values_.size() - 1;
	}
	return -1;
}

bool Scene::addLight(Light *l)
{
	if(l != 0)
	{
		if(!l->lightEnabled()) return false; //if a light is disabled, don't add it to the list of lights
		lights_.push_back(l);
		state_.changes_ |= CLight;
		return true;
	}
	return false;
}

void Scene::setCamera(Camera *cam)
{
	camera_ = cam;
}

void Scene::setImageFilm(ImageFilm *film)
{
	image_film_ = film;
}

void Scene::setBackground(Background *bg)
{
	background_ = bg;
}

void Scene::setSurfIntegrator(SurfaceIntegrator *s)
{
	surf_integrator_ = s;
	surf_integrator_->setScene(this);
	state_.changes_ |= COther;
}

void Scene::setVolIntegrator(VolumeIntegrator *v)
{
	vol_integrator_ = v;
	vol_integrator_->setScene(this);
	state_.changes_ |= COther;
}

Background *Scene::getBackground() const
{
	return background_;
}

TriangleObject *Scene::getMesh(ObjId_t id) const
{
	auto i = meshes_.find(id);
	return (i == meshes_.end()) ? 0 : i->second.obj_;
}

ObjectGeometric *Scene::getObject(ObjId_t id) const
{
	auto i = meshes_.find(id);
	if(i != meshes_.end())
	{
		if(i->second.type_ == trim__) return i->second.obj_;
		else return i->second.mobj_;
	}
	else
	{
		auto oi = objects_.find(id);
		if(oi != objects_.end()) return oi->second;
	}
	return nullptr;
}

Bound Scene::getSceneBound() const
{
	return scene_bound_;
}

/*! update scene state to prepare for rendering.
	\return false if something vital to render the scene is missing
			true otherwise
*/
bool Scene::update()
{
	Y_VERBOSE << "Scene: Mode \"" << ((mode_ == 0) ? "Triangle" : "Universal") << "\"" << YENDL;
	if(!camera_ || !image_film_) return false;
	if(state_.changes_ & CGeom)
	{
		if(tree_) delete tree_;
		if(vtree_) delete vtree_;
		tree_ = nullptr, vtree_ = nullptr;
		int nprims = 0;
		if(mode_ == 0)
		{
			for(auto i = meshes_.begin(); i != meshes_.end(); ++i)
			{
				ObjData &dat = (*i).second;

				if(!dat.obj_->isVisible()) continue;
				if(dat.obj_->isBaseObject()) continue;

				if(dat.type_ == trim__) nprims += dat.obj_->numPrimitives();
			}
			if(nprims > 0)
			{
				const Triangle **tris = new const Triangle *[nprims];
				const Triangle **insert = tris;
				for(auto i = meshes_.begin(); i != meshes_.end(); ++i)
				{
					ObjData &dat = (*i).second;

					if(!dat.obj_->isVisible()) continue;
					if(dat.obj_->isBaseObject()) continue;

					if(dat.type_ == trim__) insert += dat.obj_->getPrimitives(insert);
				}
				tree_ = new TriKdTree(tris, nprims, -1, 1, 0.8, 0.33 /* -1, 1.2, 0.40 */);
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
			for(auto i = meshes_.begin(); i != meshes_.end(); ++i)
			{
				ObjData &dat = (*i).second;
				if(dat.type_ != trim__) nprims += dat.mobj_->numPrimitives();
			}
			// include all non-mesh objects; eventually make a common map...
			for(auto i = objects_.begin(); i != objects_.end(); ++i)
			{
				nprims += i->second->numPrimitives();
			}
			if(nprims > 0)
			{
				const Primitive **tris = new const Primitive *[nprims];
				const Primitive **insert = tris;
				for(auto i = meshes_.begin(); i != meshes_.end(); ++i)
				{
					ObjData &dat = (*i).second;
					if(dat.type_ != trim__) insert += dat.mobj_->getPrimitives(insert);
				}
				for(auto i = objects_.begin(); i != objects_.end(); ++i)
				{
					insert += i->second->getPrimitives(insert);
				}
				vtree_ = new KdTree<Primitive>(tris, nprims, -1, 1, 0.8, 0.33 /* -1, 1.2, 0.40 */);
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
	}

	for(unsigned int i = 0; i < lights_.size(); ++i) lights_[i]->init(*this);

	if(!surf_integrator_)
	{
		Y_ERROR << "Scene: No surface integrator, bailing out..." << YENDL;
		return false;
	}

	if(state_.changes_ != CNone)
	{
		std::stringstream inte_settings;

		bool success = (surf_integrator_->preprocess() && vol_integrator_->preprocess());

		if(!success) return false;
	}

	state_.changes_ = CNone;

	return true;
}

bool Scene::intersect(const Ray &ray, SurfacePoint &sp) const
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

bool Scene::intersect(const DiffRay &ray, SurfacePoint &sp) const
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

bool Scene::isShadowed(RenderState &state, const Ray &ray, float &obj_index, float &mat_index) const
{

	Ray sray(ray);
	sray.from_ += sray.dir_ * sray.tmin_;
	sray.time_ = state.time_;
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

bool Scene::isShadowed(RenderState &state, const Ray &ray, int max_depth, Rgb &filt, float &obj_index, float &mat_index) const
{
	Ray sray(ray);
	sray.from_ += sray.dir_ * sray.tmin_;
	float dis;
	if(ray.tmax_ < 0) dis = std::numeric_limits<float>::infinity();
	else  dis = sray.tmax_ - 2 * sray.tmin_;
	filt = Rgb(1.0);
	void *odat = state.userdata_;
	alignas (16) unsigned char userdata[user_data_size__];
	state.userdata_ = static_cast<void *>(userdata);
	bool isect = false;
	if(mode_ == 0)
	{
		Triangle *hitt = nullptr;
		if(tree_)
		{
			isect = tree_->intersectTs(state, sray, max_depth, dis, &hitt, filt, shadow_bias_);
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
			isect = vtree_->intersectTs(state, sray, max_depth, dis, &hitt, filt, shadow_bias_);
			if(hitt)
			{
				if(hitt->getMaterial()) mat_index = hitt->getMaterial()->getAbsMaterialIndex();	//Material index of the object casting the shadow
			}
		}
	}
	state.userdata_ = odat;
	return isect;
}

bool Scene::render()
{
	sig_mutex_.lock();
	signals_ = 0;
	sig_mutex_.unlock();

	bool success = false;

	const std::map<std::string, Camera *> *camera_table = env_->getCameraTable();

	if(camera_table->size() == 0)
	{
		Y_ERROR << "No cameras/views found, exiting." << YENDL;
		return false;
	}

	for(auto cam_table_entry = camera_table->begin(); cam_table_entry != camera_table->end(); ++cam_table_entry)
	{
		int num_view = distance(camera_table->begin(), cam_table_entry);
		Camera *cam = cam_table_entry->second;
		setCamera(cam);
		if(!update()) return false;

		success = surf_integrator_->render(num_view, image_film_);

		surf_integrator_->cleanup();
		image_film_->flush(num_view);
	}

	return success;
}

//! does not do anything yet...maybe never will
bool Scene::addMaterial(Material *m, const char *name) { return false; }

ObjId_t Scene::getNextFreeId()
{
	ObjId_t id;
	id = state_.next_free_id_;

	//create new entry for object, assert that no ID collision happens:
	if(meshes_.find(id) != meshes_.end())
	{
		Y_ERROR << "Scene: Object ID already in use!" << YENDL;
		--state_.next_free_id_;
		return getNextFreeId();
	}

	--state_.next_free_id_;

	return id;
}

bool Scene::addObject(ObjectGeometric *obj, ObjId_t &id)
{
	id = getNextFreeId();
	if(id > 0)
	{
		//create new triangle object:
		objects_[id] = obj;
		return true;
	}
	else
	{
		return false;
	}
}

bool Scene::addInstance(ObjId_t base_object_id, const Matrix4 &obj_to_world)
{
	if(mode_ != 0) return false;

	if(meshes_.find(base_object_id) == meshes_.end())
	{
		Y_ERROR << "Base mesh for instance doesn't exist " << base_object_id << YENDL;
		return false;
	}

	int id = getNextFreeId();

	if(id > 0)
	{
		ObjData &od = meshes_[id];
		ObjData &base = meshes_[base_object_id];

		od.obj_ = new TriangleObjectInstance(base.obj_, obj_to_world);

		return true;
	}
	else
	{
		return false;
	}
}

const RenderPasses *Scene::getRenderPasses() const { return env_->getRenderPasses(); }
bool Scene::passEnabled(IntPassTypes int_pass_type) const { return env_->getRenderPasses()->passEnabled(int_pass_type); }


END_YAFARAY
