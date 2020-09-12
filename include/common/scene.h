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
#include "common/renderpasses.h"

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

class Light;
class Material;
class VolumeHandler;
class VolumeRegion;
class Texture;
class Camera;
class Background;
class ShaderNode;
class ObjectGeometric;
class ImageFilm;
class Scene;
class ColorOutput;
class ProgressBar;
class ImageHandler;
class ParamMap;
template<class T> class KdTree;
class Primitive;
class Triangle;
class TriangleObject;
class MeshObject;
class Integrator;
class SurfaceIntegrator;
class VolumeIntegrator;
class SurfacePoint;
class Random;
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
		Scene();
		~Scene();
		Scene(const Scene &s) = delete;
		bool render();
		void abort();
		bool startGeometry();
		bool endGeometry();
		bool startTriMesh(const std::string &name, int vertices, int triangles, bool has_orco, bool has_uv = false, int type = 0, int obj_pass_index = 0);
		bool endTriMesh();
		bool startCurveMesh(const std::string &name, int vertices, int obj_pass_index = 0);
		bool endCurveMesh(const Material *mat, float strand_start, float strand_end, float strand_shape);
		int  addVertex(const Point3 &p);
		int  addVertex(const Point3 &p, const Point3 &orco);
		void addNormal(const Vec3 &n);
		bool addTriangle(int a, int b, int c, const Material *mat);
		bool addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const Material *mat);
		int  addUv(float u, float v);
		bool smoothMesh(const std::string &name, float angle);
		bool update();

		ObjId_t getNextFreeId();
		bool addInstance(const std::string &base_object_name, const Matrix4 &obj_to_world);
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
		TriangleObject *getMesh(const std::string &name) const;
		ObjectGeometric *getObject(const std::string &name) const;
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
		const RenderPasses *getRenderPasses() const { return &render_passes_; }
		bool passEnabled(IntPassTypes int_pass_type) const;

		enum SceneState { Ready, Geometry, Object, Vmap };
		enum ChangeFlags { CNone = 0, CGeom = 1, CLight = 1 << 1, COther = 1 << 2,
		                   CAll = CGeom | CLight | COther
		                 };

		VolumeIntegrator *vol_integrator_;

		float shadow_bias_;  //shadow bias to apply to shadows to avoid self-shadow artifacts
		bool shadow_bias_auto_;  //enable automatic shadow bias calculation

		float ray_min_dist_;  //ray minimum distance
		bool ray_min_dist_auto_;  //enable automatic ray minimum distance calculation

		Material *getMaterial(const std::string &name) const;
		Texture *getTexture(const std::string &name) const;
		ShaderNode *getShaderNode(const std::string &name) const;
		Camera *getCamera(const std::string &name) const;
		Background 	*getBackground(const std::string &name) const;
		Integrator 	*getIntegrator(const std::string &name) const;
		const std::map<std::string, VolumeRegion *> &getVolumeRegions() const { return volume_regions_; }
		const std::map<std::string, Light *> &getLights() const { return lights_; }
		const std::vector<Light *> getLightsVisible() const;;
		const std::vector<Light *> getLightsEmittingCausticPhotons() const;;
		const std::vector<Light *> getLightsEmittingDiffusePhotons() const;;

		Light 		*createLight(const std::string &name, ParamMap &params);
		Texture 		*createTexture(const std::string &name, ParamMap &params);
		Material 	*createMaterial(const std::string &name, ParamMap &params, std::list<ParamMap> &eparams);
		ObjectGeometric 	*createObject(const std::string &name, ParamMap &params);
		Camera 		*createCamera(const std::string &name, ParamMap &params);
		Background 	*createBackground(const std::string &name, ParamMap &params);
		Integrator 	*createIntegrator(const std::string &name, ParamMap &params);
		ShaderNode 	*createShaderNode(const std::string &name, ParamMap &params);
		VolumeHandler *createVolumeH(const std::string &name, const ParamMap &params);
		VolumeRegion	*createVolumeRegion(const std::string &name, ParamMap &params);
		ImageFilm	*createImageFilm(const ParamMap &params, ColorOutput &output);
		ImageHandler *createImageHandler(const std::string &name, ParamMap &params);

		bool			setupScene(Scene &scene, const ParamMap &params, ColorOutput &output, ProgressBar *pb = nullptr);
		void			setupRenderPasses(const ParamMap &params);
		void			setupLoggingAndBadge(const ParamMap &params);
		const 			std::map<std::string, Camera *> *getCameraTable() const { return &cameras_; }
		void			setOutput2(ColorOutput *out_2) { output_2_ = out_2; }
		ColorOutput	*getOutput2() const { return output_2_; }

		void clearAll();

	protected:
		template <class T> static void freeMap(std::map< std::string, T * > &map);

		SceneGeometryState state_;
		Camera *camera_ = nullptr;
		ImageFilm *image_film_ = nullptr;
		KdTree<Triangle> *tree_ = nullptr; //!< kdTree for triangle-only mode
		KdTree<Primitive> *vtree_ = nullptr; //!< kdTree for universal mode
		Background *background_ = nullptr;
		SurfaceIntegrator *surf_integrator_ = nullptr;
		Bound scene_bound_; //!< bounding box of all (finite) scene geometry
		AaNoiseParams aa_noise_params_;
		int nthreads_ = 1;
		int nthreads_photons_ = 1;
		int mode_ = 0; //!< sets the scene mode (0=triangle-only, 1=virtual primitives)
		int signals_ = 0;

		std::map<std::string, Light *> 	lights_;
		std::map<std::string, Material *> 	materials_;
		std::map<std::string, Texture *> 	textures_;
		std::map<std::string, ObjectGeometric *> 	objects_;
		std::map<std::string, ObjData> 	meshes_;
		std::map<std::string, Camera *> 	cameras_;
		std::map<std::string, Background *> backgrounds_;
		std::map<std::string, Integrator *> integrators_;
		std::map<std::string, ShaderNode *> shaders_;
		std::map<std::string, VolumeHandler *> volume_handlers_;
		std::map<std::string, VolumeRegion *> volume_regions_;
		std::map<std::string, ImageHandler *> imagehandlers_;

		RenderPasses render_passes_;
		ColorOutput *output_2_ = nullptr; //secondary color output to export to file at the same time it's exported to Blender

		mutable std::mutex sig_mutex_;
};

END_YAFARAY

#endif // YAFARAY_SCENE_H
