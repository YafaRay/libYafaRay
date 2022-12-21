/****************************************************************************
 *
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
 *
 */

#include "render/renderer.h"
#include "render/progress_bar.h"
#include "common/sysinfo.h"
#include "common/version_build_info.h"
#include "format/format.h"
#include "integrator/surface/integrator_surface.h"
#include "integrator/volume/integrator_volume.h"
#include "image/image_manipulation.h"
#include "render/imagefilm.h"
#include "render/render_view.h"
#include "image/image_output.h"
#include "scene/scene.h"
#include "accelerator/accelerator.h"
#include "camera/camera.h"

namespace yafaray {

Renderer::Renderer(Logger &logger, const std::string &name, const Scene &scene, const ParamMap &param_map, ::yafaray_DisplayConsole display_console) : name_{name}, logger_{logger}
{
	logger_.logInfo("LibYafaRay (", buildinfo::getVersionString(), buildinfo::getBuildTypeSuffix(), ")", " ", buildinfo::getBuildOs(), " ", buildinfo::getBuildArchitectureBits(), "bit (", buildinfo::getBuildCompiler(), ")");
	logger_.logDebug("LibYafaRay build details:");
	if(logger_.isDebug())
	{
		const std::vector<std::string> build_details = buildinfo::getAllBuildDetails();
		for(const auto &build_detail : build_details)
		{
			logger_.logDebug(build_detail);
		}
	}
	render_control_.setDifferentialRaysEnabled(false);	//By default, disable ray differential calculations. Only if at least one texture uses them, then enable differentials.
	image_manipulation::logWarningsMissingLibraries(logger_);
}

//This is just to avoid compilation error "error: invalid application of ‘sizeof’ to incomplete type, because the destructor needs to know the type of any shared_ptr or unique_ptr objects
Renderer::~Renderer() = default;

void Renderer::setNumThreads(int threads)
{
	nthreads_ = threads;

	if(nthreads_ == -1) //Automatic detection of number of threads supported by this system, taken from Blender. (DT)
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads: Active.");
		nthreads_ = sysinfo::getNumSystemThreads();
		if(logger_.isVerbose()) logger_.logVerbose("Number of Threads supported: [", nthreads_, "].");
	}
	else
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads: Inactive.");
	}

	logger_.logParams("Renderer '", name(), "' using [", nthreads_, "] Threads.");

	std::stringstream set;
	set << "CPU threads=" << nthreads_ << std::endl;

	render_control_.setRenderInfo(set.str());
}

void Renderer::setNumThreadsPhotons(int threads_photons)
{
	nthreads_photons_ = threads_photons;

	if(nthreads_photons_ == -1) //Automatic detection of number of threads supported by this system, taken from Blender. (DT)
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads for Photon Mapping: Active.");
		nthreads_photons_ = sysinfo::getNumSystemThreads();
		if(logger_.isVerbose()) logger_.logVerbose("Number of Threads supported for Photon Mapping: [", nthreads_photons_, "].");
	}
	else
	{
		if(logger_.isVerbose()) logger_.logVerbose("Automatic Detection of Threads for Photon Mapping: Inactive.");
	}

	logger_.logParams("Renderer '", name(), "' using for Photon Mapping [", nthreads_photons_, "] Threads.");
}

