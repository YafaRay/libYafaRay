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

#ifndef LIBYAFARAY_SCENE_H
#define LIBYAFARAY_SCENE_H

#include "scene/scene_items.h"
#include "common/aa_noise_params.h"
#include "common/layers.h"
#include "common/mask_edge_toon_params.h"
#include "image/image.h"
#include "render/render_callbacks.h"
#include "render/render_control.h"
#include "render/render_view.h"
#include <list>
#include <map>
#include <vector>

namespace yafaray {

class Object;
template <typename IndexType> class FaceIndices;
class Instance;
class Light;
class Material;
class VolumeHandler;
class VolumeRegion;
class Texture;
class Camera;
class Background;
class ShaderNode;
class ImageFilm;
class Scene;
class ImageOutput;
class Format;
class ParamMap;
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
struct ParamResult;
enum class DarkDetectionType : unsigned char;

typedef unsigned int ObjId_t;

class Scene final
{
	public:
		inline static std::string getClassName() { return "Scene"; }
		explicit Scene(Logger &logger);
		Scene(const Scene &s) = delete;
		~Scene();
		int addVertex(size_t object_id, Point3f &&p, unsigned char time_step);
		int addVertex(size_t object_id, Point3f &&p, Point3f &&orco, unsigned char time_step);
		void addVertexNormal(size_t object_id, Vec3f &&n, unsigned char time_step);
		bool addFace(size_t object_id, const FaceIndices<int> &face_indices, size_t material_id);
		int addUv(size_t object_id, Uv<float> &&uv);
		bool smoothVerticesNormals(size_t object_id, float angle);
		std::pair<size_t, ParamResult> createObject(std::string &&name, ParamMap &&params);
		bool initObject(size_t object_id, size_t material_id);
		size_t createInstance();
		bool addInstanceObject(size_t instance_id, size_t object_id);
		bool addInstanceOfInstance(size_t instance_id, size_t base_instance_id);
		bool addInstanceMatrix(size_t instance_id, Matrix4f &&obj_to_world, float time);
		std::pair<const Instance *, ResultFlags> getInstance(size_t instance_id) const;
		bool updateObjects();
		std::tuple<Object *, size_t, ResultFlags> getObject(const std::string &name) const;
		std::pair<Object *, ResultFlags> getObject(size_t object_id) const;
		const SceneItems<Object> &getObjects() const { return objects_; }
		const Accelerator *getAccelerator() const { return accelerator_.get(); }

		void setAntialiasing(AaNoiseParams &&aa_noise_params) { aa_noise_params_ = aa_noise_params; };
		void setNumThreads(int threads);
		void setNumThreadsPhotons(int threads_photons);
		void createDefaultMaterial();
		void clearNonObjects();
		void clearAll();
		bool render(std::unique_ptr<ProgressBar> progress_bar);

		const Background *getBackground() const;
		const ImageFilm *getImageFilm() const { return image_film_.get(); }
		Bound<float> getSceneBound() const;
		int getNumThreads() const { return nthreads_; }
		int getNumThreadsPhotons() const { return nthreads_photons_; }
		AaNoiseParams getAaParameters() const { return aa_noise_params_; }
		RenderControl &getRenderControl() { return render_control_; }
		std::pair<size_t, ResultFlags> getMaterial(const std::string &name) const;
		const SceneItems<Material> &getMaterials() const { return materials_; }
		std::tuple<Camera *, size_t, ResultFlags> getCamera(const std::string &name) const;
		std::pair<size_t, ResultFlags> getOutput(const std::string &name) const;
		std::pair<Size2i, bool> getImageSize(size_t image_id) const;
		std::pair<Rgba, bool> getImageColor(size_t image_id, const Point2i &point) const;
		bool setImageColor(size_t image_id, const Point2i &point, const Rgba &col);
		std::tuple<Image *, size_t, ResultFlags> getImage(const std::string &name) const;
		const SceneItems<RenderView> &getRenderViews() const { return render_views_; }
		const SceneItems<VolumeRegion> &getVolumeRegions() const { return volume_regions_; }
		const SceneItems<Light> &getLights() const { return lights_; }
		SceneItems<Light> &getLights() { return lights_; }
		std::tuple<Light *, size_t, ResultFlags> getLight(const std::string &name) const;
		std::pair<Light *, ResultFlags> getLight(size_t object_id) const;
		std::pair<size_t, ParamResult> createLight(std::string &&name, ParamMap &&params);
		bool disableLight(const std::string &name);
		const SceneItems<Texture> &getTextures() const { return textures_; }
		SceneItems<Texture> &getTextures() { return textures_; }
		const SceneItems<Image> &getImages() const { return images_; }
		SceneItems<Image> &getImages() { return images_; }
		std::tuple<Texture *, size_t, ResultFlags> getTexture(const std::string &name) const;
		std::pair<Texture *, ResultFlags> getTexture(size_t object_id) const;
		std::pair<size_t, ParamResult> createTexture(std::string &&name, ParamMap &&params);
		std::pair<size_t, ParamResult> createMaterial(std::string &&name, ParamMap &&params, std::list<ParamMap> &&nodes_params);
		std::pair<size_t, ParamResult> createCamera(std::string &&name, ParamMap &&params);
		ParamResult defineBackground(ParamMap &&params);
		ParamResult defineSurfaceIntegrator(ParamMap &&params);
		ParamResult defineVolumeIntegrator(ParamMap &&params);
		std::pair<size_t, ParamResult> createVolumeRegion(std::string &&name, ParamMap &&params);
		std::pair<size_t, ParamResult> createRenderView(std::string &&name, ParamMap &&params);
		std::pair<size_t, ParamResult> createImage(std::string &&name, ParamMap &&params);
		std::pair<size_t, ParamResult> createOutput(std::string &&name, ParamMap &&params);
		bool disableOutput(std::string &&name);
		void clearOutputs();
		const SceneItems<ImageOutput> &getOutputs() const { return outputs_; }
		bool setupSceneRenderParams(Scene &scene, ParamMap &&param_map);
		void defineLayer(ParamMap &&params);
		void defineLayer(std::string &&layer_type_name, std::string &&image_type_name, std::string &&exported_image_type_name, std::string &&exported_image_name);
		void defineLayer(LayerDef::Type layer_type, Image::Type image_type = Image::Type{Image::Type::None}, Image::Type exported_image_type = Image::Type{Image::Type::None}, const std::string &exported_image_name = "");
		void clearLayers();
		const Layers * getLayers() const { return &layers_; }
		float getShadowBias() const { return shadow_bias_; }
		bool isShadowBiasAuto() const { return shadow_bias_auto_; }
		float getRayMinDist() const { return ray_min_dist_; }
		bool isRayMinDistAuto() const { return ray_min_dist_auto_; }
		const VolumeIntegrator *getVolIntegrator() const { return vol_integrator_.get(); }

