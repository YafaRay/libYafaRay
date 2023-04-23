/****************************************************************************
 *
 *      imagefilm.cc: image data handling class
 *      This is part of the libYafaRay package
 *      See AUTHORS for more information
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

#include "render/imagefilm.h"
#include "common/logger.h"
#include "format/format.h"
#include "scene/scene.h"
#include "camera/camera.h"
#include "common/file.h"
#include "param/param.h"
#include "common/timer.h"
#include "color/color_layers.h"
#include "math/filter.h"
#include "common/version_build_info.h"
#include "image/image_manipulation.h"
#include "integrator/surface/integrator_surface.h"
#include "render/render_monitor.h"
#include "image/image_output.h"

namespace yafaray {

typedef float FilterFunction_t(float dx, float dy);

std::map<std::string, const ParamMeta *> ImageFilm::Params::getParamMetaMap()
{
	std::map<std::string, const ParamMeta *> param_meta_map;
	PARAM_META(threads_);
	PARAM_META(background_resampling_);
	PARAM_META(base_sampling_offset_);
	PARAM_META(computer_node_);
	PARAM_META(aa_pixel_width_);
	PARAM_META(width_);
	PARAM_META(height_);
	PARAM_META(start_x_);
	PARAM_META(start_y_);
	PARAM_META(filter_type_);
	PARAM_META(tile_size_);
	PARAM_META(tiles_order_);
	PARAM_META(images_autosave_interval_type_);
	PARAM_META(images_autosave_interval_passes_);
	PARAM_META(images_autosave_interval_seconds_);
	PARAM_META(film_load_save_mode_);
	PARAM_META(film_load_save_path_);
	PARAM_META(film_autosave_interval_type_);
	PARAM_META(film_autosave_interval_passes_);
	PARAM_META(film_autosave_interval_seconds_);
	return param_meta_map;
}

ImageFilm::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(threads_);
	PARAM_LOAD(background_resampling_);
	PARAM_LOAD(base_sampling_offset_);
	PARAM_LOAD(computer_node_);
	PARAM_LOAD(aa_pixel_width_);
	PARAM_LOAD(width_);
	PARAM_LOAD(height_);
	PARAM_LOAD(start_x_);
	PARAM_LOAD(start_y_);
	PARAM_ENUM_LOAD(filter_type_);
	PARAM_LOAD(tile_size_);
	PARAM_ENUM_LOAD(tiles_order_);
	PARAM_ENUM_LOAD(images_autosave_interval_type_);
	PARAM_LOAD(images_autosave_interval_passes_);
	PARAM_LOAD(images_autosave_interval_seconds_);
	PARAM_ENUM_LOAD(film_load_save_mode_);
	PARAM_LOAD(film_load_save_path_);
	PARAM_ENUM_LOAD(film_autosave_interval_type_);
	PARAM_LOAD(film_autosave_interval_passes_);
	PARAM_LOAD(film_autosave_interval_seconds_);
}

ParamMap ImageFilm::getAsParamMap(bool only_non_default) const
{
	ParamMap param_map;
	param_map.setParam(Params::computer_node_meta_, computer_node_);
	param_map.setParam(Params::base_sampling_offset_meta_, base_sampling_offset_);
	PARAM_SAVE(threads_);
	PARAM_SAVE(background_resampling_);
	PARAM_SAVE(base_sampling_offset_);
	PARAM_SAVE(computer_node_);
	PARAM_SAVE(aa_pixel_width_);
	PARAM_SAVE(width_);
	PARAM_SAVE(height_);
	PARAM_SAVE(start_x_);
	PARAM_SAVE(start_y_);
	PARAM_ENUM_SAVE(filter_type_);
	PARAM_SAVE(tile_size_);
	PARAM_ENUM_SAVE(tiles_order_);
	PARAM_ENUM_SAVE(images_autosave_interval_type_);
	PARAM_SAVE(images_autosave_interval_passes_);
	PARAM_SAVE(images_autosave_interval_seconds_);
	PARAM_ENUM_SAVE(film_load_save_mode_);
	PARAM_SAVE(film_load_save_path_);
	PARAM_ENUM_SAVE(film_autosave_interval_type_);
	PARAM_SAVE(film_autosave_interval_passes_);
	PARAM_SAVE(film_autosave_interval_seconds_);
	return param_map;
}

std::pair<std::unique_ptr<ImageFilm>, ParamResult> ImageFilm::factory(Logger &logger, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + "::factory 'raw' ParamMap\n" + param_map.logContents());
	auto param_result{class_meta::check<Params>(param_map, {}, {})};
	auto result {std::make_unique<ImageFilm>(logger, param_result, name, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ImageFilm>("ImageFilm", {}));
	return {std::move(result), param_result};
}

ImageFilm::ImageFilm(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map) : params_{param_result, param_map}, name_{name}, outputs_{std::make_unique<Items<ImageOutput>>()}, logger_{logger}
{
	if(logger_.isDebug()) logger_.logDebug("**" + getClassName() + " params_:\n" + getAsParamMap(true).print());
	if(params_.images_autosave_interval_type_ == AutoSaveParams::IntervalType::Pass) logger_.logInfo(getClassName(), ": ", "AutoSave partially rendered image every ", params_.images_autosave_interval_passes_, " passes");

	if(params_.images_autosave_interval_type_ == AutoSaveParams::IntervalType::Time) logger_.logInfo(getClassName(), ": ", "AutoSave partially rendered image every ", params_.images_autosave_interval_seconds_, " seconds");

	if(params_.film_load_save_mode_ != FilmLoadSave::Mode::Save) logger_.logInfo(getClassName(), ": ", "Enabling imageFilm file saving feature");
	if(params_.film_load_save_mode_ == FilmLoadSave::Mode::LoadAndSave) logger_.logInfo(getClassName(), ": ", "Enabling imageFilm Loading feature. It will load and combine the ImageFilm files from the currently selected image output folder before start rendering, autodetecting each film format (binary/text) automatically. If they don't match exactly the scene, bad results could happen. Use WITH CARE!");

	if(params_.film_autosave_interval_type_ == AutoSaveParams::IntervalType::Pass) logger_.logInfo(getClassName(), ": ", "AutoSave internal imageFilm every ", params_.film_autosave_interval_passes_, " passes");

	if(params_.film_autosave_interval_type_ == AutoSaveParams::IntervalType::Time) logger_.logInfo(getClassName(), ": ", "AutoSave internal imageFilm image every ", params_.film_autosave_interval_seconds_, " seconds");

	// fill filter table:
	FilterFunction_t *filter_function;
	switch(params_.filter_type_.value())
	{
		case ImageFilm::FilterType::Mitchell:
			filter_function = math::filter::mitchell;
			filter_width_ *= 2.6f;
			break;
		case ImageFilm::FilterType::Lanczos:
			filter_function = math::filter::lanczos2;
			break;
		case ImageFilm::FilterType::Gauss:
			filter_function = math::filter::gauss;
			filter_width_ *= 2.f;
			break;
		case ImageFilm::FilterType::Box:
		default:
			filter_function = math::filter::box;
			break;
	}

	filter_width_ = std::min(std::max(0.501f, filter_width_), 0.5f * max_filter_size_); // filter needs to cover at least the area of one pixel and no more than MAX_FILTER_SIZE/2

	size_t filter_table_index = 0;
	for(int y = 0; y < filter_table_size_; ++y)
	{
		for(int x = 0; x < filter_table_size_; ++x)
		{
			filter_table_[filter_table_index] = filter_function((x + .5f) * filter_scale_, (y + .5f) * filter_scale_);
			++filter_table_index;
		}
	}

	filter_table_scale_ = 0.9999f * filter_table_size_ / filter_width_;
	area_cnt_ = 0;
}

//This is just to avoid compilation error "error: invalid application of ‘sizeof’ to incomplete type, because the destructor needs to know the type of any shared_ptr or unique_ptr objects
ImageFilm::~ImageFilm() = default;

void ImageFilm::initLayersImages()
{
	for(const auto &[layer_def, layer] : layers_.getLayersWithImages())
	{
		auto image_type{layer.getImageType()};
		image_type = Image::imageTypeWithAlpha(image_type); //Alpha channel is needed in all images of the weight normalization process will cause problems
		Image::Params image_params;
		image_params.width_ = params_.width_;
		image_params.height_ = params_.height_;
		image_params.type_ = image_type;
		image_params.image_optimization_ = Image::Optimization::None;
		auto image{Image::factory(image_params)};
		film_image_layers_.set(layer_def, {std::move(image), layer});
	}
}

void ImageFilm::initLayersExportedImages()
{
	for(const auto &[layer_def, layer]: layers_.getLayersWithExportedImages())
	{
		auto image_type{layer.getImageType()};
		image_type = Image::imageTypeWithAlpha(image_type); //Alpha channel is needed in all images of the weight normalization process will cause problems
		Image::Params image_params;
		image_params.width_ = params_.width_;
		image_params.height_ = params_.height_;
		image_params.type_ = image_type;
		image_params.image_optimization_ = Image::Optimization::None;
		auto image{Image::factory(image_params)};
		exported_image_layers_.set(layer_def, {std::move(image), layer});
	}
}

void ImageFilm::init(RenderControl &render_control, RenderMonitor &render_monitor, const SurfaceIntegrator &surface_integrator)
{
	aa_noise_params_ = surface_integrator.getAaParameters();
	edge_toon_params_ = surface_integrator.getEdgeToonParams();

	defineBasicLayers();
	defineDependentLayers();

	//Creation of the image buffers for the render passes
	film_image_layers_.clear();
	exported_image_layers_.clear();
	initLayersImages();
	//If there are any ImageOutputs, creation of the image buffers for the image outputs exported images
	if(!outputs_->empty())
	{
		initLayersExportedImages();
		for(auto &output : *outputs_)
		{
			output.item_->init(getSize(), getExportedImageLayers(), getName());
		}
	}

	// Clear density image
	if(estimate_density_)
	{
		density_image_ = std::make_unique<Buffer2D<Rgb>>(Size2i{{params_.width_, params_.height_}});
	}

	// Setup the bucket splitter
	if(split_)
	{
		next_area_ = 0;
		splitter_ = std::make_unique<ImageSplitter>(params_.width_, params_.height_, params_.start_x_, params_.start_y_, params_.tile_size_, params_.tiles_order_, params_.threads_);
		area_cnt_ = splitter_->size();
	}
	else area_cnt_ = 1;

	render_monitor.initProgressBar(params_.width_ * params_.height_, logger_.getConsoleLogColorsEnabled());

	cancel_ = false;
	completed_cnt_ = 0;
	n_pass_ = 1;

	images_auto_save_params_.pass_counter_ = 0;
	film_load_save_.auto_save_.pass_counter_ = 0;
	resetImagesAutoSaveTimer();
	resetFilmAutoSaveTimer();

	render_monitor.addTimerEvent("imagesAutoSaveTimer");
	render_monitor.addTimerEvent("filmAutoSaveTimer");
	render_monitor.startTimer("imagesAutoSaveTimer");
	render_monitor.startTimer("filmAutoSaveTimer");

	if(film_load_save_.mode_ == FilmLoadSave::Mode::LoadAndSave) imageFilmLoadAllInFolder(render_control, render_monitor);	//Load all the existing Film in the images output folder, combining them together. It will load only the Film files with the same "base name" as the output image film (including file name, computer node name and frame) to allow adding samples to animations.
	if(film_load_save_.mode_ == FilmLoadSave::Mode::LoadAndSave || film_load_save_.mode_ == FilmLoadSave::Mode::Save) imageFilmFileBackup(render_control, render_monitor); //If the imageFilm is set to Save, at the start rename the previous film file as a "backup" just in case the user has made a mistake and wants to get the previous film back.

	if(notify_layer_callback_)
	{
		const Layers &layers = layers_.getLayersWithExportedImages();
		for(const auto &[layer_type, layer] : layers)
		{
			notify_layer_callback_(LayerDef::getName(layer_type).c_str(), layer.getExportedImageName().c_str(), params_.width_, params_.height_, layer.getNumExportedChannels(), notify_layer_callback_data_);
		}
	}
}

bool ImageFilm::render(RenderControl &render_control, RenderMonitor &render_monitor, SurfaceIntegrator &surface_integrator, const Scene &scene)
{
	/*FIXME bool success = surface_integrator.preprocess(render_control, fast_random, scene);
	if(!success)
	{
		logger_.logError(getClassName(), " '", getName(), "': Preprocessing process failed, exiting...");
		return false;
	} */

	//logger_.logInfo(getClassName(), " '", getName(), "': Shadow Bias=", shadow_bias_, (shadow_bias_auto_ ? " (auto)" : ""), ", Ray Min Dist=", ray_min_dist_, (ray_min_dist_auto_ ? " (auto)" : ""));
	render_control.setStarted();
	bool success = surface_integrator.render(render_control, render_monitor, *this, scene.getObjectIndexHighest(), scene.getMaterialIndexHighest());
	if(!success)
	{
		logger_.logError(getClassName(), " '", getName(), "': Rendering process failed, exiting...");
		return false;
	}
	surface_integrator.cleanup(*this);
	flush(render_control, render_monitor, ImageFilm::All);
	render_control.setFinished();
	return true;
}