bool Renderer::render(std::unique_ptr<ProgressBar> progress_bar, const Scene &scene)
{
	if(!image_film_)
	{
		logger_.logError(getClassName(), "'", name(), "': No ImageFilm present, bailing out...");
		return false;
	}
	if(!surf_integrator_)
	{
		logger_.logError(getClassName(), "'", name(), "': No surface integrator, bailing out...");
		return false;
	}

	render_control_.setProgressBar(std::move(progress_bar));

	for(auto &output : outputs_)
	{
		output.item_->init(image_film_->getSize(), image_film_->getExportedImageLayers(), &render_views_);
	}

	for(auto &render_view : render_views_)
	{
		for(auto &output : outputs_) output.item_->setRenderView(render_view.item_.get());
		std::stringstream inte_settings;
		bool success = render_view.item_->init(logger_, scene);
		if(!success)
		{
			logger_.logWarning(getClassName(), "'", name(), "': No cameras or lights found at RenderView ", render_view.name_, "', skipping this RenderView...");
			continue;
		}

		FastRandom fast_random;
		success = surf_integrator_->preprocess(fast_random, image_film_.get(), render_view.item_.get(), scene, *this);
		if(vol_integrator_) success = success && vol_integrator_->preprocess(fast_random, image_film_.get(), render_view.item_.get(), scene, *this);

		if(!success)
		{
			logger_.logError(getClassName(), "'", name(), "': Preprocessing process failed, exiting...");
			return false;
		}

		cameras_.clearModifiedList();
		outputs_.clearModifiedList();
		render_views_.clearModifiedList();

		if(shadow_bias_auto_) shadow_bias_ = Accelerator::shadowBias();
		if(ray_min_dist_auto_) ray_min_dist_ = Accelerator::minRayDist();
		logger_.logInfo(getClassName(), "'", name(), "': Shadow Bias=", shadow_bias_, (shadow_bias_auto_ ? " (auto)" : ""), ", Ray Min Dist=", ray_min_dist_, (ray_min_dist_auto_ ? " (auto)" : ""));

		render_control_.setDifferentialRaysEnabled(scene.mipMapInterpolationRequired());
		render_control_.setStarted();
		success = surf_integrator_->render(fast_random, scene.getObjectIndexHighest(), scene.getMaterialIndexHighest());
		if(!success)
		{
			logger_.logError(getClassName(), "'", name(), "': Rendering process failed, exiting...");
			return false;
		}
		render_control_.setRenderInfo(surf_integrator_->getRenderInfo());
		render_control_.setAaNoiseInfo(surf_integrator_->getAaNoiseInfo());
		surf_integrator_->cleanup();
		image_film_->flush(render_view.item_.get(), render_control_, getEdgeToonParams());
		render_control_.setFinished();
		image_film_->cleanup();
	}
	return true;
}

void Renderer::clearOutputs()
{
	outputs_.clear();
}

void Renderer::clearLayers()
{
	layers_.clear();
}

std::pair<size_t, ResultFlags> Renderer::getOutput(const std::string &name) const
{
	return outputs_.findIdFromName(name);
}

bool Renderer::disableOutput(const std::string &name)
{
	const auto result{outputs_.disable(name)};
	return result.isOk();
}

std::pair<size_t, ParamResult> Renderer::createOutput(const std::string &name, const ParamMap &param_map)
{
	return createRendererItem<ImageOutput>(logger_, name, param_map, outputs_);
}

ParamResult Renderer::defineSurfaceIntegrator(const ParamMap &param_map)
{
	auto [surface_integrator, surface_integrator_result]{SurfaceIntegrator::factory(logger_, render_control_, param_map)};
	if(logger_.isVerbose() && surface_integrator)
	{
		logger_.logVerbose("Renderer '", name(), "': Added ", surface_integrator->getClassName(), " '", this->name(), "' (", surface_integrator->type().print(), ")!");
	}
	//logger_.logParams(result.first->getAsParamMap(true).print()); //TEST CODE ONLY, REMOVE!!
	surf_integrator_ = std::move(surface_integrator);
	return surface_integrator_result;
}

ParamResult Renderer::defineVolumeIntegrator(const Scene &scene, const ParamMap &param_map)
{
	auto [volume_integrator, volume_integrator_result]{VolumeIntegrator::factory(logger_, scene.getVolumeRegions(), param_map)};
	if(logger_.isVerbose() && volume_integrator)
	{
		logger_.logVerbose("Renderer '", name(), "': Added ", volume_integrator->getClassName(), " '", this->name(), "' (", volume_integrator->type().print(), ")!");
	}
	//logger_.logParams(result.first->getAsParamMap(true).print()); //TEST CODE ONLY, REMOVE!!
	vol_integrator_ = std::move(volume_integrator);
	return volume_integrator_result;
}

std::pair<size_t, ParamResult> Renderer::createRenderView(const std::string &name, const ParamMap &param_map)
{
	return createRendererItem<RenderView>(logger_, name, param_map, render_views_);
}

