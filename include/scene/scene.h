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
#include "common/thread.h"
#include "common/layers.h"
#include "render/render_view.h"
#include "render/render_control.h"
#include "image/image.h"
#include "common/aa_noise_params.h"
#include "geometry/bound.h"
#include <vector>
#include <map>
#include <list>

BEGIN_YAFARAY

class Light;
class Material;
class VolumeHandler;
class VolumeRegion;
class Texture;
class Camera;
class Background;
class ShaderNode;
class Object;
class ImageFilm;
class Scene;
class ColorOutput;
class ProgressBar;
class Format;
class ParamMap;
class Integrator;
class SurfaceIntegrator;
class VolumeIntegrator;
class SurfacePoint;
class Matrix4;
class Rgb;
class RenderData;
enum class DarkDetectionType : int;

typedef unsigned int ObjId_t;


/*! describes an instance of a scene, including all data and functionality to
	create and render a whole scene on the lowest "layer".
	Allocating, configuring and deallocating scene elements etc. however has
	to be performed on the next layer, scene_t only knows the base classes.
	Exception are triangle meshes, which are created by scene_t.
	This implementation currently only supports triangle meshes as geometry,
	for general geometric primitives there will most likely be a separate class
	to keep this one as optimized as possible;
*/

class LIBYAFARAY_EXPORT Scene
{
	public:
		static Scene *factory(ParamMap &params);
		Scene();
		Scene(const Scene &s) = delete;
		virtual ~Scene();
		virtual int addVertex(const Point3 &p) = 0;
		virtual int addVertex(const Point3 &p, const Point3 &orco) = 0;
		virtual void addNormal(const Vec3 &n) = 0;
		virtual bool addFace(const std::vector<int> &vert_indices, const std::vector<int> &uv_indices = {}) = 0;
		virtual int addUv(float u, float v) = 0;
		virtual bool smoothNormals(const std::string &name, float angle) = 0;
		virtual Object *createObject(const std::string &name, ParamMap &params) = 0;
		virtual bool endObject() = 0;
		virtual bool addInstance(const std::string &base_object_name, const Matrix4 &obj_to_world) = 0;
		virtual bool updateObjects() = 0;
		virtual bool intersect(const Ray &ray, SurfacePoint &sp) const = 0;
		virtual bool intersect(const DiffRay &ray, SurfacePoint &sp) const = 0;
		virtual bool isShadowed(RenderData &render_data, const Ray &ray, float &obj_index, float &mat_index) const = 0;
		virtual bool isShadowed(RenderData &render_data, const Ray &ray, int max_depth, Rgb &filt, float &obj_index, float &mat_index) const = 0;
		virtual Object *getObject(const std::string &name) const = 0;

		ObjId_t getNextFreeId();
		bool startObjects();
		bool endObjects();
		void setBackground(Background *bg);
		void setSurfIntegrator(SurfaceIntegrator *s);
		SurfaceIntegrator *getSurfIntegrator() const { return surf_integrator_; }
		void setVolIntegrator(VolumeIntegrator *v);
		void setAntialiasing(const AaNoiseParams &aa_noise_params) { aa_noise_params_ = aa_noise_params; };
		void setNumThreads(int threads);
		void setNumThreadsPhotons(int threads_photons);
		void setCurrentMaterial(const Material *material);
		const Material *getCurrentMaterial() const { return creation_state_.current_material_; }
		void createDefaultMaterial();
		void clearNonObjects();
		void clearAll();
		bool render();

		Background *getBackground() const;
		const ImageFilm *getImageFilm() const { return image_film_; }
		RenderControl render_control_;
		Bound getSceneBound() const;
		int getNumThreads() const { return nthreads_; }
		int getNumThreadsPhotons() const { return nthreads_photons_; }
		AaNoiseParams getAaParameters() const { return aa_noise_params_; }
		const RenderControl &getRenderControl() const { return render_control_; }
		RenderControl &getRenderControl() { return render_control_; }
		Material *getMaterial(const std::string &name) const;
		Texture *getTexture(const std::string &name) const;
		ShaderNode *getShaderNode(const std::string &name) const;
		Camera *getCamera(const std::string &name) const;
		Light *getLight(const std::string &name) const;
		Background *getBackground(const std::string &name) const;
		Integrator *getIntegrator(const std::string &name) const;
		ColorOutput *getOutput(const std::string &name) const;
		RenderView *getRenderView(const std::string &name) const;
		const std::map<std::string, RenderView *> &getRenderViews() const { return render_views_; }
		const std::map<std::string, VolumeRegion *> &getVolumeRegions() const { return volume_regions_; }
		const std::map<std::string, Light *> &getLights() const { return lights_; }