int ImageFilm::nextPass(RenderControl &render_control, RenderMonitor &render_monitor, bool adaptive_aa, const std::string &integrator_name, const EdgeToonParams &edge_params, bool skip_nrender_layer)
{
	next_area_ = 0;
	n_pass_++;
	images_auto_save_params_.pass_counter_++;
	film_load_save_.auto_save_.pass_counter_++;

	if(skip_nrender_layer) return 0;

	std::stringstream pass_string;

	if(logger_.isDebug())logger_.logDebug("nPass=", n_pass_, " imagesAutoSavePassCounter=", images_auto_save_params_.pass_counter_, " filmAutoSavePassCounter=", film_load_save_.auto_save_.pass_counter_);

	if(render_control.inProgress())
	{
		if((images_auto_save_params_.interval_type_ == AutoSaveParams::IntervalType::Pass) && (images_auto_save_params_.pass_counter_ >= images_auto_save_params_.interval_passes_))
		{
			for(auto &output : *outputs_)
			{
				if(output.item_) flush(render_control, render_monitor, All);
			}
		}

		if((film_load_save_.mode_ == FilmLoadSave::Mode::LoadAndSave || film_load_save_.mode_ == FilmLoadSave::Mode::Save) && (film_load_save_.auto_save_.interval_type_ == AutoSaveParams::IntervalType::Pass) && (film_load_save_.auto_save_.pass_counter_ >= film_load_save_.auto_save_.interval_passes_))
		{
			imageFilmSave(render_control, render_monitor);
			film_load_save_.auto_save_.pass_counter_ = 0;
		}
	}

	const Image *sampling_factor_image_pass = film_image_layers_(LayerDef::DebugSamplingFactor).image_.get();

	if(n_pass_ == 0) flags_.fill(true);
	else flags_.fill(false);
	const int variance_half_edge = aa_noise_params_->variance_edge_size_ / 2;
	auto combined_image{film_image_layers_(LayerDef::Combined).image_};

	float aa_thresh_scaled = aa_threshold_calculated_;

	int n_resample = 0;

	if(adaptive_aa && aa_threshold_calculated_ > 0.f)
	{
		for(int y = 0; y < params_.height_; ++y)
		{
			for(int x = 0; x < params_.width_; ++x)
			{
				const float weight = weights_({{x, y}}).getFloat();
				if(weight > 0.f) flags_.set({{x, y}}, false);
				else flags_.set({{x, y}}, true); //If after reloading ImageFiles there are pixels that were not yet rendered at all, make sure they are marked to be rendered in the next AA pass
			}
		}

		for(int y = 0; y < params_.height_ - 1; ++y)
		{
			for(int x = 0; x < params_.width_ - 1; ++x)
			{
				//We will only consider the Combined Pass (pass 0) for the AA additional sampling calculations.
				const float weight = weights_({{x, y}}).getFloat();
				float mat_sample_factor = 1.f;
				if(sampling_factor_image_pass)
				{
					mat_sample_factor = weight > 0.f ? sampling_factor_image_pass->getFloat({{x, y}}) / weight : 1.f;
					if(!params_.background_resampling_ && mat_sample_factor == 0.f) continue;
				}

				const Rgba pix_col = combined_image->getColor({{x, y}}).normalized(weight);
				const float pix_col_bri = pix_col.abscol2Bri();

				if(aa_noise_params_->dark_detection_type_ == AaNoiseParams::DarkDetectionType::Linear && aa_noise_params_->dark_threshold_factor_ > 0.f)
				{
					if(aa_noise_params_->dark_threshold_factor_ > 0.f) aa_thresh_scaled = aa_threshold_calculated_ * ((1.f - aa_noise_params_->dark_threshold_factor_) + (pix_col_bri * aa_noise_params_->dark_threshold_factor_));
				}
				else if(aa_noise_params_->dark_detection_type_ == AaNoiseParams::DarkDetectionType::Curve)
				{
					aa_thresh_scaled = darkThresholdCurveInterpolate(pix_col_bri);
				}

				if(pix_col.colorDifference(combined_image->getColor({{x + 1, y}}).normalized(weights_({{x + 1, y}}).getFloat()), aa_noise_params_->detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_.set({{x, y}}, true); flags_.set({{x + 1, y}}, true);
				}
				if(pix_col.colorDifference(combined_image->getColor({{x, y + 1}}).normalized(weights_({{x, y + 1}}).getFloat()), aa_noise_params_->detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_.set({{x, y}}, true); flags_.set({{x, y + 1}}, true);
				}
				if(pix_col.colorDifference(combined_image->getColor({{x + 1, y + 1}}).normalized(weights_({{x + 1, y + 1}}).getFloat()), aa_noise_params_->detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_.set({{x, y}}, true); flags_.set({{x + 1, y + 1}}, true);
				}
				if(x > 0 && pix_col.colorDifference(combined_image->getColor({{x - 1, y + 1}}).normalized(weights_({{x - 1, y + 1}}).getFloat()), aa_noise_params_->detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_.set({{x, y}}, true); flags_.set({{x - 1, y + 1}}, true);
				}

				if(aa_noise_params_->variance_pixels_ > 0)
				{
					int variance_x = 0, variance_y = 0;//, pixelcount = 0;

					//float window_accum = 0.f, window_avg = 0.f;

					for(int xd = -variance_half_edge; xd < variance_half_edge - 1 ; ++xd)
					{
						int xi = x + xd;
						if(xi < 0) xi = 0;
						else if(xi >= params_.width_ - 1) xi = params_.width_ - 2;

						const Rgba cx_0 = combined_image->getColor({{xi, y}}).normalized(weights_({{xi, y}}).getFloat());
						const Rgba cx_1 = combined_image->getColor({{xi + 1, y}}).normalized(weights_({{xi + 1, y}}).getFloat());

						if(cx_0.colorDifference(cx_1, aa_noise_params_->detect_color_noise_) >= aa_thresh_scaled) ++variance_x;
					}

					for(int yd = -variance_half_edge; yd < variance_half_edge - 1 ; ++yd)
					{
						int yi = y + yd;
						if(yi < 0) yi = 0;
						else if(yi >= params_.height_ - 1) yi = params_.height_ - 2;

						const Rgba cy_0 = combined_image->getColor({{x, yi}}).normalized(weights_({{x, yi}}).getFloat());
						const Rgba cy_1 = combined_image->getColor({{x, yi + 1}}).normalized(weights_({{x, yi + 1}}).getFloat());

						if(cy_0.colorDifference(cy_1, aa_noise_params_->detect_color_noise_) >= aa_thresh_scaled) ++variance_y;
					}

					if(variance_x + variance_y >= aa_noise_params_->variance_pixels_)
					{
						for(int xd = -variance_half_edge; xd < variance_half_edge; ++xd)
						{
							for(int yd = -variance_half_edge; yd < variance_half_edge; ++yd)
							{
								int xi = x + xd;
								if(xi < 0) xi = 0;
								else if(xi >= params_.width_) xi = params_.width_ - 1;

								int yi = y + yd;
								if(yi < 0) yi = 0;
								else if(yi >= params_.height_) yi = params_.height_ - 1;

								flags_.set({{xi, yi}}, true);
							}
						}
					}
				}
			}
		}

		for(int y = 0; y < params_.height_; ++y)
		{
			for(int x = 0; x < params_.width_; ++x)
			{
				if(flags_.get({{x, y}}))
				{
					++n_resample;
					if(highlight_pixel_callback_)
					{
						const float weight = weights_({{x, y}}).getFloat();
						const Rgba col = combined_image->getColor({{x, y}}).normalized(weight);
						highlight_pixel_callback_(x, y, col.r_, col.g_, col.b_, col.a_, highlight_pixel_callback_data_);
					}
				}
			}
		}
	}
	else
	{
		n_resample = params_.height_ * params_.width_;
	}

	if(flush_callback_) flush_callback_(flush_callback_data_);

	if(render_control.resumed()) pass_string << "Film loaded + ";

	pass_string << "Rendering pass " << n_pass_ << " of " << aa_noise_params_->passes_ << ", resampling " << n_resample << " pixels.";

	logger_.logInfo(integrator_name, ": ", pass_string.str());

	render_monitor.initProgressBar(params_.width_ * params_.height_, logger_.getConsoleLogColorsEnabled());
	render_monitor.setProgressBarTag(pass_string.str());
	completed_cnt_ = 0;

	return n_resample;
}

bool ImageFilm::nextArea(RenderArea &a)
{
	if(cancel_) return false;

	const int ifilterw = static_cast<int>(std::ceil(filter_width_));

	if(split_)
	{
		const int n = next_area_++;
		if(splitter_->getArea(n, a))
		{
			a.sx_0_ = a.x_ + ifilterw;
			a.sx_1_ = a.x_ + a.w_ - ifilterw;
			a.sy_0_ = a.y_ + ifilterw;
			a.sy_1_ = a.y_ + a.h_ - ifilterw;

			if(highlight_area_callback_)
			{
				const int end_x = a.x_ + a.w_;
				const int end_y = a.y_ + a.h_;
				highlight_area_callback_(a.id_, a.x_, a.y_, end_x, end_y, highlight_area_callback_data_);
			}
			return true;
		}
	}
	else
	{
		if(area_cnt_) return false;
		a.x_ = params_.start_x_;
		a.y_ = params_.start_y_;
		a.w_ = params_.width_;
		a.h_ = params_.height_;
		a.sx_0_ = a.x_ + ifilterw;
		a.sx_1_ = a.x_ + a.w_ - ifilterw;
		a.sy_0_ = a.y_ + ifilterw;
		a.sy_1_ = a.y_ + a.h_ - ifilterw;
		++area_cnt_;
		return true;
	}
	return false;
}

void ImageFilm::finishArea(RenderControl &render_control, RenderMonitor &render_monitor, const RenderArea &a, const EdgeToonParams &edge_params)
{
	std::lock_guard<std::mutex> lock_guard(out_mutex_);
	const int end_x = a.x_ + a.w_ - params_.start_x_;
	const int end_y = a.y_ + a.h_ - params_.start_y_;

	if(layers_.isDefined(LayerDef::DebugFacesEdges))
	{
		image_manipulation::generateDebugFacesEdges(film_image_layers_, a.x_ - params_.start_x_, end_x, a.y_ - params_.start_y_, end_y, true, edge_params, weights_);
	}

	if(layers_.isDefinedAny({LayerDef::DebugObjectsEdges, LayerDef::Toon}))
	{
		image_manipulation::generateToonAndDebugObjectEdges(film_image_layers_, a.x_ - params_.start_x_, end_x, a.y_ - params_.start_y_, end_y, true, edge_params, weights_);
	}

	for(const auto &[layer_def, image_layer] : film_image_layers_)
	{
		if(!image_layer.layer_.isExported()) continue;
		const auto &image{image_layer.image_};
		for(int j = a.y_ - params_.start_y_; j < end_y; ++j)
		{
			for(int i = a.x_ - params_.start_x_; i < end_x; ++i)
			{
				const float weight = weights_({{i, j}}).getFloat();
				Rgba color = (layer_def == LayerDef::AaSamples ? Rgba{weight} : image->getColor({{i, j}}).normalized(weight));
				switch(layer_def)
				{
					//To correct the antialiasing and ceil the "mixed" values to the upper integer in the Object/Material Index layers
					case LayerDef::ObjIndexAbs:
					case LayerDef::ObjIndexAutoAbs:
					case LayerDef::MatIndexAbs:
					case LayerDef::MatIndexAutoAbs: color.ceil(); break;
					default: break;
				}
				exported_image_layers_.setColor({{i, j}}, color, layer_def);
				if(put_pixel_callback_)
				{
					put_pixel_callback_(LayerDef::getName(layer_def).c_str(), i, j, color.r_, color.g_, color.b_, color.a_, put_pixel_callback_data_);
				}
			}
		}
	}

	if(flush_area_callback_) flush_area_callback_(a.id_, a.x_, a.y_, end_x + params_.start_x_, end_y + params_.start_y_, flush_area_callback_data_);

	if(render_control.inProgress())
	{
		render_monitor.stopTimer("imagesAutoSaveTimer");
		images_auto_save_params_.timer_ += render_monitor.getTimerTime("imagesAutoSaveTimer");
		if(images_auto_save_params_.timer_ < 0.f) resetImagesAutoSaveTimer(); //to solve some strange very negative value when using yafaray-xml, race condition somewhere?
		render_monitor.startTimer("imagesAutoSaveTimer");

		render_monitor.stopTimer("filmAutoSaveTimer");
		film_load_save_.auto_save_.timer_ += render_monitor.getTimerTime("filmAutoSaveTimer");
		if(film_load_save_.auto_save_.timer_ < 0.f) resetFilmAutoSaveTimer(); //to solve some strange very negative value when using yafaray-xml, race condition somewhere?
		render_monitor.startTimer("filmAutoSaveTimer");

		if((images_auto_save_params_.interval_type_ == AutoSaveParams::IntervalType::Time) && (images_auto_save_params_.timer_ > images_auto_save_params_.interval_seconds_))
		{
			if(logger_.isDebug())logger_.logDebug("imagesAutoSaveTimer=", images_auto_save_params_.timer_);
			flush(render_control, render_monitor, All);
			resetImagesAutoSaveTimer();
		}

		if((film_load_save_.mode_ == FilmLoadSave::Mode::LoadAndSave || film_load_save_.mode_ == FilmLoadSave::Mode::Save) && (film_load_save_.auto_save_.interval_type_ == AutoSaveParams::IntervalType::Time) && (film_load_save_.auto_save_.timer_ > film_load_save_.auto_save_.interval_seconds_))
		{
			if(logger_.isDebug())logger_.logDebug("filmAutoSaveTimer=", film_load_save_.auto_save_.timer_);
			imageFilmSave(render_control, render_monitor);
			resetFilmAutoSaveTimer();
		}
	}

	if(++completed_cnt_ == area_cnt_) render_monitor.setProgressBarAsDone();
	else render_monitor.updateProgressBar(a.w_ * a.h_);
}

void ImageFilm::flush(RenderControl &render_control, RenderMonitor &render_monitor, Flags flags)
{
	if(render_control.finished())
	{
		logger_.logInfo("imageFilm: Flushing buffer (View '", getName(), "')...");
	}

	float density_factor = 0.f;

	if(estimate_density_ && num_density_samples_ > 0) density_factor = (float) (params_.width_ * params_.height_) / (float) num_density_samples_;

	const Layers layers = layers_.getLayersWithImages();
	if(layers.isDefined(LayerDef::DebugFacesEdges))
	{
		image_manipulation::generateDebugFacesEdges(film_image_layers_, 0, params_.width_, 0, params_.height_, false, *edge_toon_params_, weights_);
	}
	if(layers.isDefinedAny({LayerDef::DebugObjectsEdges, LayerDef::Toon}))
	{
		image_manipulation::generateToonAndDebugObjectEdges(film_image_layers_, 0, params_.width_, 0, params_.height_, false, *edge_toon_params_, weights_);
	}
	for(const auto &[layer_def, image_layer] : film_image_layers_)
	{
		if(!image_layer.layer_.isExported()) continue;
		const auto &image{image_layer.image_};
		for(int j = 0; j < params_.height_; j++)
		{
			for(int i = 0; i < params_.width_; i++)
			{
				const float weight = weights_({{i, j}}).getFloat();
				Rgba color = (layer_def == LayerDef::AaSamples ? Rgba{weight} : image->getColor({{i, j}}).normalized(weight));
				switch(layer_def)
				{
					//To correct the antialiasing and ceil the "mixed" values to the upper integer in the Object/Material Index layers
					case LayerDef::ObjIndexAbs:
					case LayerDef::ObjIndexAutoAbs:
					case LayerDef::MatIndexAbs:
					case LayerDef::MatIndexAutoAbs: color.ceil(); break;
					default: break;
				}
				if(estimate_density_ && (flags & Densityimage) && layer_def == LayerDef::Combined && density_factor > 0.f) color += Rgba((*density_image_)({{i, j}}) * density_factor, 0.f);
				exported_image_layers_.setColor({{i, j}}, color, layer_def);
				if(put_pixel_callback_)
				{
					put_pixel_callback_(LayerDef::getName(layer_def).c_str(), i, j, color.r_, color.g_, color.b_, color.a_, put_pixel_callback_data_);
				}
			}
		}
	}

	if(flush_callback_) flush_callback_(flush_callback_data_);

	if(render_control.finished())
	{
		std::stringstream ss;
		ss << printRenderStats(render_control, render_monitor, Size2i{{params_.width_, params_.height_}});
		ss << render_monitor.getAaNoiseInfo();
		logger_.logParams("--------------------------------------------------------------------------------");
		for(std::string line; std::getline(ss, line, '\n');) if(line != "" && line != "\n") logger_.logParams(line);
		logger_.logParams("--------------------------------------------------------------------------------");
	}

	for(auto &output : *outputs_)
	{
		if(output.item_)
		{
			std::stringstream pass_string;
			pass_string << "Flushing output '" << output.name_ << "' and saving image files.";
			logger_.logInfo(pass_string.str());
			if(render_control.finished())
			{
				std::stringstream ss;
				ss << output.item_->printBadge(render_monitor, render_control);
				//logger_.logParams("--------------------------------------------------------------------------------");
				for(std::string line; std::getline(ss, line, '\n');) if(line != "" && line != "\n") logger_.logParams(line);
				//logger_.logParams("--------------------------------------------------------------------------------");
			}
			std::string old_tag{render_monitor.getProgressBarTag()};
			render_monitor.setProgressBarTag(pass_string.str());
			output.item_->flush(render_monitor, render_control);
			render_monitor.setProgressBarTag(old_tag);
		}
	}

	if(render_control.finished())
	{
		if(film_load_save_.mode_ == FilmLoadSave::Mode::LoadAndSave || film_load_save_.mode_ == FilmLoadSave::Mode::Save)
		{
			imageFilmSave(render_control, render_monitor);
		}

		render_monitor.stopTimer("imagesAutoSaveTimer");
		render_monitor.stopTimer("filmAutoSaveTimer");

		logger_.clearMemoryLog();
		if(logger_.isVerbose()) logger_.logVerbose("imageFilm: Done.");
	}
}

bool ImageFilm::doMoreSamples(const Point2i &point) const
{
	return aa_threshold_calculated_ <= 0.f || flags_.get({{point[Axis::X] - params_.start_x_, point[Axis::Y] - params_.start_y_}});
}

/* CAUTION! Implemantation of this function needs to be thread safe for samples that
	contribute to pixels outside the area a AND pixels that might get
	contributions from outside area a! (yes, really!) */
void ImageFilm::addSample(const Point2i &point, float dx, float dy, const RenderArea *a, int num_sample, int aa_pass_number, float inv_aa_max_possible_samples, const ColorLayers *color_layers)
{
	// get filter extent and make sure we don't leave image area:
	//FIXME: using for some reason an asymmetrical rounding function with a +0.5 bias. Using a standard rounding function would increase processing time due to increased filter applicable area and potentially causing more thread locks. Keeping this asymmetrical rounding for now to keep the original functionality, but probably something to be investigated and made better in the future.
	const int dx_0 = std::max(params_.start_x_ - point[Axis::X], roundToIntWithBias(static_cast<double>(dx) - filter_width_));
	const int dx_1 = std::min((params_.start_x_ + params_.width_ - 1) - point[Axis::X], roundToIntWithBias(static_cast<double>(dx) + filter_width_ - 1.0));
	const int dy_0 = std::max(params_.start_y_ - point[Axis::Y], roundToIntWithBias(static_cast<double>(dy) - filter_width_));
	const int dy_1 = std::min((params_.start_y_ + params_.height_ - 1) - point[Axis::Y], roundToIntWithBias(static_cast<double>(dy) + filter_width_ - 1.0));

	// get indices in filter table
	const double x_offs = dx - 0.5;
	std::array<int, max_filter_size_ + 1> x_index;
	for(int i = dx_0, n = 0; i <= dx_1; ++i, ++n)
	{
		const double d = std::abs((static_cast<double>(i) - x_offs) * filter_table_scale_);
		x_index[n] = static_cast<int>(std::floor(d));
	}
	const double y_offs = dy - 0.5;
	std::array<int, max_filter_size_ + 1> y_index;
	for(int i = dy_0, n = 0; i <= dy_1; ++i, ++n)
	{
		const double d = std::abs((static_cast<double>(i) - y_offs) * filter_table_scale_);
		y_index[n] = static_cast<int>(std::floor(d));
	}

	const int x_0 = point[Axis::X] + dx_0;
	const int x_1 = point[Axis::X] + dx_1;
	const int y_0 = point[Axis::Y] + dy_0;
	const int y_1 = point[Axis::Y] + dy_1;
	const bool outside_thread_safe_area = (x_0 < a->sx_0_ || x_1 > a->sx_1_ || y_0 < a->sy_0_ || y_1 > a->sy_1_);

	if(outside_thread_safe_area) image_mutex_.lock();
	for(int j = y_0; j <= y_1; ++j)
	{
		for(int i = x_0; i <= x_1; ++i)
		{
			// get filter value at pixel (x,y)
			const size_t offset = y_index[j - y_0] * filter_table_size_ + x_index[i - x_0];
			const float filter_wt = filter_table_[offset];
			weights_({{i - params_.start_x_, j - params_.start_y_}}).setFloat(weights_({{i - params_.start_x_, j - params_.start_y_}}).getFloat() + filter_wt);

			// update pixel values with filtered sample contribution
			for(auto &[layer_def, image_layer] : film_image_layers_)
			{
				Rgba col = color_layers ? (*color_layers)(layer_def) : Rgba{0.f};
				col.clampProportionalRgb(aa_noise_params_->clamp_samples_);
				image_layer.image_->addColor({{i - params_.start_x_, j - params_.start_y_}}, col * filter_wt);
			}
		}
	}
	if(outside_thread_safe_area) image_mutex_.unlock();
}

void ImageFilm::addDensitySample(const Rgb &c, const Point2i &point, float dx, float dy)
{
	if(!estimate_density_) return;

	// get filter extent and make sure we don't leave image area:
	//FIXME: using for some reason an asymmetrical rounding function with a +0.5 bias. Using a standard rounding function would increase processing time due to increased filter applicable area and potentially causing more thread locks. Keeping this asymmetrical rounding for now to keep the original functionality, but probably something to be investigated and made better in the future.
	const int dx_0 = std::max(params_.start_x_ - point[Axis::X], roundToIntWithBias(static_cast<double>(dx) - filter_width_));
	const int dx_1 = std::min((params_.start_x_ + params_.width_ - 1) - point[Axis::X], roundToIntWithBias(static_cast<double>(dx) + filter_width_ - 1.0));
	const int dy_0 = std::max(params_.start_y_ - point[Axis::Y], roundToIntWithBias(static_cast<double>(dy) - filter_width_));
	const int dy_1 = std::min((params_.start_y_ + params_.height_ - 1) - point[Axis::Y], roundToIntWithBias(static_cast<double>(dy) + filter_width_ - 1.0));

	const double x_offs = dx - 0.5;
	std::array<int, max_filter_size_ + 1> x_index;
	for(int i = dx_0, n = 0; i <= dx_1; ++i, ++n)
	{
		const double d = std::abs((static_cast<double>(i) - x_offs) * filter_table_scale_);
		x_index[n] = static_cast<int>(std::floor(d));
	}
	const double y_offs = dy - 0.5;
	std::array<int, max_filter_size_ + 1> y_index;
	for(int i = dy_0, n = 0; i <= dy_1; ++i, ++n)
	{
		const double d = std::abs((static_cast<double>(i) - y_offs) * filter_table_scale_);
		y_index[n] = static_cast<int>(std::floor(d));
	}

	const int x_0 = point[Axis::X] + dx_0;
	const int x_1 = point[Axis::X] + dx_1;
	const int y_0 = point[Axis::Y] + dy_0;
	const int y_1 = point[Axis::Y] + dy_1;

	std::lock_guard<std::mutex> lock_guard(density_image_mutex_);
	for(int j = y_0; j <= y_1; ++j)
	{
		for(int i = x_0; i <= x_1; ++i)
		{
			const size_t offset = y_index[j - y_0] * filter_table_size_ + x_index[i - x_0];
			Rgb &pixel = (*density_image_)({{i - params_.start_x_, j - params_.start_y_}});
			pixel += c * filter_table_[offset];
		}
	}
	++num_density_samples_;
}

void ImageFilm::setDensityEstimation(bool enable)
{
	if(enable)
	{
		if(!density_image_) density_image_ = std::make_unique<Buffer2D<Rgb>>(Size2i{{params_.width_, params_.height_}});
		else density_image_->clear();
	}
	else
	{
		density_image_ = nullptr;
	}
	estimate_density_ = enable;
}

float ImageFilm::darkThresholdCurveInterpolate(float pixel_brightness)
{
	if(pixel_brightness <= 0.10f) return 0.0001f;
	else if(pixel_brightness <= 0.20f) return (0.0001f + (pixel_brightness - 0.10f) * (0.0010f - 0.0001f) / 0.10f);
	else if(pixel_brightness <= 0.30f) return (0.0010f + (pixel_brightness - 0.20f) * (0.0020f - 0.0010f) / 0.10f);
	else if(pixel_brightness <= 0.40f) return (0.0020f + (pixel_brightness - 0.30f) * (0.0035f - 0.0020f) / 0.10f);
	else if(pixel_brightness <= 0.50f) return (0.0035f + (pixel_brightness - 0.40f) * (0.0055f - 0.0035f) / 0.10f);
	else if(pixel_brightness <= 0.60f) return (0.0055f + (pixel_brightness - 0.50f) * (0.0075f - 0.0055f) / 0.10f);
	else if(pixel_brightness <= 0.70f) return (0.0075f + (pixel_brightness - 0.60f) * (0.0100f - 0.0075f) / 0.10f);
	else if(pixel_brightness <= 0.80f) return (0.0100f + (pixel_brightness - 0.70f) * (0.0150f - 0.0100f) / 0.10f);
	else if(pixel_brightness <= 0.90f) return (0.0150f + (pixel_brightness - 0.80f) * (0.0250f - 0.0150f) / 0.10f);
	else if(pixel_brightness <= 1.00f) return (0.0250f + (pixel_brightness - 0.90f) * (0.0400f - 0.0250f) / 0.10f);
	else if(pixel_brightness <= 1.20f) return (0.0400f + (pixel_brightness - 1.00f) * (0.0800f - 0.0400f) / 0.20f);
	else if(pixel_brightness <= 1.40f) return (0.0800f + (pixel_brightness - 1.20f) * (0.0950f - 0.0800f) / 0.20f);
	else if(pixel_brightness <= 1.80f) return (0.0950f + (pixel_brightness - 1.40f) * (0.1000f - 0.0950f) / 0.40f);
	else return 0.1000f;
}

std::string ImageFilm::getFilmPath() const
{
	std::string film_path = film_load_save_.path_;
	std::stringstream node;
	node << std::setfill('0') << std::setw(4) << params_.computer_node_;
	film_path += " - node " + node.str();
	film_path += ".film";
	return film_path;
}

bool ImageFilm::imageFilmLoad(const std::string &filename)
{
	logger_.logInfo("imageFilm: Loading film from: \"", filename);

	File file(filename);
	if(!file.open("rb"))
	{
		logger_.logWarning("imageFilm file '", filename, "' not found, canceling load operation");
		return false;
	}

	std::string header;
	file.read(header);
	if(header != "YAF_FILMv4_0_0")
	{
		logger_.logWarning("imageFilm file '", filename, "' does not contain a valid YafaRay image file");
		file.close();
		return false;
	}
	file.read<int>(computer_node_);
	file.read<int>(base_sampling_offset_);
	file.read<int>(sampling_offset_);

	int filmload_check_w;
	file.read<int>(filmload_check_w);
	if(filmload_check_w != params_.width_)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Image width, expected=", params_.width_, ", in reused/loaded film=", filmload_check_w);
		return false;
	}

	int filmload_check_h;
	file.read<int>(filmload_check_h);
	if(filmload_check_h != params_.height_)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Image height, expected=", params_.height_, ", in reused/loaded film=", filmload_check_h);
		return false;
	}

	int filmload_check_cx_0;
	file.read<int>(filmload_check_cx_0);
	if(filmload_check_cx_0 != params_.start_x_)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Border cx0, expected=", params_.start_x_, ", in reused/loaded film=", filmload_check_cx_0);
		return false;
	}

	int filmload_check_cx_1;
	file.read<int>(filmload_check_cx_1);
	if(filmload_check_cx_1 != (params_.start_x_ + params_.width_ - 1))
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Border cx1, expected=", (params_.start_x_ + params_.width_ - 1), ", in reused/loaded film=", filmload_check_cx_1);
		return false;
	}

	int filmload_check_cy_0;
	file.read<int>(filmload_check_cy_0);
	if(filmload_check_cy_0 != params_.start_y_)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Border cy0, expected=", params_.start_y_, ", in reused/loaded film=", filmload_check_cy_0);
		return false;
	}

	int filmload_check_cy_1;
	file.read<int>(filmload_check_cy_1);
	if(filmload_check_cy_1 != (params_.start_y_ + params_.height_ - 1))
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Border cy1, expected=", (params_.start_y_ + params_.height_ - 1), ", in reused/loaded film=", filmload_check_cy_1);
		return false;
	}

	int loaded_image_num_layers;
	file.read<int>(loaded_image_num_layers);
	initLayersImages();
	//If there are any ImageOutputs, creation of the image buffers for the image outputs exported images
	if(!outputs_->empty()) initLayersExportedImages();
	const int num_layers = film_image_layers_.size();
	if(loaded_image_num_layers != num_layers)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Number of image layers, expected=", num_layers, ", in reused/loaded film=", loaded_image_num_layers);
		return false;
	}

	for(int y = 0; y < params_.height_; ++y)
	{
		for(int x = 0; x < params_.width_; ++x)
		{
			float weight;
			file.read<float>(weight);
			weights_({{x, y}}).setFloat(weight);
		}
	}

	for(auto &[layer_def, image_layer] : film_image_layers_)
	{
		for(int y = 0; y < params_.height_; ++y)
		{
			for(int x = 0; x < params_.width_; ++x)
			{
				Rgba col;
				file.read<float>(col.r_);
				file.read<float>(col.g_);
				file.read<float>(col.b_);
				file.read<float>(col.a_);
				image_layer.image_->setColor({{x, y}}, col);
			}
		}
	}

	file.close();
	return true;
}

