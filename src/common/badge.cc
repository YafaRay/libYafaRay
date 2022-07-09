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

#include "common/version_build_info.h"
#include "common/badge.h"
#include "common/param.h"
#include "color/color.h"
#include "common/logger.h"
#include "common/timer.h"
#include "resource/yafLogoTiny.h"
#include "format/format.h"
#include "math/interpolation.h"
#include "common/string.h"
#include "image/image_manipulation.h"

namespace yafaray {

void Badge::setParams(const ParamMap &params)
{
	std::string position_str;
	params.getParam("badge_position", position_str);
	params.getParam("badge_draw_render_settings", draw_render_settings_);
	params.getParam("badge_draw_aa_noise_settings", draw_aa_);
	params.getParam("badge_author", author_);
	params.getParam("badge_title", title_);
	params.getParam("badge_contact", contact_);
	params.getParam("badge_comment", comments_);
	params.getParam("badge_icon_path", icon_path_);
	params.getParam("badge_font_path", font_path_);
	params.getParam("badge_font_size_factor", font_size_factor_);
	setPosition(position_str);
}

void Badge::setPosition(const std::string &position)
{
	if(position == "top") position_ = Badge::Position::Top;
	else if(position == "bottom") position_ = Badge::Position::Bottom;
	else position_ = Badge::Position::None;
}

std::string Badge::getFields() const
{
	std::stringstream ss_badge;
	if(!getTitle().empty()) ss_badge << getTitle() << "\n";
	if(!getAuthor().empty() && !getContact().empty()) ss_badge << getAuthor() << " | " << getContact() << "\n";
	else if(!getAuthor().empty() && getContact().empty()) ss_badge << getAuthor() << "\n";
	else if(getAuthor().empty() && !getContact().empty()) ss_badge << getContact() << "\n";
	if(!getComments().empty()) ss_badge << getComments() << "\n";
	return ss_badge.str();
}

std::string Badge::getRenderInfo(const RenderControl &render_control, const Timer &timer) const
{
	std::stringstream ss_badge;
	ss_badge << "\nYafaRay (" << buildinfo::getVersionString() << buildinfo::getBuildTypeSuffix() << ")" << " " << buildinfo::getBuildOs() << " " << buildinfo::getBuildArchitectureBits() << "bit (" << buildinfo::getBuildCompiler() << ")";
	ss_badge << std::setprecision(2);
	double times = timer.getTimeNotStopping("rendert");
	if(render_control.finished()) times = timer.getTime("rendert");
	int timem, timeh;
	Timer::splitTime(times, &times, &timem, &timeh);
	ss_badge << " | " << image_size_[Axis::X] << "x" << image_size_[Axis::Y];
	if(render_control.inProgress()) ss_badge << " | " << (render_control.resumed() ? "film loaded + " : "") << "in progress " << std::fixed << std::setprecision(1) << render_control.currentPassPercent() << "% of pass: " << render_control.currentPass() << " / " << render_control.totalPasses();
	else if(render_control.canceled()) ss_badge << " | " << (render_control.resumed() ? "film loaded + " : "") << "stopped at " << std::fixed << std::setprecision(1) << render_control.currentPassPercent() << "% of pass: " << render_control.currentPass() << " / " << render_control.totalPasses();
	else
	{
		if(render_control.resumed()) ss_badge << " | film loaded + " << render_control.totalPasses() - 1 << " passes";
		else ss_badge << " | " << render_control.totalPasses() << " passes";
	}
	//if(cx0 != 0) ssBadge << ", xstart=" << cx0;
	//if(cy0 != 0) ssBadge << ", ystart=" << cy0;
	ss_badge << " | Render time:";
	if(timeh > 0) ss_badge << " " << timeh << "h";
	if(timem > 0) ss_badge << " " << timem << "m";
	ss_badge << " " << times << "s";

	times = timer.getTimeNotStopping("rendert") + timer.getTime("prepass");
	if(render_control.finished()) times = timer.getTime("rendert") + timer.getTime("prepass");
	Timer::splitTime(times, &times, &timem, &timeh);
	ss_badge << " | Total time:";
	if(timeh > 0) ss_badge << " " << timeh << "h";
	if(timem > 0) ss_badge << " " << timem << "m";
	ss_badge << " " << times << "s";
	return ss_badge.str();
}

std::string Badge::print(const std::string &denoise_params, const RenderControl &render_control, const Timer &timer) const
{
	std::stringstream ss_badge;
	ss_badge << getFields() << "\n";
	//ss_badge << getRenderInfo(render_control) << " | " << render_control.getRenderInfo() << "\n";
	//ss_badge << render_control.getAaNoiseInfo() << " " << denoise_params;
	ss_badge << denoise_params;
	return ss_badge.str();
}

Image * Badge::generateImage(const std::string &denoise_params, const RenderControl &render_control, const Timer &timer) const
{
	if(position_ == Badge::Position::None) return nullptr;
	std::stringstream ss_badge;
	ss_badge << getFields();
	ss_badge << getRenderInfo(render_control, timer);
	if(drawRenderSettings()) ss_badge << " | " << render_control.getRenderInfo();
	if(drawAaNoiseSettings()) ss_badge << "\n" << render_control.getAaNoiseInfo();
	if(!denoise_params.empty()) ss_badge << " | " << denoise_params;

	int badge_line_count = 0;
	constexpr float line_height = 13.f; //Pixels-measured baseline line height for automatic badge height calculation
	constexpr float additional_blank_lines = 1;
	std::string line;
	while(std::getline(ss_badge, line)) ++badge_line_count;
	const int badge_height = (badge_line_count + additional_blank_lines) * std::ceil(line_height * font_size_factor_);
	auto badge_image= Image::factory(logger_, {{image_size_[Axis::X], badge_height}}, Image::Type::Color, Image::Optimization::None);

	const bool badge_text_ok = image_manipulation::drawTextInImage(logger_, badge_image, ss_badge.str(), getFontSizeFactor(), getFontPath());
	if(!badge_text_ok) logger_.logError("Badge text could not be generated!");

	std::unique_ptr<Image> logo;

	// Draw logo image
	if(!getIconPath().empty())
	{
		std::string icon_extension = getIconPath().substr(getIconPath().find_last_of('.') + 1);
		std::transform(icon_extension.begin(), icon_extension.end(), icon_extension.begin(), ::tolower);

		std::string imagehandler_type = "png";
		if(icon_extension == "jpeg") imagehandler_type = "jpg";
		else imagehandler_type = icon_extension;

		ParamMap logo_image_params;
		logo_image_params["type"] = imagehandler_type;
		auto logo_format = std::unique_ptr<Format>(Format::factory(logger_, logo_image_params));
		if(logo_format) logo = std::unique_ptr<Image>(logo_format->loadFromFile(getIconPath(), Image::Optimization::None, ColorSpace::Srgb, 1.f));
		if(!logo_format || !logo) logger_.logWarning("Badge: custom params badge icon '", getIconPath(), "' could not be loaded. Using default YafaRay icon.");
	}

	if(!logo)
	{
		ParamMap logo_image_params;
		logo_image_params["type"] = std::string("png");
		auto logo_format = std::unique_ptr<Format>(Format::factory(logger_, logo_image_params));
		if(logo_format) logo = std::unique_ptr<Image>(logo_format->loadFromMemory(logo::yafaray_tiny.data(), logo::yafaray_tiny.size(), Image::Optimization::None, ColorSpace::Srgb, 1.f));
	}

	if(logo)
	{
		int logo_width = logo->getWidth();
		int logo_height = logo->getHeight();
		if(logo_width > 80 || logo_height > 45) logger_.logWarning("Badge: custom params badge logo is quite big (", logo_width, " x ", logo_height, "). It could invade other areas in the badge. Please try to keep logo size smaller than 80 x 45, for example.");
		logo_width = std::min(logo_width, image_size_[Axis::X]);
		logo_height = std::min(logo_height, badge_height);

		for(int lx = 0; lx < logo_width; lx++)
			for(int ly = 0; ly < logo_height; ly++)
			{
				if(position_ == Badge::Position::Top) badge_image->setColor({{image_size_[Axis::X] - logo_width + lx, ly}}, logo->getColor({{lx, ly}}));
				else badge_image->setColor({{image_size_[Axis::X] - logo_width + lx, badge_height - logo_height + ly}}, logo->getColor({{lx, ly}}));
			}
	}
	else logger_.logWarning("Badge: default YafaRay params badge icon could not be loaded. No icon will be shown.");

	if(logger_.isVerbose()) logger_.logVerbose("Badge: Rendering parameters badge created.");

	return badge_image;
}

} //namespace yafaray