		Light *createLight(const std::string &name, ParamMap &params);
		Texture *createTexture(const std::string &name, ParamMap &params);
		Material *createMaterial(const std::string &name, ParamMap &params, std::list<ParamMap> &eparams);
		Camera *createCamera(const std::string &name, ParamMap &params);
		Background *createBackground(const std::string &name, ParamMap &params);
		Integrator *createIntegrator(const std::string &name, ParamMap &params);
		ShaderNode *createShaderNode(const std::string &name, ParamMap &params);
		VolumeHandler *createVolumeHandler(const std::string &name, ParamMap &params);
		VolumeRegion *createVolumeRegion(const std::string &name, ParamMap &params);
		RenderView *createRenderView(const std::string &name, ParamMap &params);
		ColorOutput *createOutput(const std::string &name, ParamMap &params); //We do *NOT* have ownership of the outputs, do *NOT* delete them!
		ColorOutput *createOutput(const std::string &name, ColorOutput *output); //We do *NOT* have ownership of the outputs, do *NOT* delete them!
		bool removeOutput(const std::string &name); //Caution: this will delete outputs, only to be called by the client on demand, we do *NOT* have ownership of the outputs
		void clearOutputs();; //Caution: this will delete outputs, only to be called by the client on demand, we do *NOT* have ownership of the outputs
		std::map<std::string, ColorOutput *> &getOutputs() { return outputs_; }
		const std::map<std::string, ColorOutput *> getOutputs() const { return outputs_; }
		bool setupScene(Scene &scene, const ParamMap &params, ProgressBar *pb = nullptr);
		void defineLayer(const ParamMap &params);
		void defineLayer(const Layer::Type &layer_type, const Image::Type &image_type = Image::Type::None, const Image::Type &exported_image_type = Image::Type::None, const std::string &exported_image_name = "");
		void setupLayersParameters(const ParamMap &params);
		void clearLayers();
		void clearRenderViews();
		const Layers &getLayers() const { return layers_; }

		VolumeIntegrator *vol_integrator_ = nullptr;
		float shadow_bias_ = 1.0e-4f;  //shadow bias to apply to shadows to avoid self-shadow artifacts
		bool shadow_bias_auto_ = true;  //enable automatic shadow bias calculation
		float ray_min_dist_ = 1.0e-5f;  //ray minimum distance
		bool ray_min_dist_auto_ = true;  //enable automatic ray minimum distance calculation

	protected:
		template <class T> static void freeMap(std::map< std::string, T * > &map);
		struct CreationState
		{
			enum State { Ready, Geometry, Object };
			enum Flags { CNone = 0, CGeom = 1, CLight = 1 << 1, COther = 1 << 2, CAll = CGeom | CLight | COther };
			std::list<State> stack_;
			unsigned int changes_;
			ObjId_t next_free_id_;
			const Material *current_material_ = nullptr;
		} creation_state_;
		Bound scene_bound_; //!< bounding box of all (finite) scene geometry
		std::map<std::string, Light *> lights_;
		std::map<std::string, Material *> materials_;

	private:
		const Layers getLayersWithImages() const;
		const Layers getLayersWithExportedImages() const;
		template <typename T> static T *findMapItem(const std::string &name, const std::map<std::string, T*> &map);
		void setMaskParams(const ParamMap &params);
		void setEdgeToonParams(const ParamMap &params);
		template <typename T> static T *createMapItem(const std::string &name, const std::string &class_name, T *item, std::map<std::string, T*> &map);
		template <typename T> static T *createMapItem(const std::string &name, const std::string &class_name, ParamMap &params, std::map<std::string, T*> &map, Scene *scene, bool check_type_exists = true);
		void defineBasicLayers();
		void defineDependentLayers(); //!< This function generates the basic/auxiliary layers. Must be called *after* defining all render layers with the defineLayer function.

		AaNoiseParams aa_noise_params_;
		int nthreads_ = 1;
		int nthreads_photons_ = 1;
		ImageFilm *image_film_ = nullptr;
		Background *background_ = nullptr;
		SurfaceIntegrator *surf_integrator_ = nullptr;
		std::map<std::string, Texture *> textures_;
		std::map<std::string, Camera *> cameras_;
		std::map<std::string, Background *> backgrounds_;
		std::map<std::string, Integrator *> integrators_;
		std::map<std::string, ShaderNode *> shaders_;
		std::map<std::string, VolumeHandler *> volume_handlers_;
		std::map<std::string, VolumeRegion *> volume_regions_;
		std::map<std::string, ColorOutput *> outputs_;
		std::map<std::string, RenderView *> render_views_;
		Layers layers_;
};

END_YAFARAY

#endif // YAFARAY_SCENE_H