void ImageFilm::imageFilmLoadAllInFolder(RenderControl &render_control, RenderMonitor &render_monitor)
{
	std::stringstream pass_string;
	pass_string << "Loading ImageFilm files";

	logger_.logInfo(pass_string.str());

	std::string old_tag{render_monitor.getProgressBarTag()};
	render_monitor.setProgressBarTag(pass_string.str());

	const Path path_image_output(film_load_save_.path_);
	std::string dir = path_image_output.getDirectory();
	if(dir.empty()) dir = ".";	//If parent path is empty, set the path to the current folder
	const std::vector<std::string> files_list = File::listFiles(dir);

	const std::string base_image_file_name = path_image_output.getBaseName();
	std::vector<std::string> film_file_paths_list;
	for(const auto &file_name : files_list)
	{
		//if(logger_.isDebug()) Y_DEBUG PR(fileName");
		if(File::exists(dir + "//" + file_name, true))
		{
			const Path file_path {file_name };
			const std::string ext = file_path.getExtension();
			const std::string base = file_path.getBaseName();
			const int base_rfind_result = base.rfind(base_image_file_name, 0);

			//if(logger_.isDebug()) Y_DEBUG PR(baseImageFileName) PR(fileName) PR(base) PR(ext) PR(base_rfind_result");
			if(ext == "film" && base_rfind_result == 0)
			{
				film_file_paths_list.emplace_back(dir + "//" + file_name);
				//if(logger_.isDebug())logger_.logDebug("Added: " << dir + "//" + fileName);
			}
		}
	}

	std::sort(film_file_paths_list.begin(), film_file_paths_list.end());
	bool any_film_loaded = false;
	for(const auto &film_file : film_file_paths_list)
	{
		ParamResult param_result;
		auto loaded_film = std::make_unique<ImageFilm>(logger_, param_result, film_file, getAsParamMap(true));
		if(!loaded_film->imageFilmLoad(film_file))
		{
			logger_.logWarning("ImageFilm: Could not load film file '", film_file, "'");
			continue;
		}
		else any_film_loaded = true;

		for(int i = 0; i < params_.width_; ++i)
		{
			for(int j = 0; j < params_.height_; ++j)
			{
				weights_({{i, j}}).setFloat(weights_({{i, j}}).getFloat() + loaded_film->weights_({{i, j}}).getFloat());
			}
		}

		for(auto &[layer_def, image_layer] : film_image_layers_)
		{
			const ImageLayers &loaded_image_layers = loaded_film->film_image_layers_;
			for(int i = 0; i < params_.width_; ++i)
			{
				for(int j = 0; j < params_.height_; ++j)
				{
					image_layer.image_->addColor({{i, j}}, loaded_image_layers(layer_def).image_->getColor({{i, j}}));
				}
			}
		}
		if(sampling_offset_ < loaded_film->sampling_offset_) sampling_offset_ = loaded_film->sampling_offset_;
		if(base_sampling_offset_ < loaded_film->base_sampling_offset_) base_sampling_offset_ = loaded_film->base_sampling_offset_;
		if(logger_.isVerbose()) logger_.logVerbose("ImageFilm: loaded film '", film_file, "'");
	}
	if(any_film_loaded) render_control.setResumed();
	render_monitor.setProgressBarTag(old_tag);
}

