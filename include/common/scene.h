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

#ifndef YAFARAY_SCENE_H
#define YAFARAY_SCENE_H

#include "constants.h"
#include "utility/util_thread.h"
#include "common/aa_noise_params.h"

constexpr unsigned int user_data_size__ = 1024;

// Object flags

// Lower order byte indicates type
constexpr unsigned int trim__ = 0x0000;
constexpr unsigned int vtrim__ = 0x0001;
constexpr unsigned int mtrim__ = 0x0002;

// Higher order byte indicates options
constexpr unsigned int invisiblem__ = 0x0100;
constexpr unsigned int basemesh__ = 0x0200;

BEGIN_YAFARAY
class Scene;
class Camera;
class Material;
class ObjectGeometric;
class TriangleObject;
class MeshObject;
class SurfacePoint;
class Ray;
class DiffRay;
class Primitive;
class Triangle;
template<class T> class KdTree;
class Triangle;
class Background;
class Light;
class SurfaceIntegrator;
class VolumeIntegrator;
class ImageFilm;
class Random;
class RenderEnvironment;
class VolumeRegion;
class RenderPasses;
class Matrix4;
class Rgb;
enum IntPassTypes : int;
enum class DarkDetectionType : int;

typedef unsigned int ObjId_t;

/*!
	\var renderState_t::wavelength
		the range is defined going from 400nm (0.0) to 700nm (1.0)
		although the widest range humans can perceive is ofteb given 380-780nm.
*/
struct RenderState
{
	RenderState(): raylevel_(0), current_pass_(0), pixel_sample_(0), ray_division_(1), ray_offset_(0), dc_1_(0), dc_2_(0),
				   traveled_(0.0), chromatic_(true), include_lights_(false), userdata_(nullptr), lightdata_(nullptr), prng_(nullptr) {};
	RenderState(Random *rand): raylevel_(0), current_pass_(0), pixel_sample_(0), ray_division_(1), ray_offset_(0), dc_1_(0), dc_2_(0),
							   traveled_(0.0), chromatic_(true), include_lights_(false), userdata_(nullptr), lightdata_(nullptr), prng_(rand) {};
	~RenderState() {};

	int raylevel_;
	float depth_;
	float contribution_; //?
	const void *skipelement_;
	int current_pass_;
	int pixel_sample_; //!< number of samples inside this pixels so far
	int ray_division_; //!< keep track of trajectory splitting
	int ray_offset_; //!< keep track of trajectory splitting
	float dc_1_, dc_2_; //!< used to decorrelate samples from trajectory splitting
	float traveled_;
	int pixel_number_;
	int thread_id_; //!< identify the current render thread; shall range from 0 to scene_t::getNumThreads() - 1
	unsigned int sampling_offs_; //!< a "noise-like" pixel offset you may use to decorelate sampling of adjacent pixel.
	//point3d_t screenpos; //!< the image coordinates of the pixel being computed currently
	const Camera *cam_;
	bool chromatic_; //!< indicates wether the full spectrum is calculated (true) or only a single wavelength (false).
	bool include_lights_; //!< indicate that emission of materials assiciated to lights shall be included, for correctly visible lights etc.
	float wavelength_; //!< the (normalized) wavelength being used when chromatic is false.
	float time_; //!< the current (normalized) frame time
	mutable void *userdata_; //!< a fixed amount of memory where materials may keep data to avoid recalculations...really need better memory management :(
	void *lightdata_; //!< reserved; non-dirac lights may do some surface-point dependant initializations in the future to reduce redundancy...
	Random *const prng_; //!< a pseudorandom number generator

	//! set some initial values that are always the same before integrating a primary ray
	void setDefaults()
	{
		raylevel_ = 0;
		chromatic_ = true;
		ray_division_ = 1;
		ray_offset_ = 0;
		dc_1_ = dc_2_ = 0.f;
		traveled_ = 0;
	}

	//	protected:
	explicit RenderState(const RenderState &r): prng_(r.prng_) {} //forbiden
};

END_YAFARAY

#include "bound.h"
#include <vector>
#include <map>
#include <list>

#define Y_SIG_ABORT 1
#define Y_SIG_PAUSE 1<<1
#define Y_SIG_STOP  1<<2

BEGIN_YAFARAY

/*! describes an instance of a scene, including all data and functionality to
	create and render a whole scene on the lowest "layer".
	Allocating, configuring and deallocating scene elements etc. however has
	to be performed on the next layer, scene_t only knows the base classes.
	Exception are triangle meshes, which are created by scene_t.
	This implementation currently only supports triangle meshes as geometry,
	for general geometric primitives there will most likely be a separate class
	to keep this one as optimized as possible;
*/
struct ObjData
{
	TriangleObject *obj_;
	MeshObject *mobj_;
	int type_;
	size_t last_vert_id_;
};



struct SceneGeometryState
{
	std::list<unsigned int> stack_;
	unsigned int changes_;
	ObjId_t next_free_id_;
	ObjData *cur_obj_;
	Triangle *cur_tri_;
	bool orco_;
	float smooth_angle_;
};

