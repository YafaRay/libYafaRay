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

#include "common/aa_noise_params.h"
#include "common/layers.h"
#include "common/mask_edge_toon_params.h"
#include "geometry/object/object.h"
#include "geometry/object/object_instance.h"
#include "image/image.h"
#include "render/render_callbacks.h"
#include "render/render_control.h"
#include "render/render_view.h"
#include <list>
#include <map>
#include <vector>

namespace yafaray {

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
class ImageOutput;
class ProgressBar;
class Format;
class ParamMap;
class Integrator;
class SurfaceIntegrator;
class VolumeIntegrator;
class SurfacePoint;
template <typename T, size_t N> class SquareMatrix;
typedef SquareMatrix<float, 4> Matrix4f;
class Rgb;
class Accelerator;
template <typename T, size_t N> class Point;
typedef Point<float, 3> Point3f;
template <typename T, size_t N> class Vec;
typedef Vec<float, 3> Vec3f;
template<typename T> class Bound;
enum class DarkDetectionType : unsigned char;

typedef unsigned int ObjId_t;

class Scene final
{
	public:
		explicit Scene(Logger &logger);
		Scene(const Scene &s) = delete;
		~Scene();
		int addVertex(Point3f &&p, int time_step);
		int addVertex(Point3f &&p, Point3f &&orco, int time_step);
		void addVertexNormal(Vec3f &&n, int time_step);
		bool addFace(std::vector<int> &&vert_indices, std::vector<int> &&uv_indices = {});
		int addUv(Uv<float> &&uv);
		bool smoothVerticesNormals(std::string &&name, float angle);
		Object *createObject(std::string &&name, ParamMap &&params);
		bool endObject();
		int createInstance();
		bool addInstanceObject(int instance_id, std::string &&base_object_name);
		bool addInstanceOfInstance(int instance_id, size_t base_instance_id);
		bool addInstanceMatrix(int instance_id, Matrix4f &&obj_to_world, float time);
		bool updateObjects();
		Object *getObject(const std::string &name) const;
		const Accelerator *getAccelerator() const { return accelerator_.get(); }

		ObjId_t getNextFreeId();
		bool startObjects();
		bool endObjects();
		void setAntialiasing(AaNoiseParams &&aa_noise_params) { aa_noise_params_ = std::move(aa_noise_params); };
		void setNumThreads(int threads);
		void setNumThreadsPhotons(int threads_photons);
		void setCurrentMaterial(const std::unique_ptr<const Material> *material);
		void createDefaultMaterial();
		void clearNonObjects();
		void clearAll();
		bool render();

		const Background *getBackground() const;
		const ImageFilm *getImageFilm() const { return image_film_.get(); }
		Bound<float> getSceneBound() const;
		int getNumThreads() const { return nthreads_; }
		int getNumThreadsPhotons() const { return nthreads_photons_; }
		AaNoiseParams getAaParameters() const { return aa_noise_params_; }
		RenderControl &getRenderControl() { return render_control_; }
		const RenderControl &getRenderControl() const { return render_control_; }
		const std::unique_ptr<const Material> * getMaterial(const std::string &name) const;
		Texture *getTexture(const std::string &name) const;
		const Camera * getCamera(const std::string &name) const;
		Light *getLight(const std::string &name) const;
		const Background * getBackground(const std::string &name) const;
		Integrator *getIntegrator(const std::string &name) const;
		ImageOutput *getOutput(const std::string &name) const;
		std::shared_ptr<Image> getImage(const std::string &name) const;
		const std::map<std::string, std::unique_ptr<RenderView>> &getRenderViews() const { return render_views_; }
		const std::map<std::string, std::unique_ptr<VolumeRegion>> * getVolumeRegions() const { return &volume_regions_; }
		std::map<std::string, Light *> getLights() const;

