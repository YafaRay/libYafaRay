/****************************************************************************
 *
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
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

#include "output/output_image.h"
#include "color/color_layers.h"
#include "common/logger.h"
#include "common/file.h"
#include "common/param.h"
#include "format/format.h"
#include "image/image_layers.h"

BEGIN_YAFARAY

ColorOutput *ImageOutput::factory(const ParamMap &params, const Scene &scene)
{
	std::string name;
	std::string image_path;
	int border_x = 0;
	int border_y = 0;
	std::string color_space_str = "Raw_Manual_Gamma";
	float gamma = 1.f;
	bool with_alpha = false;
	bool alpha_premultiply = false;
	bool multi_layer = true;
	DenoiseParams denoise_params;

	params.getParam("name", name);
	params.getParam("image_path", image_path);
	params.getParam("border_x", border_x);
	params.getParam("border_y", border_y);
	params.getParam("color_space", color_space_str);
	params.getParam("gamma", gamma);
	params.getParam("alpha_channel", with_alpha);
	params.getParam("alpha_premultiply", alpha_premultiply);
	params.getParam("multi_layer", multi_layer);

	params.getParam("denoiseEnabled", denoise_params.enabled_);
	params.getParam("denoiseHLum", denoise_params.hlum_);
	params.getParam("denoiseHCol", denoise_params.hcol_);
	params.getParam("denoiseMix", denoise_params.mix_);

	const ColorSpace color_space = Rgb::colorSpaceFromName(color_space_str);
	ColorOutput *output = new ImageOutput(image_path, border_x, border_y, denoise_params, name, color_space, gamma, with_alpha, alpha_premultiply, multi_layer);
	output->setLoggingParams(params);
	output->setBadgeParams(params);
	return output;
}

ImageOutput::ImageOutput(const std::string &image_path, int border_x, int border_y, const DenoiseParams denoise_params, const std::string &name, const ColorSpace color_space, float gamma, bool with_alpha, bool alpha_premultiply, bool multi_layer) : ColorOutput(name, color_space, gamma, with_alpha, alpha_premultiply), image_path_(image_path), border_x_(border_x), border_y_(border_y), multi_layer_(multi_layer), denoise_params_(denoise_params)
{
	//Empty
}

ImageOutput::~ImageOutput()
{
	clearImageLayers();
}

void ImageOutput::clearImageLayers()
{
	if(image_layers_)
	{
		for(auto &it : *image_layers_) delete it.second.image_;
		delete image_layers_;
		image_layers_ = nullptr;
	}
}

void ImageOutput::init(int width, int height, const Layers *layers, const std::map<std::string, RenderView *> *render_views)
{
	ColorOutput::init(width, height, layers, render_views);
	clearImageLayers();
	image_layers_ = new ImageLayers();

	const Layers layers_exported = layers_->getLayersWithExportedImages();
	for(const auto &it : layers_exported)
	{
		{
			Image::Type image_type = it.second.getImageType();
			Image *image = new Image{width, height, image_type, Image::Optimization::None};
			image_layers_->set(it.first, {image, it.second});
		}
	}
}

bool ImageOutput::putPixel(int x, int y, const ColorLayer &color_layer)
{
	if(image_layers_)
	{
		image_layers_->setColor(x + border_x_, y + border_y_, color_layer);
		return true;
	}
	else return false;
}

void ImageOutput::flush(const RenderControl &render_control)
{
	Path path(image_path_);
	std::string directory = path.getDirectory();
	std::string base_name = path.getBaseName();
	const std::string ext = path.getExtension();
	const std::string view_name = current_render_view_->getName();
	if(view_name != "") base_name += " (view " + view_name + ")";

	ParamMap params;
	params["type"] = ext;
	Format *format = Format::factory(params);
	
	if(format)
	{
		if(multi_layer_ && format->supportsMultiLayer())
		{
			if(view_name == current_render_view_->getName())
			{
				saveImageFile(image_path_, Layer::Combined, format, render_control); //This should not be necessary but Blender API seems to be limited and the API "load_from_file" function does not work (yet) with multilayered images, so I have to generate this extra combined pass file so it's displayed in the Blender window.
			}

			if(!directory.empty()) directory += "/";
			const std::string fname_pass = directory + base_name + " (" + "multilayer" + ")." + ext;
			saveImageFileMultiChannel(fname_pass, format, render_control);

			logger__.setImagePath(fname_pass); //to show the image in the HTML log output
		}
		else
		{
			if(!directory.empty()) directory += "/";
			for(const auto &image_layer : *image_layers_)
			{
				const Layer::Type layer_type = image_layer.first;
				if(layer_type == Layer::Combined)
				{
					saveImageFile(image_path_, layer_type, format, render_control); //default imagehandler filename, when not using views nor passes and for reloading into Blender
					logger__.setImagePath(image_path_); //to show the image in the HTML log output
				}

				if(layer_type != Layer::Disabled && (image_layers_->size() > 1 || render_views_->size() > 1))
				{
					const std::string layer_type_name = Layer::getTypeName(image_layer.first);
					const std::string fname_pass = directory + base_name + " [" + layer_type_name + "]." + ext;
					saveImageFile(fname_pass, layer_type, format, render_control);
				}
			}
		}
		delete format;
	}
	if(save_log_txt_)
	{
		std::string f_log_txt_name = directory + "/" + base_name + "_log.txt";
		logger__.saveTxtLog(f_log_txt_name, badge_, render_control);
	}
	if(save_log_html_)
	{
		std::string f_log_html_name = directory + "/" + base_name + "_log.html";
		logger__.saveHtmlLog(f_log_html_name, badge_, render_control);
	}
	if(logger__.getSaveStats())
	{
		std::string f_stats_name = directory + "/" + base_name + "_stats.csv";
		logger__.statsSaveToFile(f_stats_name, /*sorted=*/ true);
	}
}