/*! setup the scene for rendering (set camera, background, integrator, create image film,
	set antialiasing etc.)
	attention: since this function creates an image film and asigns it to the scene,
	you need to delete it before deleting the scene!
*/
bool Renderer::setupSceneRenderParams(Scene &scene, const ParamMap &param_map)
{
	if(logger_.isDebug()) logger_.logDebug("**Renderer::setupSceneRenderParams 'raw' ParamMap\n" + param_map.logContents());
	std::string name;
	std::string aa_dark_detection_type_string = "none";
	AaNoiseParams aa_noise_params;
	int nthreads = -1, nthreads_photons = -1;
	bool adv_auto_shadow_bias_enabled = true;
	float adv_shadow_bias_value = Accelerator::shadowBias();
	bool adv_auto_min_raydist_enabled = true;
	float adv_min_raydist_value = Accelerator::minRayDist();
	int adv_base_sampling_offset = 0;
	int adv_computer_node = 0;
	bool background_resampling = true;  //If false, the background will not be resampled in subsequent adaptative AA passes

	param_map.getParam("AA_passes", aa_noise_params.passes_);
	param_map.getParam("AA_minsamples", aa_noise_params.samples_);
	aa_noise_params.inc_samples_ = aa_noise_params.samples_;
	param_map.getParam("AA_inc_samples", aa_noise_params.inc_samples_);
	param_map.getParam("AA_threshold", aa_noise_params.threshold_);
	param_map.getParam("AA_resampled_floor", aa_noise_params.resampled_floor_);
	param_map.getParam("AA_sample_multiplier_factor", aa_noise_params.sample_multiplier_factor_);
	param_map.getParam("AA_light_sample_multiplier_factor", aa_noise_params.light_sample_multiplier_factor_);
	param_map.getParam("AA_indirect_sample_multiplier_factor", aa_noise_params.indirect_sample_multiplier_factor_);
	param_map.getParam("AA_detect_color_noise", aa_noise_params.detect_color_noise_);
	param_map.getParam("AA_dark_detection_type", aa_dark_detection_type_string);
	param_map.getParam("AA_dark_threshold_factor", aa_noise_params.dark_threshold_factor_);
	param_map.getParam("AA_variance_edge_size", aa_noise_params.variance_edge_size_);
	param_map.getParam("AA_variance_pixels", aa_noise_params.variance_pixels_);
	param_map.getParam("AA_clamp_samples", aa_noise_params.clamp_samples_);
	param_map.getParam("AA_clamp_indirect", aa_noise_params.clamp_indirect_);
	param_map.getParam("threads", nthreads); // number of threads, -1 = auto detection
	param_map.getParam("background_resampling", background_resampling);

	nthreads_photons = nthreads;	//if no "threads_photons" parameter exists, make "nthreads_photons" equal to render threads

	param_map.getParam("threads_photons", nthreads_photons); // number of threads for photon mapping, -1 = auto detection
	param_map.getParam("adv_auto_shadow_bias_enabled", adv_auto_shadow_bias_enabled);
	param_map.getParam("adv_shadow_bias_value", adv_shadow_bias_value);
	param_map.getParam("adv_auto_min_raydist_enabled", adv_auto_min_raydist_enabled);
	param_map.getParam("adv_min_raydist_value", adv_min_raydist_value);
	param_map.getParam("adv_base_sampling_offset", adv_base_sampling_offset); //Base sampling offset, in case of multi-computer rendering each should have a different offset so they don't "repeat" the same samples (user configurable)
	param_map.getParam("adv_computer_node", adv_computer_node); //Computer node in multi-computer render environments/render farms

	defineBasicLayers();
	defineDependentLayers();
	setMaskParams(param_map);
	setEdgeToonParams(param_map);

	image_film_ = ImageFilm::factory(logger_, render_control_, param_map, *this).first;

	param_map.getParam("filter_type", name); // AA filter type
	std::stringstream aa_settings;
	aa_settings << "AA Settings (" << ((!name.empty()) ? name : "box") << "): Tile size=" << image_film_->getTileSize();
	render_control_.setAaNoiseInfo(aa_settings.str());

	if(aa_dark_detection_type_string == "linear") aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::Linear;
	else if(aa_dark_detection_type_string == "curve") aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::Curve;
	else aa_noise_params.dark_detection_type_ = AaNoiseParams::DarkDetectionType::None;

	setAntialiasing(std::move(aa_noise_params));
	setNumThreads(nthreads);
	setNumThreadsPhotons(nthreads_photons);
	shadow_bias_auto_ = adv_auto_shadow_bias_enabled;
	shadow_bias_ = adv_shadow_bias_value;
	ray_min_dist_auto_ = adv_auto_min_raydist_enabled;
	ray_min_dist_ = adv_min_raydist_value;
	if(logger_.isDebug())logger_.logDebug("adv_base_sampling_offset=", adv_base_sampling_offset);
	image_film_->setBaseSamplingOffset(adv_base_sampling_offset);
	image_film_->setComputerNode(adv_computer_node);
	image_film_->setBackgroundResampling(background_resampling);

	return true;
}

