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
#include "image/badge.h"
#include "param/param.h"
#include "color/color.h"
#include "common/logger.h"
#include "common/timer.h"
#include "resource/yafLogoTiny.h"
#include "format/format.h"
#include "math/interpolation.h"
#include "common/string.h"
#include "image/image_manipulation.h"
#include "render/render_monitor.h"

namespace yafaray {

Badge::Badge(Logger &logger, bool draw_aa, bool draw_render_settings, float font_size_factor, Position position, std::string title, std::string author, std::string contact, std::string comments, std::string icon_path, std::string font_path) :
draw_aa_{draw_aa}, draw_render_settings_{draw_render_settings}, font_size_factor_{font_size_factor}, position_{position}, title_{std::move(title)}, author_{std::move(author)}, contact_{std::move(contact)}, comments_{std::move(comments)}, icon_path_{std::move(icon_path)}, font_path_{std::move(font_path)}, logger_{logger}
{
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

std::string Badge::getRenderInfo(const RenderMonitor &render_monitor, const RenderControl &render_control) const
{
	std::stringstream ss_badge;
	ss_badge << "\nYafaRay (" << buildinfo::getVersionString() << buildinfo::getBuildTypeSuffix() << ")" << " " << buildinfo::getBuildOs() << " " << buildinfo::getBuildArchitectureBits() << "bit (" << buildinfo::getBuildCompiler() << ")";
	ss_badge << std::setprecision(2);
	double times = render_control.getTimerTimeNotStopping("rendert");
	if(render_control.finished()) times = render_control.getTimerTime("rendert");
	int timem, timeh;
	Timer::splitTime(times, &times, &timem, &timeh);
	ss_badge << " | " << image_size_[Axis::X] << "x" << image_size_[Axis::Y];
	if(render_control.inProgress()) ss_badge << " | " << (render_control.resumed() ? "film loaded + " : "") << "in progress " << std::fixed << std::setprecision(1) << render_monitor.currentPassPercent() << "% of pass: " << render_monitor.currentPass() << " / " << render_monitor.totalPasses();
	else if(render_control.canceled()) ss_badge << " | " << (render_control.resumed() ? "film loaded + " : "") << "stopped at " << std::fixed << std::setprecision(1) << render_monitor.currentPassPercent() << "% of pass: " << render_monitor.currentPass() << " / " << render_monitor.totalPasses();
	else
	{
		if(render_control.resumed()) ss_badge << " | film loaded + " << render_monitor.totalPasses() - 1 << " passes";
		else ss_badge << " | " << render_monitor.totalPasses() << " passes";
	}
	//if(cx0 != 0) ssBadge << ", xstart=" << cx0;
	//if(cy0 != 0) ssBadge << ", ystart=" << cy0;
	ss_badge << " | Render time:";
	if(timeh > 0) ss_badge << " " << timeh << "h";
	if(timem > 0) ss_badge << " " << timem << "m";
	ss_badge << " " << times << "s";

	times = render_control.getTimerTimeNotStopping("rendert") + render_control.getTimerTime("prepass");
	if(render_control.finished()) times = render_control.getTimerTime("rendert") + render_control.getTimerTime("prepass");
	Timer::splitTime(times, &times, &timem, &timeh);
	ss_badge << " | Total time:";
	if(timeh > 0) ss_badge << " " << timeh << "h";
	if(timem > 0) ss_badge << " " << timem << "m";
	ss_badge << " " << times << "s";
	return ss_badge.str();
}

std::string Badge::print(const std::string &denoise_params, const RenderMonitor &render_monitor, const RenderControl &render_control) const
{
	std::stringstream ss_badge;
	ss_badge << getFields() << "\n";
	//ss_badge << getRenderInfo(render_monitor, render_control) << " | " << render_monitor.getRenderInfo() << "\n";
	//ss_badge << render_monitor.getAaNoiseInfo() << " " << denoise_params;
	ss_badge << denoise_params;
	return ss_badge.str();
}

std::unique_ptr<Image> Badge::generateImage(const std::string &denoise_params, const RenderMonitor &render_monitor, const RenderControl &render_control) const
{
	if(position_ == Badge::Position::None) return nullptr;
	std::stringstream ss_badge;
	ss_badge << getFields();
	ss_badge << getRenderInfo(render_monitor, render_control);
	if(drawRenderSettings()) ss_badge << " | " << render_monitor.getRenderInfo();
	if(drawAaNoiseSettings()) ss_badge << "\n" << render_monitor.getAaNoiseInfo();
	if(!denoise_params.empty()) ss_badge << " | " << denoise_params;

	int badge_line_count = 0;
	constexpr float line_height = 13.f; //Pixels-measured baseline line height for automatic badge height calculation
	constexpr float additional_blank_lines = 1;
	std::string line;
	while(std::getline(ss_badge, line)) ++badge_line_count;
	const int badge_height = (badge_line_count + additional_blank_lines) * std::ceil(line_height * font_size_factor_);
	Image::Params badge_params;
	badge_params.width_ = image_size_[Axis::X];
	badge_params.height_ = badge_height;
	badge_params.type_ = Image::Type::Color;
	badge_params.image_optimization_ = Image::Optimization::None;
	auto badge_image= Image::factory(badge_params);

	const bool badge_text_ok = image_manipulation::drawTextInImage(logger_, badge_image.get(), ss_badge.str(), getFontSizeFactor(), getFontPath());
	if(!badge_text_ok) logger_.logError("Badge text could not be generated!");

	std::unique_ptr<Image> logo;

	// Draw logo image
	if(!getIconPath().empty())
	{
		std::string icon_extension = getIconPath().substr(getIconPath().find_last_of('.') + 1);
		std::transform(icon_extension.begin(), icon_extension.end(), icon_extension.begin(), ::tolower);
		ParamMap logo_image_params;
		logo_image_params["type"] = icon_extension;
		auto logo_format{Format::factory(logger_, logo_image_params).first};
		if(logo_format) logo = logo_format->loadFromFile(getIconPath(), Image::Optimization::None, ColorSpace::Srgb, 1.f);
		if(!logo_format || !logo) logger_.logWarning("Badge: custom params badge icon '", getIconPath(), "' could not be loaded. Using default YafaRay icon.");
	}

	if(!logo)
	{
		ParamMap logo_image_params;
		logo_image_params["type"] = std::string("png");
		auto logo_format{Format::factory(logger_, logo_image_params).first};
		if(logo_format) logo = logo_format->loadFromMemory(logo::yafaray_tiny.data(), logo::yafaray_tiny.size(), Image::Optimization::None, ColorSpace::Srgb, 1.f);
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