		Light *createLight(std::string &&name, ParamMap &&params);
		Texture *createTexture(std::string &&name, ParamMap &&params);
		std::unique_ptr<const Material> *createMaterial(std::string &&name, ParamMap &&params, std::list<ParamMap> &&nodes_params);
		const Camera *createCamera(std::string &&name, ParamMap &&params);
		const Background *defineBackground(ParamMap &&params);
		SurfaceIntegrator *defineSurfaceIntegrator(ParamMap &&params);
		VolumeIntegrator *defineVolumeIntegrator(ParamMap &&params);
		VolumeRegion *createVolumeRegion(std::string &&name, ParamMap &&params);
		RenderView *createRenderView(std::string &&name, ParamMap &&params);
		std::shared_ptr<Image> createImage(std::string &&name, ParamMap &&params);
		ImageOutput *createOutput(std::string &&name, ParamMap &&params);
		bool removeOutput(std::string &&name);
		void clearOutputs();
		std::map<std::string, std::unique_ptr<ImageOutput>> &getOutputs() { return outputs_; }
		const std::map<std::string, std::unique_ptr<ImageOutput>> &getOutputs() const { return outputs_; }
		bool setupSceneRenderParams(Scene &scene, ParamMap &&params);
		bool setupSceneProgressBar(Scene &scene, std::shared_ptr<ProgressBar> &&progress_bar);
		void defineLayer(ParamMap &&params);
		void defineLayer(std::string &&layer_type_name, std::string &&image_type_name, std::string &&exported_image_type_name, std::string &&exported_image_name);
		void defineLayer(LayerDef::Type layer_type, Image::Type image_type = Image::Type::None, Image::Type exported_image_type = Image::Type::None, const std::string &exported_image_name = "");
		void clearLayers();
		const Layers * getLayers() const { return &layers_; }
		float getShadowBias() const { return shadow_bias_; }
		bool isShadowBiasAuto() const { return shadow_bias_auto_; }
		float getRayMinDist() const { return ray_min_dist_; }
		bool isRayMinDistAuto() const { return ray_min_dist_auto_; }
		const VolumeIntegrator *getVolIntegrator() const { return vol_integrator_.get(); }

		unsigned int getMaterialIndexHighest() const { return material_index_highest_; }
		unsigned int getMaterialIndexAuto() const { return material_index_auto_; }
		unsigned int getObjectIndexHighest() const { return object_index_highest_; }
		unsigned int getObjectIndexAuto() const { return object_index_auto_; }

		static void logWarnExist(Logger &logger, const std::string &pname, const std::string &name);
		static void logErrNoType(Logger &logger, const std::string &pname, const std::string &name, const std::string &type);
		static void logErrOnCreate(Logger &logger, const std::string &pname, const std::string &name, const std::string &type);
		static void logInfoVerboseSuccess(Logger &logger, const std::string &pname, const std::string &name, const std::string &t);
		static void logInfoVerboseSuccessDisabled(Logger &logger, const std::string &pname, const std::string &name, const std::string &t);

		const MaskParams &getMaskParams() const { return mask_params_; }
		void setMaskParams(const MaskParams &mask_params) { mask_params_ = mask_params; }
		const EdgeToonParams &getEdgeToonParams() const { return edge_toon_params_; }
		void setEdgeToonParams(const EdgeToonParams &edge_toon_params) { edge_toon_params_ = edge_toon_params; }

		void setRenderNotifyViewCallback(yafaray_RenderNotifyViewCallback_t callback, void *callback_data);
		void setRenderNotifyLayerCallback(yafaray_RenderNotifyLayerCallback_t callback, void *callback_data);
		void setRenderPutPixelCallback(yafaray_RenderPutPixelCallback_t callback, void *callback_data);
		void setRenderHighlightPixelCallback(yafaray_RenderHighlightPixelCallback_t callback, void *callback_data);
		void setRenderFlushAreaCallback(yafaray_RenderFlushAreaCallback_t callback, void *callback_data);
		void setRenderFlushCallback(yafaray_RenderFlushCallback_t callback, void *callback_data);
		void setRenderHighlightAreaCallback(yafaray_RenderHighlightAreaCallback_t callback, void *callback_data);
		const RenderCallbacks &getRenderCallbacks() const { return render_callbacks_; }