bool ImageFilm::imageFilmSave(RenderControl &render_control, RenderMonitor &render_monitor)
{
	bool result_ok = true;
	std::stringstream pass_string;
	pass_string << "Saving internal ImageFilm file";

	logger_.logInfo(pass_string.str());

	std::string old_tag{render_monitor.getProgressBarTag()};
	render_monitor.setProgressBarTag(pass_string.str());

	const std::string film_path = getFilmPath();
	File file(film_path);
	file.open("wb");
	file.append(std::string("YAF_FILMv4_0_0"));
	file.append<int>(computer_node_);
	file.append<int>(base_sampling_offset_);
	file.append<int>(sampling_offset_);
	file.append<int>(params_.width_);
	file.append<int>(params_.height_);
	file.append<int>(params_.start_x_);
	file.append<int>(params_.start_x_ + params_.width_ - 1);
	file.append<int>(params_.start_y_);
	file.append<int>(params_.start_y_ + params_.height_ - 1);
	file.append<int>((int) film_image_layers_.size());

	const int weights_w = weights_.getWidth();
	if(weights_w != params_.width_)
	{
		logger_.logWarning("ImageFilm saving problems, film weights width ", params_.width_, " different from internal 2D image width ", weights_w);
		result_ok = false;
	}
	const int weights_h = weights_.getHeight();
	if(weights_h != params_.height_)
	{
		logger_.logWarning("ImageFilm saving problems, film weights height ", params_.height_, " different from internal 2D image height ", weights_h);
		result_ok = false;
	}
	for(int y = 0; y < params_.height_; ++y)
	{
		for(int x = 0; x < params_.width_; ++x)
		{
			file.append<float>(weights_({{x, y}}).getFloat());
		}
	}

	for(const auto &[layer_def, image_layer] : film_image_layers_)
	{
		const int img_w = image_layer.image_->getWidth();
		if(img_w != params_.width_)
		{
			logger_.logWarning("ImageFilm saving problems, film width ", params_.width_, " different from internal 2D image width ", img_w);
			result_ok = false;
			break;
		}
		const int img_h = image_layer.image_->getHeight();
		if(img_h != params_.height_)
		{
			logger_.logWarning("ImageFilm saving problems, film height ", params_.height_, " different from internal 2D image height ", img_h);
			result_ok = false;
			break;
		}
		for(int y = 0; y < params_.height_; ++y)
		{
			for(int x = 0; x < params_.width_; ++x)
			{
				const Rgba &col = image_layer.image_->getColor({{x, y}});
				file.append<float>(col.r_);
				file.append<float>(col.g_);
				file.append<float>(col.b_);
				file.append<float>(col.a_);
			}
		}
	}
	file.close();
	render_monitor.setProgressBarTag(old_tag);
	return result_ok;
}

