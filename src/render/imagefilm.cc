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
#include "image/image_output.h"
#include "format/format.h"
#include "scene/scene.h"
#include "common/file.h"
#include "common/param.h"
#include "render/progress_bar.h"
#include "common/timer.h"
#include "color/color_layers.h"
#include "math/filter.h"
#include "common/version_build_info.h"
#include "image/image_manipulation.h"

BEGIN_YAFARAY

constexpr int ImageFilm::filter_table_size_;
constexpr int ImageFilm::max_filter_size_;

typedef float FilterFunc_t(float dx, float dy);


ImageFilm * ImageFilm::factory(Logger &logger, const ParamMap &params, Scene *scene)
{
	if(logger.isDebug())
	{
		logger.logDebug("**ImageFilm::factory");
		params.logContents(logger);
	}
	std::string name;
	std::string tiles_order;
	int width = 320, height = 240, xstart = 0, ystart = 0;
	float filt_sz = 1.5;
	int tile_size = 32;
	std::string images_autosave_interval_type_string = "none";
	ImageFilm::AutoSaveParams images_autosave_params;
	std::string film_load_save_mode_str = "none";
	std::string film_autosave_interval_type_str = "none";
	ImageFilm::FilmLoadSave film_load_save;

	params.getParam("AA_pixelwidth", filt_sz);
	params.getParam("width", width); // width of rendered image
	params.getParam("height", height); // height of rendered image
	params.getParam("xstart", xstart); // x-offset (for cropped rendering)
	params.getParam("ystart", ystart); // y-offset (for cropped rendering)
	params.getParam("filter_type", name); // AA filter type
	params.getParam("tile_size", tile_size); // Size of the render buckets or tiles
	params.getParam("tiles_order", tiles_order); // Order of the render buckets or tiles
	params.getParam("images_autosave_interval_type", images_autosave_interval_type_string);
	params.getParam("images_autosave_interval_passes", images_autosave_params.interval_passes_);
	params.getParam("images_autosave_interval_seconds", images_autosave_params.interval_seconds_);
	params.getParam("film_load_save_mode", film_load_save_mode_str);
	params.getParam("film_load_save_path", film_load_save.path_);
	params.getParam("film_autosave_interval_type", film_autosave_interval_type_str);
	params.getParam("film_autosave_interval_passes", film_load_save.auto_save_.interval_passes_);
	params.getParam("film_autosave_interval_seconds", film_load_save.auto_save_.interval_seconds_);

	if(logger.isDebug())logger.logDebug("Images autosave: ", images_autosave_interval_type_string, ", ", images_autosave_params.interval_passes_, ", ", images_autosave_params.interval_seconds_);

	if(images_autosave_interval_type_string == "pass-interval") images_autosave_params.interval_type_ = ImageFilm::ImageFilm::AutoSaveParams::IntervalType::Pass;
	else if(images_autosave_interval_type_string == "time-interval") images_autosave_params.interval_type_ = ImageFilm::AutoSaveParams::IntervalType::Time;
	else images_autosave_params.interval_type_ = ImageFilm::AutoSaveParams::IntervalType::None;


	if(logger.isDebug())logger.logDebug("ImageFilm load/save mode: ", film_load_save_mode_str, ", path:'", film_load_save.path_, "', interval: ", film_autosave_interval_type_str, ", ", film_load_save.auto_save_.interval_passes_, ", ", film_load_save.auto_save_.interval_seconds_);

	if(film_load_save_mode_str == "load-save") film_load_save.mode_ = ImageFilm::FilmLoadSave::LoadAndSave;
	else if(film_load_save_mode_str == "save") film_load_save.mode_ = ImageFilm::FilmLoadSave::Save;
	else film_load_save.mode_ = ImageFilm::FilmLoadSave::None;

	if(film_autosave_interval_type_str == "pass-interval") film_load_save.auto_save_.interval_type_ = ImageFilm::AutoSaveParams::IntervalType::Pass;
	else if(film_autosave_interval_type_str == "time-interval") film_load_save.auto_save_.interval_type_ = ImageFilm::AutoSaveParams::IntervalType::Time;
	else film_load_save.auto_save_.interval_type_ = ImageFilm::AutoSaveParams::IntervalType::None;

	ImageFilm::FilterType type = ImageFilm::FilterType::Box;
	if(name == "mitchell") type = ImageFilm::FilterType::Mitchell;
	else if(name == "gauss") type = ImageFilm::FilterType::Gauss;
	else if(name == "lanczos") type = ImageFilm::FilterType::Lanczos;
	else if(name != "box") logger.logWarning("ImageFilm: ", "No AA filter defined defaulting to Box!");

	ImageSplitter::TilesOrderType tiles_order_type = ImageSplitter::CentreRandom;
	if(tiles_order == "linear") tiles_order_type = ImageSplitter::Linear;
	else if(tiles_order == "random") tiles_order_type = ImageSplitter::Random;
	else if(tiles_order != "centre" && logger.isVerbose()) logger.logVerbose("ImageFilm: ", "Defaulting to Centre tiles order."); // this is info imho not a warning

	auto film = new ImageFilm(logger, width, height, xstart, ystart, scene->getNumThreads(), scene->getRenderControl(), *scene->getLayers(), scene->getOutputs(), filt_sz, type, tile_size, tiles_order_type);

	film->setImagesAutoSaveParams(images_autosave_params);
	film->setFilmLoadSaveParams(film_load_save);

	if(images_autosave_params.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Pass) logger.logInfo("ImageFilm: ", "AutoSave partially rendered image every ", images_autosave_params.interval_passes_, " passes");

	if(images_autosave_params.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Time) logger.logInfo("ImageFilm: ", "AutoSave partially rendered image every ", images_autosave_params.interval_seconds_, " seconds");

	if(film_load_save.mode_ != ImageFilm::FilmLoadSave::Save) logger.logInfo("ImageFilm: ", "Enabling imageFilm file saving feature");
	if(film_load_save.mode_ == ImageFilm::FilmLoadSave::LoadAndSave) logger.logInfo("ImageFilm: ", "Enabling imageFilm Loading feature. It will load and combine the ImageFilm files from the currently selected image output folder before start rendering, autodetecting each film format (binary/text) automatically. If they don't match exactly the scene, bad results could happen. Use WITH CARE!");

	if(film_load_save.auto_save_.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Pass) logger.logInfo("ImageFilm: ", "AutoSave internal imageFilm every ", film_load_save.auto_save_.interval_passes_, " passes");

	if(film_load_save.auto_save_.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Time) logger.logInfo("ImageFilm: ", "AutoSave internal imageFilm image every ", film_load_save.auto_save_.interval_seconds_, " seconds");

	film->render_views_ = &scene->getRenderViews();
	film->render_callbacks_ = &scene->getRenderCallbacks();
	return film;
}