	private:
		template <typename T> static T *findMapItem(const std::string &name, const std::map<std::string, std::unique_ptr<T>> &map);
		template <typename T> static std::shared_ptr<T> findMapItem(const std::string &name, const std::map<std::string, std::shared_ptr<T>> &map);
		template <typename T> std::unique_ptr<T> defineItem(ParamMap &&params, std::string &&name, std::string &&class_name);
		template <typename T> std::unique_ptr<T> defineIntegrator(ParamMap &&params, std::string &&name, std::string &&class_name);
		void setMaskParams(const ParamMap &params);
		void setEdgeToonParams(const ParamMap &params);
		template <typename T> static T *createMapItem(Logger &logger, std::string &&name, std::string &&class_name, ParamMap &&params, std::map<std::string, std::unique_ptr<T>> &map, const Scene *scene, bool check_type_exists = true);
		template <typename T> static std::shared_ptr<T> createMapItem(Logger &logger, std::string &&name, std::string &&class_name, ParamMap &&params, std::map<std::string, std::shared_ptr<T>> &map, const Scene *scene, bool check_type_exists = true);
		void defineBasicLayers();
		void defineDependentLayers(); //!< This function generates the basic/auxiliary layers. Must be called *after* defining all render layers with the defineLayer function.

		struct CreationState
		{
			enum State { Ready, Geometry, Object };
			enum Flags { CNone = 0, CGeom = 1, CLight = 1 << 1, CMaterial = 1 << 2, COther = 1 << 3, CAll = CGeom | CLight | CMaterial | COther };
			std::list<State> stack_;
			unsigned int changes_;
			ObjId_t next_free_id_;
			const std::unique_ptr<const Material> *current_material_ = nullptr;
		} creation_state_;
		std::unique_ptr<Bound<float>> scene_bound_; //!< bounding box of all (finite) scene geometry
		std::string scene_accelerator_;
		std::unique_ptr<const Accelerator> accelerator_;
		Object *current_object_ = nullptr;
		std::map<std::string, std::unique_ptr<Object>> objects_;
		std::vector<std::unique_ptr<ObjectInstance>> instances_;
		std::map<std::string, std::unique_ptr<Light>> lights_;
		std::map<std::string, std::unique_ptr<std::unique_ptr<const Material>>> materials_;
		RenderControl render_control_;
		Logger &logger_;

		AaNoiseParams aa_noise_params_;
		int nthreads_ = 1;
		int nthreads_photons_ = 1;
		float shadow_bias_ = 1.0e-4f;  //shadow bias to apply to shadows to avoid self-shadow artifacts
		bool shadow_bias_auto_ = true; //enable automatic shadow bias calculation
		float ray_min_dist_ = 1.0e-5f;  //ray minimum distance
		bool ray_min_dist_auto_ = true;  //enable automatic ray minimum distance calculation
		unsigned int material_index_highest_ = 1; //!< Highest material index used for the Normalized Material Index pass.
		unsigned int material_index_auto_ = 1; //!< Material Index automatically generated for the material-index-auto render pass
		unsigned int object_index_highest_ = 1; //!< Highest object index used for the Normalized Object Index pass.
		unsigned int object_index_auto_ = 1; //!< Object Index automatically generated for the object-index-auto render pass
		std::unique_ptr<ImageFilm> image_film_;
		std::unique_ptr<const Background> background_;
		std::unique_ptr<SurfaceIntegrator> surf_integrator_;
		std::unique_ptr<VolumeIntegrator> vol_integrator_;
		std::map<std::string, std::unique_ptr<Texture>> textures_;
		std::map<std::string, std::unique_ptr<const Camera>> cameras_;
		std::map<std::string, std::unique_ptr<VolumeRegion>> volume_regions_;
		std::map<std::string, std::unique_ptr<ImageOutput>> outputs_;
		std::map<std::string, std::unique_ptr<RenderView>> render_views_;
		std::map<std::string, std::shared_ptr<Image>> images_;
		Layers layers_;
		MaskParams mask_params_;
		EdgeToonParams edge_toon_params_;
		RenderCallbacks render_callbacks_;
};

} //namespace yafaray

#endif // YAFARAY_SCENE_H
