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

#include "common/yafaray_common.h"
#include "common/layers.h"
#include "render/render_view.h"
#include "render/render_control.h"
#include "image/image.h"
#include "common/aa_noise_params.h"
#include "geometry/bound.h"
#include "render/render_callbacks.h"
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
class ImageOutput;
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
class Accelerator;
enum class DarkDetectionType : int;

typedef unsigned int ObjId_t;

struct MaskParams
{
	unsigned int obj_index_ = 0; //!Object Index used for masking in/out in the Mask Render Layers
	unsigned int mat_index_ = 0; //!Material Index used for masking in/out in the Mask Render Layers
	bool invert_ = false; //!False=mask in, True=mask out
	bool only_ = false; //!False=rendered image is masked, True=only the mask is shown without rendered image
};

struct EdgeToonParams //Options for Edge detection and Toon Render Layers
{
	int thickness_ = 2; //!Thickness of the edges used in the Object Edge and Toon Render Layers
	float threshold_ = 0.3f; //!Threshold for the edge detection process used in the Object Edge and Toon Render Layers
	float smoothness_ = 0.75f; //!Smoothness (blur) of the edges used in the Object Edge and Toon Render Layers
	std::array<float, 3> toon_color_ {{0.f, 0.f, 0.f}}; //!Color of the edges used in the Toon Render Layers.
	//Using array<float, 3> to avoid including color.h header dependency
	float toon_pre_smooth_ = 3.f; //!Toon effect: smoothness applied to the original image
	float toon_quantization_ = 0.1f; //!Toon effect: color Quantization applied to the original image
	float toon_post_smooth_ = 3.f; //!Toon effect: smoothness applied after Quantization
	int face_thickness_ = 1; //!Thickness of the edges used in the Faces Edge Render Layers
	float face_threshold_ = 0.01f; //!Threshold for the edge detection process used in the Faces Edge Render Layers
	float face_smoothness_ = 0.5f; //!Smoothness (blur) of the edges used in the Faces Edge Render Layers
};

class Scene
{
	public:
		static std::unique_ptr<Scene> factory(Logger &logger, ParamMap &params);
		Scene(Logger &logger);
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
		virtual Object *getObject(const std::string &name) const = 0;
		virtual const Accelerator *getAccelerator() const = 0;

		ObjId_t getNextFreeId();
		bool startObjects();
		bool endObjects();
		void setBackground(std::shared_ptr<Background> bg);
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
		const ImageFilm *getImageFilm() const { return image_film_.get(); }
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
		std::shared_ptr<Background> getBackground(const std::string &name) const;
		Integrator *getIntegrator(const std::string &name) const;
		ImageOutput *getOutput(const std::string &name) const;
		RenderView *getRenderView(const std::string &name) const;
		std::shared_ptr<Image> getImage(const std::string &name) const;
		const std::map<std::string, std::unique_ptr<RenderView>> &getRenderViews() const { return render_views_; }
		const std::map<std::string, std::unique_ptr<VolumeRegion>> &getVolumeRegions() const { return volume_regions_; }
		const std::map<std::string, std::unique_ptr<Light>> &getLights() const { return lights_; }

		Light *createLight(const std::string &name, ParamMap &params);
		Texture *createTexture(const std::string &name, ParamMap &params);
		Material *createMaterial(const std::string &name, ParamMap &params, std::list<ParamMap> &eparams);
		Camera *createCamera(const std::string &name, ParamMap &params);
		std::shared_ptr<Background> createBackground(const std::string &name, ParamMap &params);
		Integrator *createIntegrator(const std::string &name, ParamMap &params);
		ShaderNode *createShaderNode(const std::string &name, ParamMap &params);
		VolumeRegion *createVolumeRegion(const std::string &name, ParamMap &params);
		RenderView *createRenderView(const std::string &name, ParamMap &params);
		std::shared_ptr<Image> createImage(const std::string &name, ParamMap &params);
		ImageOutput *createOutput(const std::string &name, ParamMap &params);
		bool removeOutput(const std::string &name);
		void clearOutputs();
		std::map<std::string, std::unique_ptr<ImageOutput>> &getOutputs() { return outputs_; }
		const std::map<std::string, std::unique_ptr<ImageOutput>> &getOutputs() const { return outputs_; }
		bool setupSceneRenderParams(Scene &scene, const ParamMap &params);
		bool setupSceneProgressBar(Scene &scene, std::shared_ptr<ProgressBar> progress_bar);
		void defineLayer(const ParamMap &params);
		void defineLayer(const std::string &layer_type_name, const std::string &image_type_name, const std::string &exported_image_type_name, const std::string &exported_image_name);
		void defineLayer(const Layer::Type &layer_type, const Image::Type &image_type = Image::Type::None, const Image::Type &exported_image_type = Image::Type::None, const std::string &exported_image_name = "");
		void clearLayers();
		void clearRenderViews();
		const Layers &getLayers() const { return layers_; }
		float getShadowBias() const { return shadow_bias_; }
		float getRayMinDist() const { return ray_min_dist_; }