ImageFilm::ImageFilm(Logger &logger, int width, int height, int xstart, int ystart, int num_threads, RenderControl &render_control, const Layers &layers, const std::map<std::string, std::unique_ptr<ImageOutput>> &outputs, float filter_size, FilterType filt, int t_size, ImageSplitter::TilesOrderType tiles_order_type) : width_(width), height_(height), cx_0_(xstart), cy_0_(ystart), tile_size_(t_size), tiles_order_(tiles_order_type), num_threads_(num_threads), layers_(layers), outputs_(outputs), filterw_(filter_size * 0.5), flags_(width, height), weights_(width, height), logger_(logger)
{
	cx_1_ = xstart + width;
	cy_1_ = ystart + height;
	filter_table_ = std::unique_ptr<float[]>(new float[filter_table_size_ * filter_table_size_]);

	estimate_density_ = false;

	// fill filter table:
	float *f_tp = filter_table_.get();
	const float scale = 1.f / static_cast<float>(filter_table_size_);

	FilterFunc_t *ffunc = nullptr;
	switch(filt)
	{
		case ImageFilm::FilterType::Mitchell: ffunc = math::filter::mitchell; filterw_ *= 2.6f; break;
		case ImageFilm::FilterType::Lanczos: ffunc = math::filter::lanczos2; break;
		case ImageFilm::FilterType::Gauss: ffunc = math::filter::gauss; filterw_ *= 2.f; break;
		default:
		case ImageFilm::FilterType::Box: ffunc = math::filter::box; break;
	}

	filterw_ = std::min(std::max(0.501f, filterw_), 0.5f * max_filter_size_); // filter needs to cover at least the area of one pixel and no more than MAX_FILTER_SIZE/2

	for(int y = 0; y < filter_table_size_; ++y)
	{
		for(int x = 0; x < filter_table_size_; ++x)
		{
			*f_tp = ffunc((x + .5f) * scale, (y + .5f) * scale);
			++f_tp;
		}
	}

	table_scale_ = 0.9999 * filter_table_size_ / filterw_;
	area_cnt_ = 0;

	progress_bar_ = std::unique_ptr<ProgressBar>(new ConsoleProgressBar(80));
	render_control.setCurrentPassPercent(progress_bar_->getPercent());

	aa_noise_params_.detect_color_noise_ = false;
	aa_noise_params_.dark_threshold_factor_ = 0.f;
	aa_noise_params_.variance_edge_size_ = 10;
	aa_noise_params_.variance_pixels_ = 0;
	aa_noise_params_.clamp_samples_ = 0.f;
}

void ImageFilm::initLayersImages()
{
	for(const auto &[layer_def, layer] : layers_.getLayersWithImages())
	{
		Image::Type image_type = layer.getImageType();
		image_type = Image::imageTypeWithAlpha(image_type); //Alpha channel is needed in all images of the weight normalization process will cause problems
		std::unique_ptr<Image> image(Image::factory(logger_, width_, height_, image_type, Image::Optimization::None));
		film_image_layers_.set(layer_def, {std::move(image), layer});
	}
}

void ImageFilm::initLayersExportedImages()
{
	for(const auto &[layer_def, layer]: layers_.getLayersWithExportedImages())
	{
		Image::Type image_type = layer.getImageType();
		image_type = Image::imageTypeWithAlpha(image_type); //Alpha channel is needed in all images of the weight normalization process will cause problems
		std::unique_ptr<Image> image(Image::factory(logger_, width_, height_, image_type, Image::Optimization::None));
		exported_image_layers_.set(layer_def, {std::move(image), layer});
	}
}

void ImageFilm::init(RenderControl &render_control, int num_passes)
{
	//Creation of the image buffers for the render passes
	film_image_layers_.clear();
	exported_image_layers_.clear();
	initLayersImages();
	//If there are any ImageOutputs, creation of the image buffers for the image outputs exported images
	if(!outputs_.empty()) initLayersExportedImages();

	// Clear density image
	if(estimate_density_)
	{
		density_image_ = std::unique_ptr<ImageBuffer2D<Rgb>>(new ImageBuffer2D<Rgb>(width_, height_));
	}

	// Setup the bucket splitter
	if(split_)
	{
		next_area_ = 0;
		splitter_ = std::unique_ptr<ImageSplitter>(new ImageSplitter(width_, height_, cx_0_, cy_0_, tile_size_, tiles_order_, num_threads_));
		area_cnt_ = splitter_->size();
	}
	else area_cnt_ = 1;

	if(progress_bar_) progress_bar_->init(width_ * height_, logger_.getConsoleLogColorsEnabled());
	render_control.setCurrentPassPercent(progress_bar_->getPercent());

	cancel_ = false;
	completed_cnt_ = 0;
	n_pass_ = 1;
	n_passes_ = num_passes;

	images_auto_save_params_.pass_counter_ = 0;
	film_load_save_.auto_save_.pass_counter_ = 0;
	resetImagesAutoSaveTimer();
	resetFilmAutoSaveTimer();

	timer_.addEvent("imagesAutoSaveTimer");
	timer_.addEvent("filmAutoSaveTimer");
	timer_.start("imagesAutoSaveTimer");
	timer_.start("filmAutoSaveTimer");

	if(film_load_save_.mode_ == FilmLoadSave::LoadAndSave) imageFilmLoadAllInFolder(render_control);	//Load all the existing Film in the images output folder, combining them together. It will load only the Film files with the same "base name" as the output image film (including file name, computer node name and frame) to allow adding samples to animations.
	if(film_load_save_.mode_ == FilmLoadSave::LoadAndSave || film_load_save_.mode_ == FilmLoadSave::Save) imageFilmFileBackup(); //If the imageFilm is set to Save, at the start rename the previous film file as a "backup" just in case the user has made a mistake and wants to get the previous film back.

	if(render_callbacks_ && render_callbacks_->notify_view_)
	{
		for(const auto &[render_view_name, render_view]: *render_views_)
		{
			render_callbacks_->notify_view_(render_view_name.c_str(), render_callbacks_->notify_view_data_);
		}
	}
	if(render_callbacks_ && render_callbacks_->notify_layer_)
	{
		const Layers &layers = layers_.getLayersWithExportedImages();
		for(const auto &[layer_type, layer] : layers)
		{
			render_callbacks_->notify_layer_(LayerDef::getName(layer_type).c_str(), layer.getExportedImageName().c_str(), width_, height_, layer.getNumExportedChannels(), render_callbacks_->notify_layer_data_);
		}
	}
}