class LIBYAFARAY_EXPORT Scene
{
	public:
		Scene(const RenderEnvironment *render_environment);
		~Scene();
		explicit Scene(const Scene &s);
		bool render();
		void abort();
		bool startGeometry();
		bool endGeometry();
		bool startTriMesh(ObjId_t id, int vertices, int triangles, bool has_orco, bool has_uv = false, int type = 0, int object_pass_index = 0);
		bool endTriMesh();
		bool startCurveMesh(ObjId_t id, int vertices, int object_pass_index = 0);
		bool endCurveMesh(const Material *mat, float strand_start, float strand_end, float strand_shape);
		int  addVertex(const Point3 &p);
		int  addVertex(const Point3 &p, const Point3 &orco);
		void addNormal(const Vec3 &n);
		bool addTriangle(int a, int b, int c, const Material *mat);
		bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const Material *mat);
		int  addUv(float u, float v);
		bool startVmap(int id, int type, int dimensions);
		bool endVmap();
		bool addVmapValues(float *val);
		bool smoothMesh(ObjId_t id, float angle);
		bool update();

		bool addLight(Light *l);
		bool addMaterial(Material *m, const char *name);
		ObjId_t getNextFreeId();
		bool addObject(ObjectGeometric *obj, ObjId_t &id);
		bool addInstance(ObjId_t base_object_id, const Matrix4 &obj_to_world);
		void addVolumeRegion(VolumeRegion *vr) { volumes_.push_back(vr); }
		void setCamera(Camera *cam);
		void setImageFilm(ImageFilm *film);
		void setBackground(Background *bg);
		void setSurfIntegrator(SurfaceIntegrator *s);
		SurfaceIntegrator *getSurfIntegrator() const { return surf_integrator_; }
		void setVolIntegrator(VolumeIntegrator *v);
		void setAntialiasing(const AaNoiseParams &aa_noise_params) { aa_noise_params_ = aa_noise_params; };
		void setNumThreads(int threads);
		void setNumThreadsPhotons(int threads_photons);
		void setMode(int m) { mode_ = m; }
		Background *getBackground() const;
		TriangleObject *getMesh(ObjId_t id) const;
		ObjectGeometric *getObject(ObjId_t id) const;
		std::vector<VolumeRegion *> getVolumes() const { return volumes_; }
		const Camera *getCamera() const { return camera_; }
		ImageFilm *getImageFilm() const { return image_film_; }
		Bound getSceneBound() const;
		int getNumThreads() const { return nthreads_; }
		int getNumThreadsPhotons() const { return nthreads_photons_; }
		int getSignals() const;
		//! only for backward compatibility!
		AaNoiseParams getAaParameters() const { return aa_noise_params_; }
		bool intersect(const Ray &ray, SurfacePoint &sp) const;
		bool intersect(const DiffRay &ray, SurfacePoint &sp) const;
		bool isShadowed(RenderState &state, const Ray &ray, float &obj_index, float &mat_index) const;
		bool isShadowed(RenderState &state, const Ray &ray, int max_depth, Rgb &filt, float &obj_index, float &mat_index) const;
		const RenderPasses *getRenderPasses() const;
		bool passEnabled(IntPassTypes int_pass_type) const;

		enum SceneState { Ready, Geometry, Object, Vmap };
		enum ChangeFlags { CNone = 0, CGeom = 1, CLight = 1 << 1, COther = 1 << 2,
		                   CAll = CGeom | CLight | COther
		                 };

		std::vector<Light *> lights_;
		VolumeIntegrator *vol_integrator_;

		float shadow_bias_;  //shadow bias to apply to shadows to avoid self-shadow artifacts
		bool shadow_bias_auto_;  //enable automatic shadow bias calculation

		float ray_min_dist_;  //ray minimum distance
		bool ray_min_dist_auto_;  //enable automatic ray minimum distance calculation

	protected:

		SceneGeometryState state_;
		std::map<ObjId_t, ObjectGeometric *> objects_;
		std::map<ObjId_t, ObjData> meshes_;
		std::map< std::string, Material * > materials_;
		std::vector<VolumeRegion *> volumes_;
		Camera *camera_;
		ImageFilm *image_film_;
		KdTree<Triangle> *tree_; //!< kdTree for triangle-only mode
		KdTree<Primitive> *vtree_; //!< kdTree for universal mode
		Background *background_;
		SurfaceIntegrator *surf_integrator_;
		Bound scene_bound_; //!< bounding box of all (finite) scene geometry
		AaNoiseParams aa_noise_params_;
		int nthreads_;
		int nthreads_photons_;
		int mode_; //!< sets the scene mode (triangle-only, virtual primitives)
		int signals_;
		const RenderEnvironment *env_;	//!< reference to the environment to which this scene belongs to
		mutable std::mutex sig_mutex_;
};

END_YAFARAY

#endif // YAFARAY_SCENE_H