void ImageOutput::saveImageFile(const std::string &filename, const Layer::Type &layer_type, Format *format, const RenderControl &render_control)
{
	if(render_control.inProgress()) Y_INFO << name_ << ": Autosaving partial render (" << math::roundFloatPrecision(render_control.currentPassPercent(), 0.01) << "% of pass " << render_control.currentPass() << " of " << render_control.totalPasses() << ") file as \"" << filename << "\"...  " << printDenoiseParams() << YENDL;
	else Y_INFO << name_ << ": Saving file as \"" << filename << "\"...  " << printDenoiseParams() << YENDL;

	const Image *image = (*image_layers_)(layer_type).image_;

	if(!image)
	{
		Y_WARNING << name_ << ": Image does not exist (it is null) and could not be saved." << YENDL;
		return;
	}

	if(badge_.getPosition() != Badge::Position::None)
	{
		const Image *badge_image = generateBadgeImage(render_control);
		Image::Position badge_image_position = Image::Position::Bottom;
		if(badge_.getPosition() == Badge::Position::Top) badge_image_position = Image::Position::Top;
		image = Image::getComposedImage(image, badge_image, badge_image_position);
		delete badge_image;
		if(!image)
		{
			Y_WARNING << name_ << ": Image could not be composed with badge and could not be saved." << YENDL;
			return;
		}
	}

	if(denoiseEnabled())
	{
		const Image *image_denoised = Image::getDenoisedLdrImage(image, denoise_params_);
		if(image_denoised)
		{
			format->saveToFile(filename, image_denoised);
			if(image_denoised != image) delete image_denoised;
		}
		else
		{
			Y_VERBOSE << name_ << ": Denoise was not possible, saving image without denoise postprocessing." << YENDL;
			format->saveToFile(filename, image);
		}
	}
	else format->saveToFile(filename, image);

	if(with_alpha_ && !format->supportsAlpha())
	{
		Path file_path(filename);
		std::string file_name_alpha = file_path.getBaseName() + "_alpha." + file_path.getExtension();
		Y_INFO << name_ << ": Saving separate alpha channel file as \"" << filename << "\"...  " << printDenoiseParams() << YENDL;

		if(denoiseEnabled())
		{
			const Image *image_denoised = Image::getDenoisedLdrImage(image, denoise_params_);
			if(image_denoised)
			{
				format->saveAlphaChannelOnlyToFile(filename, image_denoised);
				if(image_denoised != image) delete image_denoised;
			}
			else
			{
				Y_VERBOSE << name_ << ": Denoise was not possible, saving image without denoise postprocessing." << YENDL;
				format->saveAlphaChannelOnlyToFile(filename, image);
			}
		}
		else format->saveAlphaChannelOnlyToFile(filename, image);
	}
	if(image != (*image_layers_)(layer_type).image_) delete image; //only delete the image if it's a postprocessed copy and not the original
}

void ImageOutput::saveImageFileMultiChannel(const std::string &filename, Format *format, const RenderControl &render_control)
{
	if(badge_.getPosition() != Badge::Position::None)
	{
		const Image *badge_image = generateBadgeImage(render_control);
		Image::Position badge_image_position = Image::Position::Bottom;
		if(badge_.getPosition() == Badge::Position::Top) badge_image_position = Image::Position::Top;
		ImageLayers image_layers_badge;
		for(const auto &image_layer : *image_layers_)
		{
			Image *image_layer_badge = Image::getComposedImage(image_layer.second.image_, badge_image, badge_image_position);
			image_layers_badge.set(image_layer.first, {image_layer_badge, image_layer.first});
		}
		delete badge_image;
		format->saveToFileMultiChannel(filename, &image_layers_badge);
	}
	else format->saveToFileMultiChannel(filename, image_layers_);
}

std::string ImageOutput::printDenoiseParams() const
{
#ifdef HAVE_OPENCV	//Denoise only works if YafaRay is built with OpenCV support
	if(!denoiseEnabled()) return "";
	std::stringstream param_string;
	param_string << "| Image file denoise enabled [mix=" << denoise_params_.mix_ << ", h(Luminance)=" << denoise_params_.hlum_ << ", h(Chrominance)=" << denoise_params_.hcol_ << "]" << YENDL;
	return param_string.str();
#else
	return "";
#endif
}

END_YAFARAY