void ImageFilm::imageFilmFileBackup(RenderControl &render_control, RenderMonitor &render_monitor)
{
	std::stringstream pass_string;
	pass_string << "Creating backup of the previous ImageFilm file...";

	logger_.logInfo(pass_string.str());

	std::string old_tag{render_monitor.getProgressBarTag()};
	render_monitor.setProgressBarTag(pass_string.str());

	const std::string film_path = getFilmPath();
	const std::string film_path_backup = film_path + "-previous.bak";

	if(File::exists(film_path, true))
	{
		if(logger_.isVerbose()) logger_.logVerbose("imageFilm: Creating backup of previously saved film to: \"", film_path_backup, "\"");
		const bool result_ok = File::rename(film_path, film_path_backup, true, true);
		if(!result_ok) logger_.logWarning("imageFilm: error during imageFilm file backup");
	}

	render_monitor.setProgressBarTag(old_tag);
}

std::string ImageFilm::printRenderStats(const RenderControl &render_control, const RenderMonitor &render_monitor, const Size2i &size)
{
	std::stringstream ss;
	ss << "\nYafaRay (" << buildinfo::getVersionString() << buildinfo::getBuildTypeSuffix() << ")" << " " << buildinfo::getBuildOs() << " " << buildinfo::getBuildArchitectureBits() << "bit (" << buildinfo::getBuildCompiler() << ")";
	ss << std::setprecision(2);

	double times = render_monitor.getTimerTimeNotStopping("rendert");
	if(render_control.finished()) times = render_monitor.getTimerTime("rendert");
	int timem, timeh;
	Timer::splitTime(times, &times, &timem, &timeh);
	ss << " | " << size[Axis::X] << "x" << size[Axis::Y];
	if(render_control.inProgress()) ss << " | " << (render_control.resumed() ? "film loaded + " : "") << "in progress " << std::fixed << std::setprecision(1) << render_monitor.currentPassPercent() << "% of pass: " << render_monitor.currentPass() << " / " << render_monitor.totalPasses();
	else if(render_control.canceled()) ss << " | " << (render_control.resumed() ? "film loaded + " : "") << "stopped at " << std::fixed << std::setprecision(1) << render_monitor.currentPassPercent() << "% of pass: " << render_monitor.currentPass() << " / " << render_monitor.totalPasses();
	else
	{
		if(render_control.resumed()) ss << " | film loaded + " << render_monitor.totalPasses() - 1 << " passes";
		else ss << " | " << render_monitor.totalPasses() << " passes";
	}
	//if(cx0 != 0) ssBadge << ", xstart=" << cx0;
	//if(cy0 != 0) ssBadge << ", ystart=" << cy0;
	ss << " | Render time:";
	if(timeh > 0) ss << " " << timeh << "h";
	if(timem > 0) ss << " " << timem << "m";
	ss << " " << times << "s";

	times = render_monitor.getTimerTimeNotStopping("rendert") + render_monitor.getTimerTime("prepass");
	if(render_control.finished()) times = render_monitor.getTimerTime("rendert") + render_monitor.getTimerTime("prepass");
	Timer::splitTime(times, &times, &timem, &timeh);
	ss << " | Total time:";
	if(timeh > 0) ss << " " << timeh << "h";
	if(timem > 0) ss << " " << timem << "m";
	ss << " " << times << "s";
	return ss.str();
}

