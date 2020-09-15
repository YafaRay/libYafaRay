/****************************************************************************
 *
 *      imageOutput.cc: generic color output based on imageHandlers
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
#include "common/color.h"
#include "common/logging.h"
#include "common/session.h"
#include "common/file.h"
#include "common/renderpasses.h"
#include "imagehandler/imagehandler.h"

BEGIN_YAFARAY

ImageOutput::ImageOutput(ImageHandler *handle, const std::string &name, int bx, int by) : image_(handle), fname_(name), b_x_(bx), b_y_(by)
{
	Path path(name);
	Path output_path(path.getDirectory(), path.getBaseName(), "");
	//Y_DEBUG PR(name) PR(outputPath.getFullPath()) PREND;
	session__.setPathImageOutput(output_path.getFullPath());
}

bool ImageOutput::putPixel(int num_view, int x, int y, int ext_pass, const Rgba &color, bool alpha)
{
	Rgba col(0.f);
	col.set(color.r_, color.g_, color.b_, ((alpha || ext_pass > 0) ? color.a_ : 1.f));
	image_->putPixel(x + b_x_, y + b_y_, col, ext_pass);
	return true;
}

bool ImageOutput::putPixel(int num_view, int x, int y, const std::vector<Rgba> &colors, bool alpha)
{
	if(image_)
	{
		for(size_t idx = 0; idx < colors.size(); ++idx)
		{
			Rgba col(0.f);
			col.set(colors[idx].r_, colors[idx].g_, colors[idx].b_, ((alpha || idx > 0) ? colors[idx].a_ : 1.f));
			image_->putPixel(x + b_x_, y + b_y_, col, idx);
		}
	}
	return true;
}

void ImageOutput::flush(int num_view)
{
	std::string fname_pass, path, name, base_name, ext;

	size_t sep = fname_.find_last_of("\\/");
	if(sep != std::string::npos)
		name = fname_.substr(sep + 1, fname_.size() - sep - 1);

	path = fname_.substr(0, sep + 1);

	if(path == "") name = fname_;

	size_t dot = name.find_last_of(".");

	if(dot != std::string::npos)
	{
		base_name = name.substr(0, dot);
		ext  = name.substr(dot, name.size() - dot);
	}
	else
	{
		base_name = name;
		ext  = "";
	}

	const std::string view_name = passes_settings_->view_names_.at(num_view);

	if(view_name != "") base_name += " (view " + view_name + ")";

	if(image_)
	{
		if(image_->isMultiLayer())
		{
			if(num_view == 0)
			{
				saveImageFile(fname_, 0); //This should not be necessary but Blender API seems to be limited and the API "load_from_file" function does not work (yet) with multilayer EXR, so I have to generate this extra combined pass file so it's displayed in the Blender window.
			}

			fname_pass = path + base_name + " [" + "multilayer" + "]" + ext;
			saveImageFileMultiChannel(fname_pass);

			logger__.setImagePath(fname_pass); //to show the image in the HTML log output
		}
		else
		{
			const IntPassesSettings &int_passes_settings = passes_settings_->intPassesSettings();
			const ExtPassesSettings &ext_passes_settings = passes_settings_->extPassesSettings();
			for(size_t ext_pass = 0; ext_pass < ext_passes_settings.size(); ++ext_pass)
			{
				if(ext_passes_settings(ext_pass).toSave())
				{
					const PassTypes int_pass_type = ext_passes_settings(ext_pass).intPassType();
					const std::string int_pass_name = int_passes_settings.name(int_pass_type);
					const std::string ext_pass_name = ext_passes_settings(ext_pass).name();

					if(num_view == 0 && ext_pass == 0)
					{
						saveImageFile(fname_, ext_pass); //default m_imagehandler filename, when not using views nor passes and for reloading into Blender
						logger__.setImagePath(fname_); //to show the image in the HTML log output
					}

					if(int_pass_type != PassDisabled && (ext_passes_settings.size() >= 2 || passes_settings_->view_names_.size() >= 2))
					{
						fname_pass = path + base_name + " [pass " + ext_pass_name + " - " + int_pass_name + "]" + ext;
						saveImageFile(fname_pass, ext_pass);
					}
				}
			}
		}
	}

	if(logger__.getSaveLog())
	{
		std::string f_log_name = path + base_name + "_log.txt";
		logger__.saveTxtLog(f_log_name);
	}

	if(logger__.getSaveHtml())
	{
		std::string f_log_html_name = path + base_name + "_log.html";
		logger__.saveHtmlLog(f_log_html_name);
	}

	if(logger__.getSaveStats())
	{
		std::string f_stats_name = path + base_name + "_stats.csv";
		logger__.statsSaveToFile(f_stats_name, /*sorted=*/ true);
	}
}

void ImageOutput::saveImageFile(std::string filename, int idx)
{
	image_->saveToFile(filename, idx);
}

void ImageOutput::saveImageFileMultiChannel(std::string filename)
{
	image_->saveToFileMultiChannel(filename, passes_settings_);
}

std::string ImageOutput::getDenoiseParams() const {
	if(image_) return image_->getDenoiseParams();
	else return "";
}

END_YAFARAY