int ImageFilm::nextPass(const RenderView *render_view, RenderControl &render_control, bool adaptive_aa, const std::string &integrator_name, const EdgeToonParams &edge_params, bool skip_nrender_layer)
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
		if((images_auto_save_params_.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Pass) && (images_auto_save_params_.pass_counter_ >= images_auto_save_params_.interval_passes_))
		{
			for(auto &[output_name, output] : outputs_)
			{
				if(output) flush(render_view, render_control, edge_params, All);
			}
		}

		if((film_load_save_.mode_ == FilmLoadSave::LoadAndSave || film_load_save_.mode_ == FilmLoadSave::Save) && (film_load_save_.auto_save_.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Pass) && (film_load_save_.auto_save_.pass_counter_ >= film_load_save_.auto_save_.interval_passes_))
		{
				imageFilmSave();
				film_load_save_.auto_save_.pass_counter_ = 0;
		}
	}

	const Image *sampling_factor_image_pass = film_image_layers_(LayerDef::DebugSamplingFactor).image_.get();

	if(n_pass_ == 0) flags_.fill(true);
	else flags_.fill(false);
	const int variance_half_edge = aa_noise_params_.variance_edge_size_ / 2;
	std::shared_ptr<Image> combined_image = film_image_layers_(LayerDef::Combined).image_;

	float aa_thresh_scaled = aa_noise_params_.threshold_;

	int n_resample = 0;

	if(adaptive_aa && aa_noise_params_.threshold_ > 0.f)
	{
		for(int y = 0; y < height_; ++y)
		{
			for(int x = 0; x < width_; ++x)
			{
				const float weight = weights_(x, y).getFloat();
				if(weight > 0.f) flags_.set(x, y, false);
				else flags_.set(x, y, true); //If after reloading ImageFiles there are pixels that were not yet rendered at all, make sure they are marked to be rendered in the next AA pass
			}
		}

		for(int y = 0; y < height_ - 1; ++y)
		{
			for(int x = 0; x < width_ - 1; ++x)
			{
				//We will only consider the Combined Pass (pass 0) for the AA additional sampling calculations.
				const float weight = weights_(x, y).getFloat();
				float mat_sample_factor = 1.f;
				if(sampling_factor_image_pass)
				{
					mat_sample_factor = weight > 0.f ? sampling_factor_image_pass->getFloat(x, y) / weight : 1.f;
					if(!background_resampling_ && mat_sample_factor == 0.f) continue;
				}

				const Rgba pix_col = combined_image->getColor(x, y).normalized(weight);
				const float pix_col_bri = pix_col.abscol2Bri();

				if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Linear && aa_noise_params_.dark_threshold_factor_ > 0.f)
				{
					if(aa_noise_params_.dark_threshold_factor_ > 0.f) aa_thresh_scaled = aa_noise_params_.threshold_ * ((1.f - aa_noise_params_.dark_threshold_factor_) + (pix_col_bri * aa_noise_params_.dark_threshold_factor_));
				}
				else if(aa_noise_params_.dark_detection_type_ == AaNoiseParams::DarkDetectionType::Curve)
				{
					aa_thresh_scaled = darkThresholdCurveInterpolate(pix_col_bri);
				}

				if(pix_col.colorDifference(combined_image->getColor(x + 1, y).normalized(weights_(x + 1, y).getFloat()), aa_noise_params_.detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_.set(x, y, true); flags_.set(x + 1, y, true);
				}
				if(pix_col.colorDifference(combined_image->getColor(x, y + 1).normalized(weights_(x, y + 1).getFloat()), aa_noise_params_.detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_.set(x, y, true); flags_.set(x, y + 1, true);
				}
				if(pix_col.colorDifference(combined_image->getColor(x + 1, y + 1).normalized(weights_(x + 1, y + 1).getFloat()), aa_noise_params_.detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_.set(x, y, true); flags_.set(x + 1, y + 1, true);
				}
				if(x > 0 && pix_col.colorDifference(combined_image->getColor(x - 1, y + 1).normalized(weights_(x - 1, y + 1).getFloat()), aa_noise_params_.detect_color_noise_) >= aa_thresh_scaled)
				{
					flags_.set(x, y, true); flags_.set(x - 1, y + 1, true);
				}

				if(aa_noise_params_.variance_pixels_ > 0)
				{
					int variance_x = 0, variance_y = 0;//, pixelcount = 0;

					//float window_accum = 0.f, window_avg = 0.f;

					for(int xd = -variance_half_edge; xd < variance_half_edge - 1 ; ++xd)
					{
						int xi = x + xd;
						if(xi < 0) xi = 0;
						else if(xi >= width_ - 1) xi = width_ - 2;

						const Rgba cx_0 = combined_image->getColor(xi, y).normalized(weights_(xi, y).getFloat());
						const Rgba cx_1 = combined_image->getColor(xi + 1, y).normalized(weights_(xi + 1, y).getFloat());

						if(cx_0.colorDifference(cx_1, aa_noise_params_.detect_color_noise_) >= aa_thresh_scaled) ++variance_x;
					}

					for(int yd = -variance_half_edge; yd < variance_half_edge - 1 ; ++yd)
					{
						int yi = y + yd;
						if(yi < 0) yi = 0;
						else if(yi >= height_ - 1) yi = height_ - 2;

						const Rgba cy_0 = combined_image->getColor(x, yi).normalized(weights_(x, yi).getFloat());
						const Rgba cy_1 = combined_image->getColor(x, yi + 1).normalized(weights_(x, yi + 1).getFloat());

						if(cy_0.colorDifference(cy_1, aa_noise_params_.detect_color_noise_) >= aa_thresh_scaled) ++variance_y;
					}

					if(variance_x + variance_y >= aa_noise_params_.variance_pixels_)
					{
						for(int xd = -variance_half_edge; xd < variance_half_edge; ++xd)
						{
							for(int yd = -variance_half_edge; yd < variance_half_edge; ++yd)
							{
								int xi = x + xd;
								if(xi < 0) xi = 0;
								else if(xi >= width_) xi = width_ - 1;

								int yi = y + yd;
								if(yi < 0) yi = 0;
								else if(yi >= height_) yi = height_ - 1;

								flags_.set(xi, yi, true);
							}
						}
					}
				}
			}
		}

		for(int y = 0; y < height_; ++y)
		{
			for(int x = 0; x < width_; ++x)
			{
				if(flags_.get(x, y))
				{
					++n_resample;
					if(render_callbacks_ && render_callbacks_->highlight_pixel_)
					{
						const float weight = weights_(x, y).getFloat();
						const Rgba col = combined_image->getColor(x, y).normalized(weight);
						render_callbacks_->highlight_pixel_(render_view->getName().c_str(), x, y, col.r_, col.g_, col.b_, col.a_, render_callbacks_->highlight_pixel_data_);
					}
				}
			}
		}
	}
	else
	{
		n_resample = height_ * width_;
	}

	if(render_callbacks_ && render_callbacks_->flush_) render_callbacks_->flush_(render_view->getName().c_str(), render_callbacks_->flush_data_);

	if(render_control.resumed()) pass_string << "Film loaded + ";

	pass_string << "Rendering pass " << n_pass_ << " of " << n_passes_ << ", resampling " << n_resample << " pixels.";

	logger_.logInfo(integrator_name, ": ", pass_string.str());

	if(progress_bar_)
	{
		progress_bar_->init(width_ * height_, logger_.getConsoleLogColorsEnabled());
		render_control.setCurrentPassPercent(progress_bar_->getPercent());
		progress_bar_->setTag(pass_string.str().c_str());
	}
	completed_cnt_ = 0;

	return n_resample;
}

bool ImageFilm::nextArea(const RenderView *render_view, const RenderControl &render_control, RenderArea &a)
{
	if(cancel_) return false;

	const int ifilterw = static_cast<int>(std::ceil(filterw_));

	if(split_)
	{
		const int n = next_area_++;
		if(splitter_->getArea(n, a))
		{
			a.sx_0_ = a.x_ + ifilterw;
			a.sx_1_ = a.x_ + a.w_ - ifilterw;
			a.sy_0_ = a.y_ + ifilterw;
			a.sy_1_ = a.y_ + a.h_ - ifilterw;

			if(render_callbacks_ && render_callbacks_->highlight_area_)
			{
				const int end_x = a.x_ + a.w_;
				const int end_y = a.y_ + a.h_;
				render_callbacks_->highlight_area_(render_view->getName().c_str(), a.id_, a.x_, a.y_, end_x, end_y, render_callbacks_->highlight_area_data_);
			}
			return true;
		}
	}
	else
	{
		if(area_cnt_) return false;
		a.x_ = cx_0_;
		a.y_ = cy_0_;
		a.w_ = width_;
		a.h_ = height_;
		a.sx_0_ = a.x_ + ifilterw;
		a.sx_1_ = a.x_ + a.w_ - ifilterw;
		a.sy_0_ = a.y_ + ifilterw;
		a.sy_1_ = a.y_ + a.h_ - ifilterw;
		++area_cnt_;
		return true;
	}
	return false;
}

void ImageFilm::finishArea(const RenderView *render_view, RenderControl &render_control, const RenderArea &a, const EdgeToonParams &edge_params)
{
	std::lock_guard<std::mutex> lock_guard(out_mutex_);
	const int end_x = a.x_ + a.w_ - cx_0_;
	const int end_y = a.y_ + a.h_ - cy_0_;

	if(layers_.isDefined(LayerDef::DebugFacesEdges))
	{
		image_manipulation::generateDebugFacesEdges(film_image_layers_, a.x_ - cx_0_, end_x, a.y_ - cy_0_, end_y, true, edge_params, weights_);
	}

	if(layers_.isDefinedAny({LayerDef::DebugObjectsEdges, LayerDef::Toon}))
	{
		image_manipulation::generateToonAndDebugObjectEdges(film_image_layers_, a.x_ - cx_0_, end_x, a.y_ - cy_0_, end_y, true, edge_params, weights_);
	}

	for(const auto &[layer_def, image_layer] : film_image_layers_)
	{
		if(!image_layer.layer_.isExported()) continue;
		const std::shared_ptr<Image> &image = image_layer.image_;
		for(int j = a.y_ - cy_0_; j < end_y; ++j)
		{
			for(int i = a.x_ - cx_0_; i < end_x; ++i)
			{
				const float weight = weights_(i, j).getFloat();
				Rgba color = (layer_def == LayerDef::AaSamples ? Rgba{weight} : image->getColor(i, j).normalized(weight));
				switch(layer_def)
				{
					//To correct the antialiasing and ceil the "mixed" values to the upper integer in the Object/Material Index layers
					case LayerDef::ObjIndexAbs:
					case LayerDef::ObjIndexAutoAbs:
					case LayerDef::MatIndexAbs:
					case LayerDef::MatIndexAutoAbs: color.ceil(); break;
					default: break;
				}
				exported_image_layers_.setColor(i, j, color, layer_def);
				if(render_callbacks_ && render_callbacks_->put_pixel_)
				{
					render_callbacks_->put_pixel_(render_view->getName().c_str(), LayerDef::getName(layer_def).c_str(), i, j, color.r_, color.g_, color.b_, color.a_, render_callbacks_->put_pixel_data_);
				}
			}
		}
	}

	if(render_callbacks_ && render_callbacks_->flush_area_) render_callbacks_->flush_area_(render_view->getName().c_str(), a.id_, a.x_, a.y_, end_x + cx_0_, end_y + cy_0_, render_callbacks_->flush_area_data_);

	if(render_control.inProgress())
	{
		timer_.stop("imagesAutoSaveTimer");
		images_auto_save_params_.timer_ += timer_.getTime("imagesAutoSaveTimer");
		if(images_auto_save_params_.timer_ < 0.f) resetImagesAutoSaveTimer(); //to solve some strange very negative value when using yafaray-xml, race condition somewhere?
		timer_.start("imagesAutoSaveTimer");

		timer_.stop("filmAutoSaveTimer");
		film_load_save_.auto_save_.timer_ += timer_.getTime("filmAutoSaveTimer");
		if(film_load_save_.auto_save_.timer_ < 0.f) resetFilmAutoSaveTimer(); //to solve some strange very negative value when using yafaray-xml, race condition somewhere?
		timer_.start("filmAutoSaveTimer");

		if((images_auto_save_params_.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Time) && (images_auto_save_params_.timer_ > images_auto_save_params_.interval_seconds_))
		{
			if(logger_.isDebug())logger_.logDebug("imagesAutoSaveTimer=", images_auto_save_params_.timer_);
			flush(render_view, render_control, edge_params, All);
			resetImagesAutoSaveTimer();
		}

		if((film_load_save_.mode_ == FilmLoadSave::LoadAndSave || film_load_save_.mode_ == FilmLoadSave::Save) && (film_load_save_.auto_save_.interval_type_ == ImageFilm::AutoSaveParams::IntervalType::Time) && (film_load_save_.auto_save_.timer_ > film_load_save_.auto_save_.interval_seconds_))
		{
			if(logger_.isDebug())logger_.logDebug("filmAutoSaveTimer=", film_load_save_.auto_save_.timer_);
			imageFilmSave();
			resetFilmAutoSaveTimer();
		}
	}

	if(progress_bar_)
	{
		if(++completed_cnt_ == area_cnt_) progress_bar_->done();
		else progress_bar_->update(a.w_ * a.h_);
		render_control.setCurrentPassPercent(progress_bar_->getPercent());
	}
}

void ImageFilm::flush(const RenderView *render_view, const RenderControl &render_control, const EdgeToonParams &edge_params, int flags)
{
	if(render_control.finished())
	{
		logger_.logInfo("imageFilm: Flushing buffer (View '", render_view->getName(), "')...");
	}

	float density_factor = 0.f;

	if(estimate_density_ && num_density_samples_ > 0) density_factor = (float) (width_ * height_) / (float) num_density_samples_;

	const Layers layers = layers_.getLayersWithImages();
	if(layers.isDefined(LayerDef::DebugFacesEdges))
	{
		image_manipulation::generateDebugFacesEdges(film_image_layers_, 0, width_, 0, height_, false, edge_params, weights_);
	}
	if(layers.isDefinedAny({LayerDef::DebugObjectsEdges, LayerDef::Toon}))
	{
		image_manipulation::generateToonAndDebugObjectEdges(film_image_layers_, 0, width_, 0, height_, false, edge_params, weights_);
	}
	for(const auto &[layer_def, image_layer] : film_image_layers_)
	{
		if(!image_layer.layer_.isExported()) continue;
		const std::shared_ptr<Image> &image = image_layer.image_;
		for(int j = 0; j < height_; j++)
		{
			for(int i = 0; i < width_; i++)
			{
				const float weight = weights_(i, j).getFloat();
				Rgba color = (layer_def == LayerDef::AaSamples ? Rgba{weight} : image->getColor(i, j).normalized(weight));
				switch(layer_def)
				{
					//To correct the antialiasing and ceil the "mixed" values to the upper integer in the Object/Material Index layers
					case LayerDef::ObjIndexAbs:
					case LayerDef::ObjIndexAutoAbs:
					case LayerDef::MatIndexAbs:
					case LayerDef::MatIndexAutoAbs: color.ceil(); break;
					default: break;
				}
				if(estimate_density_ && (flags & Densityimage) && layer_def == LayerDef::Combined && density_factor > 0.f) color += Rgba((*density_image_)(i, j) * density_factor, 0.f);
				exported_image_layers_.setColor(i, j, color, layer_def);
				if(render_callbacks_ && render_callbacks_->put_pixel_)
				{
					render_callbacks_->put_pixel_(render_view->getName().c_str(), LayerDef::getName(layer_def).c_str(), i, j, color.r_, color.g_, color.b_, color.a_, render_callbacks_->put_pixel_data_);
				}
			}
		}
	}

	if(render_callbacks_ && render_callbacks_->flush_) render_callbacks_->flush_(render_view->getName().c_str(), render_callbacks_->flush_data_);

	if(render_control.finished())
	{
		std::stringstream ss;
		ss << printRenderStats(render_control, timer_, width_, height_);
		ss << render_control.getAaNoiseInfo();
		logger_.logParams("--------------------------------------------------------------------------------");
		for(std::string line; std::getline(ss, line, '\n');) if(line != "" && line != "\n") logger_.logParams(line);
		logger_.logParams("--------------------------------------------------------------------------------");
	}

	for(auto &[output_name, output] : outputs_)
	{
		if(output)
		{
			std::stringstream pass_string;
			pass_string << "Flushing output '" << output_name << "' and saving image files.";
			logger_.logInfo(pass_string.str());
			if(render_control.finished())
			{
				std::stringstream ss;
				ss << output->printBadge(render_control, timer_);
				//logger_.logParams("--------------------------------------------------------------------------------");
				for(std::string line; std::getline(ss, line, '\n');) if(line != "" && line != "\n") logger_.logParams(line);
				//logger_.logParams("--------------------------------------------------------------------------------");
			}
			std::string old_tag;
			if(progress_bar_)
			{
				old_tag = progress_bar_->getTag();
				progress_bar_->setTag(pass_string.str().c_str());
			}
			output->flush(render_control, timer_);
			if(progress_bar_) progress_bar_->setTag(old_tag);
		}
	}

	if(render_control.finished())
	{
		if(film_load_save_.mode_ == FilmLoadSave::LoadAndSave || film_load_save_.mode_ == FilmLoadSave::Save)
		{
			imageFilmSave();
		}

		timer_.stop("imagesAutoSaveTimer");
		timer_.stop("filmAutoSaveTimer");

		logger_.clearMemoryLog();
		if(logger_.isVerbose()) logger_.logVerbose("imageFilm: Done.");
	}
}

bool ImageFilm::doMoreSamples(int x, int y) const
{
	return aa_noise_params_.threshold_ <= 0.f || flags_.get(x - cx_0_, y - cy_0_);
}

/* CAUTION! Implemantation of this function needs to be thread safe for samples that
	contribute to pixels outside the area a AND pixels that might get
	contributions from outside area a! (yes, really!) */
void ImageFilm::addSample(int x, int y, float dx, float dy, const RenderArea *a, int num_sample, int aa_pass_number, float inv_aa_max_possible_samples, const ColorLayers *color_layers)
{
	// get filter extent and make sure we don't leave image area:

	const int dx_0 = std::max(cx_0_ - x, math::roundToInt(static_cast<double>(dx) - filterw_));
	const int dx_1 = std::min(cx_1_ - x - 1, math::roundToInt(static_cast<double>(dx) + filterw_ - 1.0));
	const int dy_0 = std::max(cy_0_ - y, math::roundToInt(static_cast<double>(dy) - filterw_));
	const int dy_1 = std::min(cy_1_ - y - 1, math::roundToInt(static_cast<double>(dy) + filterw_ - 1.0));

	// get indizes in filter table
	const double x_offs = dx - 0.5;

	int x_index[max_filter_size_ + 1];
	int y_index[max_filter_size_ + 1];

	for(int i = dx_0, n = 0; i <= dx_1; ++i, ++n)
	{
		const double d = std::abs((static_cast<double>(i) - x_offs) * table_scale_);
		x_index[n] = math::floorToInt(d);
	}

	const double y_offs = dy - 0.5;

	for(int i = dy_0, n = 0; i <= dy_1; ++i, ++n)
	{
		const double d = std::abs((static_cast<double>(i) - y_offs) * table_scale_);
		y_index[n] = math::floorToInt(d);
	}

	const int x_0 = x + dx_0;
	const int x_1 = x + dx_1;
	const int y_0 = y + dy_0;
	const int y_1 = y + dy_1;

	std::lock_guard<std::mutex> lock_guard(image_mutex_);
	for(int j = y_0; j <= y_1; ++j)
	{
		for(int i = x_0; i <= x_1; ++i)
		{
			// get filter value at pixel (x,y)
			const int offset = y_index[j - y_0] * filter_table_size_ + x_index[i - x_0];
			const float filter_wt = filter_table_[offset];
			weights_(i - cx_0_, j - cy_0_).setFloat(weights_(i - cx_0_, j - cy_0_).getFloat() + filter_wt);

			// update pixel values with filtered sample contribution
			for(auto &[layer_def, image_layer] : film_image_layers_)
			{
				Rgba col = color_layers ? (*color_layers)(layer_def) : Rgba{0.f};
				col.clampProportionalRgb(aa_noise_params_.clamp_samples_);
				image_layer.image_->setColor(i - cx_0_, j - cy_0_, image_layer.image_->getColor(i - cx_0_, j - cy_0_) + (col * filter_wt));
			}
		}
	}
}

void ImageFilm::addDensitySample(const Rgb &c, int x, int y, float dx, float dy, const RenderArea *a)
{
	if(!estimate_density_) return;

	// get filter extent and make sure we don't leave image area:
	const int dx_0 = std::max(cx_0_ - x, math::roundToInt(static_cast<double>(dx) - filterw_));
	const int dx_1 = std::min(cx_1_ - x - 1, math::roundToInt(static_cast<double>(dx) + filterw_ - 1.0));
	const int dy_0 = std::max(cy_0_ - y, math::roundToInt(static_cast<double>(dy) - filterw_));
	const int dy_1 = std::min(cy_1_ - y - 1, math::roundToInt(static_cast<double>(dy) + filterw_ - 1.0));

	int x_index[max_filter_size_ + 1];
	int y_index[max_filter_size_ + 1];

	const double x_offs = dx - 0.5;
	for(int i = dx_0, n = 0; i <= dx_1; ++i, ++n)
	{
		const double d = std::abs((static_cast<double>(i) - x_offs) * table_scale_);
		x_index[n] = math::floorToInt(d);
	}

	const double y_offs = dy - 0.5;
	for(int i = dy_0, n = 0; i <= dy_1; ++i, ++n)
	{
		const float d = std::abs(static_cast<float>((static_cast<double>(i) - y_offs) * table_scale_));
		y_index[n] = math::floorToInt(d);
	}

	const int x_0 = x + dx_0;
	const int x_1 = x + dx_1;
	const int y_0 = y + dy_0;
	const int y_1 = y + dy_1;

	std::lock_guard<std::mutex> lock_guard(density_image_mutex_);
	for(int j = y_0; j <= y_1; ++j)
	{
		for(int i = x_0; i <= x_1; ++i)
		{
			const int offset = y_index[j - y_0] * filter_table_size_ + x_index[i - x_0];
			Rgb &pixel = (*density_image_)(i - cx_0_, j - cy_0_);
			pixel += c * filter_table_[offset];
		}
	}
	++num_density_samples_;
}

void ImageFilm::setDensityEstimation(bool enable)
{
	if(enable)
	{
		if(!density_image_) density_image_ = std::unique_ptr<ImageBuffer2D<Rgb>>(new ImageBuffer2D<Rgb>(width_, height_));
		else density_image_->clear();
	}
	else
	{
		density_image_ = nullptr;
	}
	estimate_density_ = enable;
}

void ImageFilm::setProgressBar(std::shared_ptr<ProgressBar> pb)
{
	progress_bar_ = std::move(pb);
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
	node << std::setfill('0') << std::setw(4) << computer_node_;
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
	file.read<unsigned int>(computer_node_);
	file.read<unsigned int>(base_sampling_offset_);
	file.read<unsigned int>(sampling_offset_);

	int filmload_check_w;
	file.read<int>(filmload_check_w);
	if(filmload_check_w != width_)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Image width, expected=", width_, ", in reused/loaded film=", filmload_check_w);
		return false;
	}

	int filmload_check_h;
	file.read<int>(filmload_check_h);
	if(filmload_check_h != height_)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Image height, expected=", height_, ", in reused/loaded film=", filmload_check_h);
		return false;
	}

	int filmload_check_cx_0;
	file.read<int>(filmload_check_cx_0);
	if(filmload_check_cx_0 != cx_0_)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Border cx0, expected=", cx_0_, ", in reused/loaded film=", filmload_check_cx_0);
		return false;
	}

	int filmload_check_cx_1;
	file.read<int>(filmload_check_cx_1);
	if(filmload_check_cx_1 != cx_1_)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Border cx1, expected=", cx_1_, ", in reused/loaded film=", filmload_check_cx_1);
		return false;
	}

	int filmload_check_cy_0;
	file.read<int>(filmload_check_cy_0);
	if(filmload_check_cy_0 != cy_0_)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Border cy0, expected=", cy_0_, ", in reused/loaded film=", filmload_check_cy_0);
		return false;
	}

	int filmload_check_cy_1;
	file.read<int>(filmload_check_cy_1);
	if(filmload_check_cy_1 != cy_1_)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Border cy1, expected=", cy_1_, ", in reused/loaded film=", filmload_check_cy_1);
		return false;
	}

	int loaded_image_num_layers;
	file.read<int>(loaded_image_num_layers);
	initLayersImages();
	//If there are any ImageOutputs, creation of the image buffers for the image outputs exported images
	if(!outputs_.empty()) initLayersExportedImages();
	const int num_layers = film_image_layers_.size();
	if(loaded_image_num_layers != num_layers)
	{
		logger_.logWarning("imageFilm: loading/reusing film check failed. Number of image layers, expected=", num_layers, ", in reused/loaded film=", loaded_image_num_layers);
		return false;
	}

	for(int y = 0; y < height_; ++y)
	{
		for(int x = 0; x < width_; ++x)
		{
			float weight;
			file.read<float>(weight);
			weights_(x, y).setFloat(weight);
		}
	}

	for(auto &[layer_def, image_layer] : film_image_layers_)
	{
		for(int y = 0; y < height_; ++y)
		{
			for(int x = 0; x < width_; ++x)
			{
				Rgba col;
				file.read<float>(col.r_);
				file.read<float>(col.g_);
				file.read<float>(col.b_);
				file.read<float>(col.a_);
				image_layer.image_->setColor(x, y, col);
			}
		}
	}

	file.close();
	return true;
}