		static void logWarnExist(Logger &logger, const std::string &pname, const std::string &name);
		static void logErrNoType(Logger &logger, const std::string &pname, const std::string &name, const std::string &type);
		static void logErrOnCreate(Logger &logger, const std::string &pname, const std::string &name, const std::string &t);
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

		VolumeIntegrator *vol_integrator_ = nullptr;
		float shadow_bias_ = 1.0e-4f;  //shadow bias to apply to shadows to avoid self-shadow artifacts
		bool shadow_bias_auto_ = true;  //enable automatic shadow bias calculation
		float ray_min_dist_ = 1.0e-5f;  //ray minimum distance
		bool ray_min_dist_auto_ = true;  //enable automatic ray minimum distance calculation

	protected:
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
		std::string scene_accelerator_;
		std::map<std::string, std::unique_ptr<Light>> lights_;
		std::map<std::string, std::unique_ptr<Material>> materials_;
		RenderControl render_control_;
		Logger &logger_;

	private:
		const Layers getLayersWithImages() const;
		const Layers getLayersWithExportedImages() const;
		template <typename T> static T *findMapItem(const std::string &name, const std::map<std::string, std::unique_ptr<T>> &map);
		template <typename T> static std::shared_ptr<T> findMapItem(const std::string &name, const std::map<std::string, std::shared_ptr<T>> &map);
		void setMaskParams(const ParamMap &params);
		void setEdgeToonParams(const ParamMap &params);
		template <typename T> static T *createMapItem(Logger &logger, const std::string &name, const std::string &class_name, std::unique_ptr<T> item, std::map<std::string, std::unique_ptr<T>> &map);
		template <typename T> static T *createMapItem(Logger &logger, const std::string &name, const std::string &class_name, ParamMap &params, std::map<std::string, std::unique_ptr<T>> &map, Scene *scene, bool check_type_exists = true);
		template <typename T> static std::shared_ptr<T> createMapItem(Logger &logger, const std::string &name, const std::string &class_name, ParamMap &params, std::map<std::string, std::shared_ptr<T>> &map, Scene *scene, bool check_type_exists = true);
		void defineBasicLayers();
		void defineDependentLayers(); //!< This function generates the basic/auxiliary layers. Must be called *after* defining all render layers with the defineLayer function.

		AaNoiseParams aa_noise_params_;
		int nthreads_ = 1;
		int nthreads_photons_ = 1;
		std::unique_ptr<ImageFilm> image_film_;
		std::shared_ptr<Background> background_;
		SurfaceIntegrator *surf_integrator_ = nullptr;
		std::map<std::string, std::unique_ptr<Texture>> textures_;
		std::map<std::string, std::unique_ptr<Camera>> cameras_;
		std::map<std::string, std::shared_ptr<Background>> backgrounds_;
		std::map<std::string, std::unique_ptr<Integrator>> integrators_;
		std::map<std::string, std::unique_ptr<ShaderNode>> shaders_;
		std::map<std::string, std::unique_ptr<VolumeRegion>> volume_regions_;
		std::map<std::string, std::unique_ptr<ImageOutput>> outputs_;
		std::map<std::string, std::unique_ptr<RenderView>> render_views_;
		std::map<std::string, std::shared_ptr<Image>> images_;
		Layers layers_;
		MaskParams mask_params_;
		EdgeToonParams edge_toon_params_;
		RenderCallbacks render_callbacks_;
};

END_YAFARAY

#endif // YAFARAY_SCENE_H
