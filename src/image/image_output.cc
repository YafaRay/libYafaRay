/****************************************************************************
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#include "image/image_output.h"

#include "color/color_layers.h"
#include "scene/scene.h"
#include "common/logger.h"
#include "param/param.h"
#include "common/layers.h"
#include "common/file.h"
#include "format/format.h"
#include "render/render_view.h"
#include "render/imagefilm.h"
#include "image/image_manipulation.h"

namespace yafaray {

ImageOutput::Params::Params(ParamResult &param_result, const ParamMap &param_map)
{
	PARAM_LOAD(image_path_);
	PARAM_ENUM_LOAD(color_space_);
	PARAM_LOAD(gamma_);
	PARAM_LOAD(alpha_channel_);
	PARAM_LOAD(alpha_premultiply_);
	PARAM_LOAD(multi_layer_);
	PARAM_LOAD(denoise_enabled_);
	PARAM_LOAD(denoise_h_lum_);
	PARAM_LOAD(denoise_h_col_);
	PARAM_LOAD(denoise_mix_);
	PARAM_LOAD(logging_save_txt_);
	PARAM_LOAD(logging_save_html_);
	PARAM_ENUM_LOAD(badge_position_);
	PARAM_LOAD(badge_draw_render_settings_);
	PARAM_LOAD(badge_draw_aa_noise_settings_);
	PARAM_LOAD(badge_author_);
	PARAM_LOAD(badge_title_);
	PARAM_LOAD(badge_contact_);
	PARAM_LOAD(badge_comment_);
	PARAM_LOAD(badge_icon_path_);
	PARAM_LOAD(badge_font_path_);
	PARAM_LOAD(badge_font_size_factor_);
}

ParamMap ImageOutput::Params::getAsParamMap(bool only_non_default) const
{
	PARAM_SAVE_START;
	PARAM_SAVE(image_path_);
	PARAM_ENUM_SAVE(color_space_);
	PARAM_SAVE(gamma_);
	PARAM_SAVE(alpha_channel_);
	PARAM_SAVE(alpha_premultiply_);
	PARAM_SAVE(multi_layer_);
	PARAM_SAVE(denoise_enabled_);
	PARAM_SAVE(denoise_h_lum_);
	PARAM_SAVE(denoise_h_col_);
	PARAM_SAVE(denoise_mix_);
	PARAM_SAVE(logging_save_txt_);
	PARAM_SAVE(logging_save_html_);
	PARAM_ENUM_SAVE(badge_position_);
	PARAM_SAVE(badge_draw_render_settings_);
	PARAM_SAVE(badge_draw_aa_noise_settings_);
	PARAM_SAVE(badge_author_);
	PARAM_SAVE(badge_title_);
	PARAM_SAVE(badge_contact_);
	PARAM_SAVE(badge_comment_);
	PARAM_SAVE(badge_icon_path_);
	PARAM_SAVE(badge_font_path_);
	PARAM_SAVE(badge_font_size_factor_);
	PARAM_SAVE_END;
}

ParamMap ImageOutput::getAsParamMap(bool only_non_default) const
{
	return params_.getAsParamMap(only_non_default);
}

std::pair<std::unique_ptr<ImageOutput>, ParamResult> ImageOutput::factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &param_map)
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + "::factory 'raw' ParamMap\n" + param_map.logContents());
	auto param_result{Params::meta_.check(param_map, {}, {})};
	auto output {std::make_unique<ImageOutput>(logger, param_result, param_map)};
	if(param_result.notOk()) logger.logWarning(param_result.print<ImageOutput>(name, {}));
	return {std::move(output), param_result};
}

ImageOutput::ImageOutput(Logger &logger, ParamResult &param_result, const ParamMap &param_map) : params_{param_result, param_map}, logger_{logger}
{
	if(logger.isDebug()) logger.logDebug("**" + getClassName() + " params_:\n" + params_.getAsParamMap(true).print());
	if(params_.color_space_ == ColorSpace::RawManualGamma)
	{
		//If the gamma is too close to 1.f, or negative, ignore gamma and do a pure linear RGB processing without gamma.
		if(params_.gamma_ <= 0.f || std::abs(1.f - params_.gamma_) <= 0.001f)
		{
			color_space_ = ColorSpace::LinearRgb;
			gamma_ = 1.f;
		}
	}
}

std::string ImageOutput::printBadge(const RenderControl &render_control, const Timer &timer) const
{
	return badge_.print(image_manipulation::printDenoiseParams(denoise_params_), render_control, timer);
}

std::unique_ptr<Image> ImageOutput::generateBadgeImage(const RenderControl &render_control, const Timer &timer) const
{
	return badge_.generateImage(image_manipulation::printDenoiseParams(denoise_params_), render_control, timer);
}

void ImageOutput::init(const Size2i &size, const ImageLayers *exported_image_layers, const std::map<std::string, std::unique_ptr<RenderView>> *render_views)
{
	image_layers_ = exported_image_layers;
	render_views_ = render_views;
	badge_.setImageSize(size);
}

void ImageOutput::flush(const RenderControl &render_control, const Timer &timer)
{
	Path path(params_.image_path_);
	std::string directory = path.getDirectory();
	std::string base_name = path.getBaseName();
	const std::string ext = path.getExtension();
	const std::string view_name = current_render_view_->getName();
	if(!view_name.empty()) base_name += " (view " + view_name + ")";
	ParamMap params;
	params["type"] = ext;
	auto format{Format::factory(logger_, params).first};

	if(format)
	{
		if(params_.multi_layer_ && format->supportsMultiLayer())
		{
			if(view_name == current_render_view_->getName())
			{
				saveImageFile(params_.image_path_, LayerDef::Combined, format.get(), render_control, timer); //This should not be necessary but Blender API seems to be limited and the API "load_from_file" function does not work (yet) with multilayered images, so I have to generate this extra combined pass file so it's displayed in the Blender window.
			}

			if(!directory.empty()) directory += "/";
			const std::string fname_pass = directory + base_name + " (" + "multilayer" + ")." + ext;
			saveImageFileMultiChannel(fname_pass, format.get(), render_control, timer);

			logger_.setImagePath(fname_pass); //to show the image in the HTML log output
		}
		else
		{
			if(!directory.empty()) directory += "/";
			for(const auto &[layer_def, image_layer] : *image_layers_)
			{
				const std::string exported_image_name = image_layer.layer_.getExportedImageName();
				if(layer_def == LayerDef::Combined)
				{
					saveImageFile(params_.image_path_, layer_def, format.get(), render_control, timer); //default imagehandler filename, when not using views nor passes and for reloading into Blender
					logger_.setImagePath(params_.image_path_); //to show the image in the HTML log output
				}

				if(layer_def != LayerDef::Disabled && (image_layers_->size() > 1 || render_views_->size() > 1))
				{
					const std::string layer_type_name = LayerDef::getName(layer_def);
					std::string fname_pass = directory + base_name + " [" + layer_type_name;
					if(!exported_image_name.empty()) fname_pass += " - " + exported_image_name;
					fname_pass += "]." + ext;
					saveImageFile(fname_pass, layer_def, format.get(), render_control, timer);
				}
			}
		}
	}
	if(params_.logging_save_txt_)
	{
		std::string f_log_txt_name = directory + "/" + base_name + "_log.txt";
		logger_.saveTxtLog(f_log_txt_name, badge_, render_control, timer);
	}
	if(params_.logging_save_html_)
	{
		std::string f_log_html_name = directory + "/" + base_name + "_log.html";
		logger_.saveHtmlLog(f_log_html_name, badge_, render_control, timer);
	}
	if(logger_.getSaveStats())
	{
		std::string f_stats_name = directory + "/" + base_name + "_stats.csv";
		logger_.statsSaveToFile(f_stats_name, /*sorted=*/ true);
	}
}