void Renderer::defineLayer(const ParamMap &param_map)
{
	if(logger_.isDebug()) logger_.logDebug("**Renderer::defineLayer 'raw' ParamMap\n" + param_map.logContents());
	std::string layer_type_name, image_type_name, exported_image_name, exported_image_type_name;
	param_map.getParam("type", layer_type_name);
	param_map.getParam("image_type", image_type_name);
	param_map.getParam("exported_image_name", exported_image_name);
	param_map.getParam("exported_image_type", exported_image_type_name);
	defineLayer(std::move(layer_type_name), std::move(image_type_name), std::move(exported_image_type_name), std::move(exported_image_name));
}

void Renderer::defineLayer(std::string &&layer_type_name, std::string &&image_type_name, std::string &&exported_image_type_name, std::string &&exported_image_name)
{
	const LayerDef::Type layer_type = LayerDef::getType(layer_type_name);
	const Image::Type image_type = image_type_name.empty() ? LayerDef::getDefaultImageType(layer_type) : Image::getTypeFromName(image_type_name);
	const Image::Type exported_image_type = Image::getTypeFromName(exported_image_type_name);
	defineLayer(layer_type, image_type, exported_image_type, exported_image_name);
}

void Renderer::defineLayer(LayerDef::Type layer_type, Image::Type image_type, Image::Type exported_image_type, const std::string &exported_image_name)
{
	if(layer_type == LayerDef::Disabled)
	{
		logger_.logWarning(getClassName(), "'", name(), "': cannot create layer '", LayerDef::getName(layer_type), "' of unknown or disabled layer type");
		return;
	}

	if(Layer *existing_layer = layers_.find(layer_type))
	{
		if(existing_layer->getType() == layer_type &&
		   existing_layer->getImageType() == image_type &&
		   existing_layer->getExportedImageType() == exported_image_type) return;

		if(logger_.isDebug())logger_.logDebug(getClassName(), "'", name(), "': had previously defined: ", existing_layer->print());
		if(image_type == Image::Type::None && existing_layer->getImageType() != Image::Type::None)
		{
			if(logger_.isDebug())logger_.logDebug(getClassName(), "'", name(), "': the layer '", LayerDef::getName(layer_type), "' had previously a defined internal image which cannot be removed.");
		}
		else existing_layer->setImageType(image_type);

		if(exported_image_type == Image::Type::None && existing_layer->getExportedImageType() != Image::Type::None)
		{
			if(logger_.isDebug())logger_.logDebug(getClassName(), "'", name(), "': the layer '", LayerDef::getName(layer_type), "' was previously an exported layer and cannot be changed into an internal layer now.");
		}
		else
		{
			existing_layer->setExportedImageType(exported_image_type);
			existing_layer->setExportedImageName(exported_image_name);
		}
		existing_layer->setType(layer_type);
		logger_.logInfo(getClassName(), "'", name(), "': layer redefined: " + existing_layer->print());
	}
	else
	{
		Layer new_layer(layer_type, image_type, exported_image_type, exported_image_name);
		layers_.set(layer_type, new_layer);
		logger_.logInfo(getClassName(), "'", name(), "': layer defined: ", new_layer.print());
	}
}

void Renderer::defineBasicLayers()
{
	//by default we will have an external/internal Combined layer
	if(!layers_.isDefined(LayerDef::Combined)) defineLayer(LayerDef::Combined, Image::Type::ColorAlpha, Image::Type::ColorAlpha);

	//This auxiliary layer will always be needed for material-specific number of samples calculation
	if(!layers_.isDefined(LayerDef::DebugSamplingFactor)) defineLayer(LayerDef::DebugSamplingFactor, Image::Type::Gray);
}