void ImageFilm::defineLayer(const ParamMap &param_map)
{
	if(logger_.isDebug()) logger_.logDebug("**ImageFilm::defineLayer 'raw' ParamMap\n" + param_map.logContents());
	std::string layer_type_name, image_type_name, exported_image_name, exported_image_type_name;
	param_map.getParam("type", layer_type_name);
	param_map.getParam("image_type", image_type_name);
	param_map.getParam("exported_image_name", exported_image_name);
	param_map.getParam("exported_image_type", exported_image_type_name);
	defineLayer(std::move(layer_type_name), std::move(image_type_name), std::move(exported_image_type_name), std::move(exported_image_name));
}

void ImageFilm::defineLayer(std::string &&layer_type_name, std::string &&image_type_name, std::string &&exported_image_type_name, std::string &&exported_image_name)
{
	const LayerDef::Type layer_type = LayerDef::getType(layer_type_name);
	const Image::Type image_type = image_type_name.empty() ? LayerDef::getDefaultImageType(layer_type) : Image::getTypeFromName(image_type_name);
	const Image::Type exported_image_type = Image::getTypeFromName(exported_image_type_name);
	defineLayer(layer_type, image_type, exported_image_type, exported_image_name);
}

void ImageFilm::defineLayer(LayerDef::Type layer_type, Image::Type image_type, Image::Type exported_image_type, const std::string &exported_image_name)
{
	if(layer_type == LayerDef::Disabled)
	{
		logger_.logWarning(getClassName(), " '", getName(), "': cannot create layer '", LayerDef::getName(layer_type), "' of unknown or disabled layer type");
		return;
	}

	if(Layer *existing_layer = layers_.find(layer_type))
	{
		if(existing_layer->getType() == layer_type &&
		   existing_layer->getImageType() == image_type &&
		   existing_layer->getExportedImageType() == exported_image_type) return;

		if(logger_.isDebug())logger_.logDebug(getClassName(), " '", getName(), "': had previously defined: ", existing_layer->print());
		if(image_type == Image::Type::None && existing_layer->getImageType() != Image::Type::None)
		{
			if(logger_.isDebug())logger_.logDebug(getClassName(), " '", getName(), "': the layer '", LayerDef::getName(layer_type), "' had previously a defined internal image which cannot be removed.");
		}
		else existing_layer->setImageType(image_type);

		if(exported_image_type == Image::Type::None && existing_layer->getExportedImageType() != Image::Type::None)
		{
			if(logger_.isDebug())logger_.logDebug(getClassName(), " '", getName(), "': the layer '", LayerDef::getName(layer_type), "' was previously an exported layer and cannot be changed into an internal layer now.");
		}
		else
		{
			existing_layer->setExportedImageType(exported_image_type);
			existing_layer->setExportedImageName(exported_image_name);
		}
		existing_layer->setType(layer_type);
		logger_.logInfo(getClassName(), " '", getName(), "': layer redefined: " + existing_layer->print());
	}
	else
	{
		Layer new_layer(layer_type, image_type, exported_image_type, exported_image_name);
		layers_.set(layer_type, new_layer);
		logger_.logInfo(getClassName(), " '", getName(), "': layer defined: ", new_layer.print());
	}
}