		static void logInfoVerboseSuccess(Logger &logger, const std::string &pname, const std::string &name, const std::string &t);

		MaskParams getMaskParams() const { return mask_params_; }
		EdgeToonParams getEdgeToonParams() const { return edge_toon_params_; }

		void setRenderNotifyViewCallback(yafaray_RenderNotifyViewCallback callback, void *callback_data);
		void setRenderNotifyLayerCallback(yafaray_RenderNotifyLayerCallback callback, void *callback_data);
		void setRenderPutPixelCallback(yafaray_RenderPutPixelCallback callback, void *callback_data);
		void setRenderHighlightPixelCallback(yafaray_RenderHighlightPixelCallback callback, void *callback_data);
		void setRenderFlushAreaCallback(yafaray_RenderFlushAreaCallback callback, void *callback_data);
		void setRenderFlushCallback(yafaray_RenderFlushCallback callback, void *callback_data);
		void setRenderHighlightAreaCallback(yafaray_RenderHighlightAreaCallback callback, void *callback_data);
		const RenderCallbacks &getRenderCallbacks() const { return render_callbacks_; }

	private:
		void setMaskParams(const ParamMap &params);
		void setEdgeToonParams(const ParamMap &params);
		template <typename T> std::pair<size_t, ParamResult> createSceneItem(Logger &logger, std::string &&name, ParamMap &&params, SceneItems<T> &map);
		void defineBasicLayers();
		void defineDependentLayers(); //!< This function generates the basic/auxiliary layers. Must be called *after* defining all render layers with the defineLayer function.

		std::unique_ptr<Bound<float>> scene_bound_; //!< bounding box of all (finite) scene geometry
		int nthreads_ = 1;
		int nthreads_photons_ = 1;
		float shadow_bias_ = 1.0e-4f;  //shadow bias to apply to shadows to avoid self-shadow artifacts
		bool shadow_bias_auto_ = true; //enable automatic shadow bias calculation
		float ray_min_dist_ = 1.0e-5f;  //ray minimum distance
		bool ray_min_dist_auto_ = true;  //enable automatic ray minimum distance calculation
		int object_index_highest_ = 1; //!< Highest object index used for the Normalized Object Index pass.
		int material_index_highest_ = 1; //!< Highest material index used for the Normalized Object Index pass.
		size_t material_id_default_ = 0;
		std::string scene_accelerator_;
		std::unique_ptr<const Accelerator> accelerator_;
		RenderControl render_control_;
		std::unique_ptr<ImageFilm> image_film_;
		std::unique_ptr<Background> background_;
		std::unique_ptr<SurfaceIntegrator> surf_integrator_;
		std::unique_ptr<VolumeIntegrator> vol_integrator_;
		std::vector<std::unique_ptr<Instance>> instances_;
		SceneItems<Object> objects_;
		SceneItems<Light> lights_;
		SceneItems<Material> materials_;
		SceneItems<Texture> textures_;
		SceneItems<Camera> cameras_;
		SceneItems<VolumeRegion> volume_regions_;
		SceneItems<ImageOutput> outputs_;
		SceneItems<RenderView> render_views_;
		SceneItems<Image> images_;
		Layers layers_;
		AaNoiseParams aa_noise_params_;
		MaskParams mask_params_;
		EdgeToonParams edge_toon_params_;
		RenderCallbacks render_callbacks_;
		Logger &logger_;
};

} //namespace yafaray

#endif // LIBYAFARAY_SCENE_H