void Renderer::defineDependentLayers()
{
	for(const auto &[layer_def, layer] : layers_)
	{
		switch(layer_def)
		{
			case LayerDef::ZDepthNorm:
				if(!layers_.isDefined(LayerDef::Mist)) defineLayer(LayerDef::Mist);
				break;

			case LayerDef::Mist:
				if(!layers_.isDefined(LayerDef::ZDepthNorm)) defineLayer(LayerDef::ZDepthNorm);
				break;

			case LayerDef::ReflectAll:
				if(!layers_.isDefined(LayerDef::ReflectPerfect)) defineLayer(LayerDef::ReflectPerfect);
				if(!layers_.isDefined(LayerDef::Glossy)) defineLayer(LayerDef::Glossy);
				if(!layers_.isDefined(LayerDef::GlossyIndirect)) defineLayer(LayerDef::GlossyIndirect);
				break;

			case LayerDef::RefractAll:
				if(!layers_.isDefined(LayerDef::RefractPerfect)) defineLayer(LayerDef::RefractPerfect);
				if(!layers_.isDefined(LayerDef::Trans)) defineLayer(LayerDef::Trans);
				if(!layers_.isDefined(LayerDef::TransIndirect)) defineLayer(LayerDef::TransIndirect);
				break;

			case LayerDef::IndirectAll:
				if(!layers_.isDefined(LayerDef::Indirect)) defineLayer(LayerDef::Indirect);
				if(!layers_.isDefined(LayerDef::DiffuseIndirect)) defineLayer(LayerDef::DiffuseIndirect);
				break;

			case LayerDef::ObjIndexMaskAll:
				if(!layers_.isDefined(LayerDef::ObjIndexMask)) defineLayer(LayerDef::ObjIndexMask);
				if(!layers_.isDefined(LayerDef::ObjIndexMaskShadow)) defineLayer(LayerDef::ObjIndexMaskShadow);
				break;

			case LayerDef::MatIndexMaskAll:
				if(!layers_.isDefined(LayerDef::MatIndexMask)) defineLayer(LayerDef::MatIndexMask);
				if(!layers_.isDefined(LayerDef::MatIndexMaskShadow)) defineLayer(LayerDef::MatIndexMaskShadow);
				break;

			case LayerDef::DebugFacesEdges:
				if(!layers_.isDefined(LayerDef::NormalGeom)) defineLayer(LayerDef::NormalGeom, Image::Type::ColorAlpha);
				if(!layers_.isDefined(LayerDef::ZDepthNorm)) defineLayer(LayerDef::ZDepthNorm, Image::Type::GrayAlpha);
				break;

			case LayerDef::DebugObjectsEdges:
				if(!layers_.isDefined(LayerDef::Toon)) defineLayer(LayerDef::Toon, Image::Type::ColorAlpha);
				if(!layers_.isDefined(LayerDef::NormalSmooth)) defineLayer(LayerDef::NormalSmooth, Image::Type::ColorAlpha);
				if(!layers_.isDefined(LayerDef::ZDepthNorm)) defineLayer(LayerDef::ZDepthNorm, Image::Type::GrayAlpha);
				break;

			case LayerDef::Toon:
				if(!layers_.isDefined(LayerDef::DebugObjectsEdges)) defineLayer(LayerDef::DebugObjectsEdges, Image::Type::ColorAlpha);
				if(!layers_.isDefined(LayerDef::NormalSmooth)) defineLayer(LayerDef::NormalSmooth, Image::Type::ColorAlpha);
				if(!layers_.isDefined(LayerDef::ZDepthNorm)) defineLayer(LayerDef::ZDepthNorm, Image::Type::GrayAlpha);
				break;

			default:
				break;
		}
	}
}

void Renderer::setMaskParams(const ParamMap &params)
{
	std::string external_pass, internal_pass;
	int mask_obj_index = 0, mask_mat_index = 0;
	bool mask_invert = false;
	bool mask_only = false;

	params.getParam("layer_mask_obj_index", mask_obj_index);
	params.getParam("layer_mask_mat_index", mask_mat_index);
	params.getParam("layer_mask_invert", mask_invert);
	params.getParam("layer_mask_only", mask_only);

	MaskParams mask_params;
	mask_params.obj_index_ = mask_obj_index;
	mask_params.mat_index_ = mask_mat_index;
	mask_params.invert_ = mask_invert;
	mask_params.only_ = mask_only;

	mask_params_ = mask_params;
}