void ImageFilm::defineBasicLayers()
{
	//by default we will have an external/internal Combined layer
	if(!layers_.isDefined(LayerDef::Combined)) defineLayer(LayerDef::Combined, Image::Type::ColorAlpha, Image::Type::ColorAlpha);

	//This auxiliary layer will always be needed for material-specific number of samples calculation
	if(!layers_.isDefined(LayerDef::DebugSamplingFactor)) defineLayer(LayerDef::DebugSamplingFactor, Image::Type::Gray);
}

void ImageFilm::defineDependentLayers()
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

void ImageFilm::setRenderNotifyLayerCallback(yafaray_FilmNotifyLayerCallback callback, void *callback_data)
{
	notify_layer_callback_ = callback;
	notify_layer_callback_data_ = callback_data;
}

void ImageFilm::setRenderPutPixelCallback(yafaray_FilmPutPixelCallback callback, void *callback_data)
{
	put_pixel_callback_ = callback;
	put_pixel_callback_data_ = callback_data;
}

void ImageFilm::setRenderHighlightPixelCallback(yafaray_FilmHighlightPixelCallback callback, void *callback_data)
{
	highlight_pixel_callback_ = callback;
	highlight_pixel_callback_data_ = callback_data;
}

void ImageFilm::setRenderFlushAreaCallback(yafaray_FilmFlushAreaCallback callback, void *callback_data)
{
	flush_area_callback_ = callback;
	flush_area_callback_data_ = callback_data;
}

