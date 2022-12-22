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

bool Renderer::render(ImageFilm &image_film, std::unique_ptr<ProgressBar> progress_bar, const Scene &scene)
{
	if(!surf_integrator_)
	{
		logger_.logError(getClassName(), "'", name(), "': No surface integrator, bailing out...");
		return false;
	}

	render_control_.setProgressBar(std::move(progress_bar));

	for(auto &output : outputs_)
	{
		output.item_->init(image_film.getSize(), image_film.getExportedImageLayers(), &render_views_);
	}

	for(auto &[render_view, render_view_name, render_view_enabled] : render_views_)
	{
		if(!render_view_enabled) continue;
		for(auto &output : outputs_) output.item_->setRenderView(render_view.get());
		std::stringstream inte_settings;
		bool success = render_view->init(logger_, scene);
		if(!success)
		{
			logger_.logWarning(getClassName(), "'", name(), "': No cameras or lights found at RenderView ", render_view_name, "', skipping this RenderView...");
			continue;
		}

		FastRandom fast_random;
		success = surf_integrator_->preprocess(fast_random, &image_film, render_view.get(), scene, *this);
		if(vol_integrator_) success = success && vol_integrator_->preprocess(render_view.get(), scene, *this);

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
		image_film.flush(render_view.get(), render_control_, getEdgeToonParams());
		render_control_.setFinished();
	}
	return true;
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
	auto result{Items<RenderView>::createItem<Renderer>(logger_, *this, render_views_, name, param_map)};
	return result;
}

/*! setup the scene for rendering (set camera, background, integrator, create image film,
	set antialiasing etc.)
	attention: since this function creates an image film and asigns it to the scene,
	you need to delete it before deleting the scene!
*/
bool Renderer::setupSceneRenderParams(const ParamMap &param_map)
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

	image_film_ = ImageFilm::factory(logger_, render_control_, param_map).first;

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

std::pair<size_t, ParamResult> Renderer::createCamera(const std::string &name, const ParamMap &param_map)
{
	return Items<Camera>::createItem<Renderer>(logger_, *this, cameras_, name, param_map);
}

std::tuple<Camera *, size_t, ResultFlags> Renderer::getCamera(const std::string &name) const
{
	return cameras_.getByName(name);
}

} //namespace yafaray