void ImageFilm::imageFilmLoadAllInFolder(RenderControl &render_control)
{
	std::stringstream pass_string;
	pass_string << "Loading ImageFilm files";

	logger_.logInfo(pass_string.str());

	std::string old_tag;

	if(progress_bar_)
	{
		old_tag = progress_bar_->getTag();
		progress_bar_->setTag(pass_string.str().c_str());
	}

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
				film_file_paths_list.push_back(dir + "//" + file_name);
				//if(logger_.isDebug())logger_.logDebug("Added: " << dir + "//" + fileName);
			}
		}
	}

	std::sort(film_file_paths_list.begin(), film_file_paths_list.end());
	bool any_film_loaded = false;
	for(const auto &film_file : film_file_paths_list)
	{
		auto loaded_film = std::unique_ptr<ImageFilm>(new ImageFilm(logger_, width_, height_, cx_0_, cy_0_, num_threads_, render_control, layers_, outputs_, 1.0, FilterType::Box));
		if(!loaded_film->imageFilmLoad(film_file))
		{
			logger_.logWarning("ImageFilm: Could not load film file '", film_file, "'");
			continue;
		}
		else any_film_loaded = true;

		for(int i = 0; i < width_; ++i)
		{
			for(int j = 0; j < height_; ++j)
			{
				weights_(i, j).setFloat(weights_(i, j).getFloat() + loaded_film->weights_(i, j).getFloat());
			}
		}

		for(auto &[layer_def, image_layer] : film_image_layers_)
		{
			const ImageLayers &loaded_image_layers = loaded_film->film_image_layers_;
			for(int i = 0; i < width_; ++i)
			{
				for(int j = 0; j < height_; ++j)
				{
					image_layer.image_->setColor(i, j, image_layer.image_->getColor(i, j) + loaded_image_layers(layer_def).image_->getColor(i, j));
				}
			}
		}
		if(sampling_offset_ < loaded_film->sampling_offset_) sampling_offset_ = loaded_film->sampling_offset_;
		if(base_sampling_offset_ < loaded_film->base_sampling_offset_) base_sampling_offset_ = loaded_film->base_sampling_offset_;
		if(logger_.isVerbose()) logger_.logVerbose("ImageFilm: loaded film '", film_file, "'");
	}
	if(any_film_loaded) render_control.setResumed();
	if(progress_bar_) progress_bar_->setTag(old_tag);
}