void ImageFilm::setRenderFlushCallback(yafaray_FilmFlushCallback callback, void *callback_data)
{
	flush_callback_ = callback;
	flush_callback_data_ = callback_data;
}

void ImageFilm::setRenderHighlightAreaCallback(yafaray_FilmHighlightAreaCallback callback, void *callback_data)
{
	highlight_area_callback_ = callback;
	highlight_area_callback_data_ = callback_data;
}

void ImageFilm::clearOutputs()
{
	outputs_->clear();
}

bool ImageFilm::disableOutput(const std::string &name)
{
	const auto result{outputs_->disable(name)};
	return result.isOk();
}

std::pair<size_t, ParamResult> ImageFilm::createOutput(const std::string &name, const ParamMap &param_map)
{
	auto result{Items<ImageOutput>::createItem<ImageFilm>(logger_, *outputs_, name, param_map, *this)};
	return result;
}

ParamResult ImageFilm::defineCamera(const std::string &name, const ParamMap &param_map)
{
	auto [camera, camera_integrator_result]{Camera::factory(logger_, name, param_map)};
	if(logger_.isVerbose() && camera)
	{
		logger_.logVerbose(getClassName(), " '", getName(), "': Added ", camera->getClassName(), " '", name, "' (", camera->type().print(), ")!");
	}
	//logger_.logParams(result.first->getAsParamMap(true).print()); //TEST CODE ONLY, REMOVE!!
	camera_ = std::move(camera);
	return camera_integrator_result;
}

} //namespace yafaray