void ImageOutput::saveImageFile(const std::string &filename, LayerDef::Type layer_type, Format *format, const RenderControl &render_control, const Timer &timer)
{
	if(render_control.inProgress()) logger_.logInfo(name_, ": Autosaving partial render (", math::roundFloatPrecision(render_control.currentPassPercent(), 0.01), "% of pass ", render_control.currentPass(), " of ", render_control.totalPasses(), ") file as \"", filename, "\"...  ", image_manipulation::printDenoiseParams(denoise_params_));
	else logger_.logInfo(name_, ": Saving file as \"", filename, "\"...  ", image_manipulation::printDenoiseParams(denoise_params_));

	auto image{(*image_layers_)(layer_type).image_};
	if(!image)
	{
		logger_.logWarning(name_, ": Image does not exist (it is null) and could not be saved.");
		return;
	}

	if(badge_.getPosition() != Badge::Position::None)
	{
		const auto badge_image{generateBadgeImage(render_control, timer)};
		auto badge_image_position{Image::Position::Bottom};
		if(badge_.getPosition() == Badge::Position::Top) badge_image_position = Image::Position::Top;
		image = image_manipulation::getComposedImage(logger_, image.get(), badge_image.get(), badge_image_position);
		if(!image)
		{
			logger_.logWarning(name_, ": Image could not be composed with badge and could not be saved.");
			return;
		}
	}

	ImageLayer image_layer { image, Layer(layer_type) };
	if(denoiseEnabled())
	{
		auto image_denoised{image_manipulation::getDenoisedLdrImage(logger_, image.get(), denoise_params_)};
		if(image_denoised) image_layer.image_ = std::move(image_denoised);
		else if(logger_.isVerbose()) logger_.logVerbose(name_, ": Denoise was not possible, saving image without denoise postprocessing.");
	}
	format->saveToFile(filename, image_layer, color_space_, gamma_, params_.alpha_premultiply_);

	if(params_.alpha_channel_ && !format->supportsAlpha())
	{
		Path file_path(filename);
		std::string file_name_alpha = file_path.getBaseName() + "_alpha." + file_path.getExtension();
		logger_.logInfo(name_, ": Saving separate alpha channel file as \"", file_name_alpha, "\"...  ", image_manipulation::printDenoiseParams(denoise_params_));
		format->saveAlphaChannelOnlyToFile(file_name_alpha, image_layer);
	}
}

void ImageOutput::saveImageFileMultiChannel(const std::string &filename, Format *format, const RenderControl &render_control, const Timer &timer)
{
	if(badge_.getPosition() != Badge::Position::None)
	{
		auto badge_image{generateBadgeImage(render_control, timer)};
		auto badge_image_position{Image::Position::Bottom};
		if(badge_.getPosition() == Badge::Position::Top) badge_image_position = Image::Position::Top;
		ImageLayers image_layers_badge;
		for(const auto &[layer_def, image_layer] : *image_layers_)
		{
			auto image_layer_badge{image_manipulation::getComposedImage(logger_, image_layer.image_.get(), badge_image.get(), badge_image_position)};
			image_layers_badge.set(layer_def, {std::move(image_layer_badge), image_layer.layer_});
		}
		format->saveToFileMultiChannel(filename, image_layers_badge, color_space_, gamma_, params_.alpha_premultiply_);
	}
	else format->saveToFileMultiChannel(filename, *image_layers_, color_space_, gamma_, params_.alpha_premultiply_);
}

} //namespace yafaray