bool ImageFilm::imageFilmSave()
{
	bool result_ok = true;
	std::stringstream pass_string;
	pass_string << "Saving internal ImageFilm file";

	logger_.logInfo(pass_string.str());

	std::string old_tag;

	if(progress_bar_)
	{
		old_tag = progress_bar_->getTag();
		progress_bar_->setTag(pass_string.str().c_str());
	}

	const std::string film_path = getFilmPath();
	File file(film_path);
	file.open("wb");
	file.append(std::string("YAF_FILMv4_0_0"));
	file.append<unsigned int>(computer_node_);
	file.append<unsigned int>(base_sampling_offset_);
	file.append<unsigned int>(sampling_offset_);
	file.append<int>(width_);
	file.append<int>(height_);
	file.append<int>(cx_0_);
	file.append<int>(cx_1_);
	file.append<int>(cy_0_);
	file.append<int>(cy_1_);
	file.append<int>((int) film_image_layers_.size());

	const int weights_w = weights_.getWidth();
	if(weights_w != width_)
	{
		logger_.logWarning("ImageFilm saving problems, film weights width ", width_, " different from internal 2D image width ", weights_w);
		result_ok = false;
	}
	const int weights_h = weights_.getHeight();
	if(weights_h != height_)
	{
		logger_.logWarning("ImageFilm saving problems, film weights height ", height_, " different from internal 2D image height ", weights_h);
		result_ok = false;
	}
	for(int y = 0; y < height_; ++y)
	{
		for(int x = 0; x < width_; ++x)
		{
			file.append<float>(weights_(x, y).getFloat());
		}
	}

	for(const auto &[layer_def, image_layer] : film_image_layers_)
	{
		const int img_w = image_layer.image_->getWidth();
		if(img_w != width_)
		{
			logger_.logWarning("ImageFilm saving problems, film width ", width_, " different from internal 2D image width ", img_w);
			result_ok = false;
			break;
		}
		const int img_h = image_layer.image_->getHeight();
		if(img_h != height_)
		{
			logger_.logWarning("ImageFilm saving problems, film height ", height_, " different from internal 2D image height ", img_h);
			result_ok = false;
			break;
		}
		for(int y = 0; y < height_; ++y)
		{
			for(int x = 0; x < width_; ++x)
			{
				const Rgba &col = image_layer.image_->getColor(x, y);
				file.append<float>(col.r_);
				file.append<float>(col.g_);
				file.append<float>(col.b_);
				file.append<float>(col.a_);
			}
		}
	}
	file.close();
	if(progress_bar_) progress_bar_->setTag(old_tag);
	return result_ok;
}