void Renderer::setEdgeToonParams(const ParamMap &params)
{
	Rgb toon_edge_color(0.f);
	int object_edge_thickness = 2;
	float object_edge_threshold = 0.3f;
	float object_edge_smoothness = 0.75f;
	float toon_pre_smooth = 3.f;
	float toon_quantization = 0.1f;
	float toon_post_smooth = 3.f;
	int faces_edge_thickness = 1;
	float faces_edge_threshold = 0.01f;
	float faces_edge_smoothness = 0.5f;

	params.getParam("layer_toon_edge_color", toon_edge_color);
	params.getParam("layer_object_edge_thickness", object_edge_thickness);
	params.getParam("layer_object_edge_threshold", object_edge_threshold);
	params.getParam("layer_object_edge_smoothness", object_edge_smoothness);
	params.getParam("layer_toon_pre_smooth", toon_pre_smooth);
	params.getParam("layer_toon_quantization", toon_quantization);
	params.getParam("layer_toon_post_smooth", toon_post_smooth);
	params.getParam("layer_faces_edge_thickness", faces_edge_thickness);
	params.getParam("layer_faces_edge_threshold", faces_edge_threshold);
	params.getParam("layer_faces_edge_smoothness", faces_edge_smoothness);

	EdgeToonParams edge_params;
	edge_params.thickness_ = object_edge_thickness;
	edge_params.threshold_ = object_edge_threshold;
	edge_params.smoothness_ = object_edge_smoothness;
	edge_params.toon_color_ = toon_edge_color;
	edge_params.toon_pre_smooth_ = toon_pre_smooth;
	edge_params.toon_quantization_ = toon_quantization;
	edge_params.toon_post_smooth_ = toon_post_smooth;
	edge_params.face_thickness_ = faces_edge_thickness;
	edge_params.face_threshold_ = faces_edge_threshold;
	edge_params.face_smoothness_ = faces_edge_smoothness;
	edge_toon_params_ = edge_params;
}

void Renderer::setRenderNotifyViewCallback(yafaray_RenderNotifyViewCallback callback, void *callback_data)
{
	render_callbacks_.notify_view_ = callback;
	render_callbacks_.notify_view_data_ = callback_data;
}

void Renderer::setRenderNotifyLayerCallback(yafaray_RenderNotifyLayerCallback callback, void *callback_data)
{
	render_callbacks_.notify_layer_ = callback;
	render_callbacks_.notify_layer_data_ = callback_data;
}

void Renderer::setRenderPutPixelCallback(yafaray_RenderPutPixelCallback callback, void *callback_data)
{
	render_callbacks_.put_pixel_ = callback;
	render_callbacks_.put_pixel_data_ = callback_data;
}

void Renderer::setRenderHighlightPixelCallback(yafaray_RenderHighlightPixelCallback callback, void *callback_data)
{
	render_callbacks_.highlight_pixel_ = callback;
	render_callbacks_.highlight_pixel_data_ = callback_data;
}

void Renderer::setRenderFlushAreaCallback(yafaray_RenderFlushAreaCallback callback, void *callback_data)
{
	render_callbacks_.flush_area_ = callback;
	render_callbacks_.flush_area_data_ = callback_data;
}

void Renderer::setRenderFlushCallback(yafaray_RenderFlushCallback callback, void *callback_data)
{
	render_callbacks_.flush_ = callback;
	render_callbacks_.flush_data_ = callback_data;
}

void Renderer::setRenderHighlightAreaCallback(yafaray_RenderHighlightAreaCallback callback, void *callback_data)
{
	render_callbacks_.highlight_area_ = callback;
	render_callbacks_.highlight_area_data_ = callback_data;
}

template <typename T>
std::pair<size_t, ParamResult> Renderer::createRendererItem(Logger &logger, const std::string &name, const ParamMap &param_map, SceneItems<T> &map)
{
	const auto [existing_item, existing_item_id, existing_item_result]{map.getByName(name)};
	if(existing_item)
	{
		if(logger.isVerbose()) logger.logWarning(getClassName(), "'", this->name(), "': item with name '", name, "' already exists, overwriting with new item.");
	}
	std::unique_ptr<T> new_item;
	ParamResult param_result;
	if constexpr (std::is_same_v<T, RenderView>) std::tie(new_item, param_result) = T::factory(logger, *this, name, param_map);
	else std::tie(new_item, param_result) = T::factory(logger, name, param_map);
	if(new_item)
	{
		if(logger.isVerbose())
		{
			logger.logVerbose(getClassName(), "'", this->name(), "': Added ", new_item->getClassName(), " '", name, "' (", new_item->type().print(), ")!");
		}
		const auto [new_item_id, adding_result]{map.add(name, std::move(new_item))};
		param_result.flags_ |= adding_result;
		return {new_item_id, param_result};
	}
	else return {0, ParamResult{YAFARAY_RESULT_ERROR_WHILE_CREATING}};
}

std::pair<size_t, ParamResult> Renderer::createCamera(const std::string &name, const ParamMap &param_map)
{
	return createRendererItem<Camera>(logger_, name, param_map, cameras_);
}

std::tuple<Camera *, size_t, ResultFlags> Renderer::getCamera(const std::string &name) const
{
	return cameras_.getByName(name);
}

} //namespace yafaray