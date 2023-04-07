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
#include "image/image_manipulation.h"
#include "render/imagefilm.h"
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

bool Renderer::render(ImageFilm &image_film, std::unique_ptr<ProgressBar> progress_bar, const Scene &scene)
{
	if(!surf_integrator_)
	{
		logger_.logError(getClassName(), " '", getName(), "': No surface integrator, bailing out...");
		return false;
	}

	render_control_.setProgressBar(std::move(progress_bar));

	FastRandom fast_random;
	bool success = surf_integrator_->preprocess(render_control, fast_random, scene);

	if(!success)
	{
		logger_.logError(getClassName(), " '", getName(), "': Preprocessing process failed, exiting...");
		return false;
	}

	if(shadow_bias_auto_) shadow_bias_ = Accelerator::shadowBias();
	if(ray_min_dist_auto_) ray_min_dist_ = Accelerator::minRayDist();
	logger_.logInfo(getClassName(), " '", getName(), "': Shadow Bias=", shadow_bias_, (shadow_bias_auto_ ? " (auto)" : ""), ", Ray Min Dist=", ray_min_dist_, (ray_min_dist_auto_ ? " (auto)" : ""));

	render_control_.setDifferentialRaysEnabled(scene.mipMapInterpolationRequired());
	render_control_.setStarted();
	success = surf_integrator_->render(image_film, fast_random, scene.getObjectIndexHighest(), scene.getMaterialIndexHighest());
	if(!success)
	{
		logger_.logError(getClassName(), " '", getName(), "': Rendering process failed, exiting...");
		return false;
	}
	render_control_.setRenderInfo(surf_integrator_->getRenderInfo());
	render_control_.setAaNoiseInfo(surf_integrator_->getAaNoiseInfo());
	surf_integrator_->cleanup(image_film);
	image_film.flush(ImageFilm::All);
	render_control_.setFinished();
	return true;
}

} //namespace yafaray