void ImageFilm::imageFilmFileBackup() const
{
	std::stringstream pass_string;
	pass_string << "Creating backup of the previous ImageFilm file...";

	logger_.logInfo(pass_string.str());

	std::string old_tag;

	if(progress_bar_)
	{
		old_tag = progress_bar_->getTag();
		progress_bar_->setTag(pass_string.str().c_str());
	}

	const std::string film_path = getFilmPath();
	const std::string film_path_backup = film_path + "-previous.bak";

	if(File::exists(film_path, true))
	{
		if(logger_.isVerbose()) logger_.logVerbose("imageFilm: Creating backup of previously saved film to: \"", film_path_backup, "\"");
		const bool result_ok = File::rename(film_path, film_path_backup, true, true);
		if(!result_ok) logger_.logWarning("imageFilm: error during imageFilm file backup");
	}

	if(progress_bar_) progress_bar_->setTag(old_tag);
}

std::string ImageFilm::printRenderStats(const RenderControl &render_control, const Timer &timer, int width, int height)
{
	std::stringstream ss;
	ss << "\nYafaRay (" << buildinfo::getVersionString() << buildinfo::getBuildTypeSuffix() << ")" << " " << buildinfo::getBuildOs() << " " << buildinfo::getBuildArchitectureBits() << "bit (" << buildinfo::getBuildCompiler() << ")";
	ss << std::setprecision(2);

	double times = timer.getTimeNotStopping("rendert");
	if(render_control.finished()) times = timer.getTime("rendert");
	int timem, timeh;
	timer.splitTime(times, &times, &timem, &timeh);
	ss << " | " << width << "x" << height;
	if(render_control.inProgress()) ss << " | " << (render_control.resumed() ? "film loaded + " : "") << "in progress " << std::fixed << std::setprecision(1) << render_control.currentPassPercent() << "% of pass: " << render_control.currentPass() << " / " << render_control.totalPasses();
	else if(render_control.canceled()) ss << " | " << (render_control.resumed() ? "film loaded + " : "") << "stopped at " << std::fixed << std::setprecision(1) << render_control.currentPassPercent() << "% of pass: " << render_control.currentPass() << " / " << render_control.totalPasses();
	else
	{
		if(render_control.resumed()) ss << " | film loaded + " << render_control.totalPasses() - 1 << " passes";
		else ss << " | " << render_control.totalPasses() << " passes";
	}
	//if(cx0 != 0) ssBadge << ", xstart=" << cx0;
	//if(cy0 != 0) ssBadge << ", ystart=" << cy0;
	ss << " | Render time:";
	if(timeh > 0) ss << " " << timeh << "h";
	if(timem > 0) ss << " " << timem << "m";
	ss << " " << times << "s";

	times = timer.getTimeNotStopping("rendert") + timer.getTime("prepass");
	if(render_control.finished()) times = timer.getTime("rendert") + timer.getTime("prepass");
	timer.splitTime(times, &times, &timem, &timeh);
	ss << " | Total time:";
	if(timeh > 0) ss << " " << timeh << "h";
	if(timem > 0) ss << " " << timem << "m";
	ss << " " << times << "s";
	return ss.str();
}

END_YAFARAY
