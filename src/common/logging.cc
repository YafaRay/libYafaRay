/****************************************************************************
 *      logging.cc: YafaRay Logging control
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez for original Console_Verbosity file
 *      Copyright (C) 2016 David Bluecame for all changes to convert original
 *      console output classes/objects into full Logging classes/objects
 *      and the Log and HTML file saving.
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
#include "constants.h"
#include "common/logging.h"
#include "common/file.h"
#include "common/color_console.h"
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <cmath>

BEGIN_YAFARAY

Logger::Logger()
{
}

Logger::Logger(const Logger &)	//We need to redefine the copy constructor to avoid trying to copy the mutex (not copiable). This copy constructor will not copy anything, but we only have one log object in the session anyway so it should be ok.
{
}

Logger::~Logger()
{
}


// Definition of the logging functions

void Logger::saveTxtLog(const std::string &name)
{
	if(!save_log_) return;

	std::stringstream ss;

	ss << "YafaRay Image Log file " << std::endl << std::endl;

	ss << "Image: \"" << image_path_ << "\"" << std::endl << std::endl;

	if(!title_.empty()) ss << "Title: \"" << title_ << "\"" << std::endl;
	if(!author_.empty()) ss << "Author: \"" << author_ << "\"" << std::endl;
	if(!contact_.empty()) ss << "Contact: \"" << contact_ << "\"" << std::endl;
	if(!comments_.empty()) ss << "Comments: \"" << comments_ << "\"" << std::endl;

	ss << std::endl << "Render Information:" << std::endl << "  " << render_info_ << std::endl << "  " << render_settings_ << std::endl;
	ss << std::endl << "AA/Noise Control Settings:" << std::endl << "  " << aa_noise_settings_ << std::endl;

	if(!memory_log_.empty())
	{
		ss << std::endl;

		for(auto it = memory_log_.begin() ; it != memory_log_.end(); ++it)
		{
			ss << "[" << printDate(it->event_date_time_) << " " << printTime(it->event_date_time_) << " (" << printDuration(it->event_duration_) << ")] ";

			switch(it->verbosity_level_)
			{
				case VlDebug:		ss << "DEBUG: "; break;
				case VlVerbose:	ss << "VERB: "; break;
				case VlInfo:		ss << "INFO: "; break;
				case VlParams:		ss << "PARM: "; break;
				case VlWarning:	ss << "WARNING: "; break;
				case VlError:		ss << "ERROR: "; break;
				default:			ss << "LOG: "; break;
			}

			ss << it->event_description_;
		}
	}

	File log_file(name);
	log_file.save(ss.str(), true);
}

void Logger::saveHtmlLog(const std::string &name)
{
	if(!save_html_) return;

	std::stringstream ss;

	std::string base_img_path, base_img_file_name, img_extension;

	splitPath(image_path_, base_img_path, base_img_file_name, img_extension);

	ss << "<!DOCTYPE html>" << std::endl;
	ss << "<html lang=\"en\">" << std::endl << "<head>" << std::endl << "<meta charset=\"UTF-8\">" << std::endl;

	ss << "<title>YafaRay Log: " << base_img_file_name << "." << img_extension << "</title>" << std::endl;

	ss << "<!--[if lt IE 9]>" << std::endl << "<script src=\"http://html5shiv.googlecode.com/svn/trunk/html5.js\">" << std::endl << "</script>" << std::endl << "<![endif]-->" << std::endl << std::endl;

	ss << "<style>" << std::endl << "body {font-family: Verdana, sans-serif; font-size:0.8em;}" << std::endl << "header, nav, section, article, footer" << std::endl << "{border:1px solid grey; margin:5px; padding:8px;}" << std::endl << "nav ul {margin:0; padding:0;}" << std::endl << "nav ul li {display:inline; margin:5px;}" << std::endl;

	ss << "table {" << std::endl;
	ss << "    width:100%;" << std::endl;
	ss << "}" << std::endl;
	ss << "table, th, td {" << std::endl;
	ss << "    border: 1px solid black;" << std::endl;
	ss << "    border-collapse: collapse;" << std::endl;
	ss << "}" << std::endl;
	ss << "th:first-child{" << std::endl;
	ss << "    width:1%;" << std::endl;
	ss << "    white-space:nowrap;" << std::endl;
	ss << "}" << std::endl;
	ss << "th, td {" << std::endl;
	ss << "    padding: 5px;" << std::endl;
	ss << "    text-align: left;" << std::endl;
	ss << "}" << std::endl;
	ss << "table#yafalog tr:nth-child(even) {" << std::endl;
	ss << "    background-color: #eee;" << std::endl;
	ss << "}" << std::endl;
	ss << "table#yafalog tr:nth-child(odd) {" << std::endl;
	ss << "   background-color:#fff;" << std::endl;
	ss << "}" << std::endl;
	ss << "table#yafalog th	{" << std::endl;
	ss << "    background-color: black;" << std::endl;
	ss << "    color: white;" << std::endl;
	ss << "}" << std::endl;

	ss << "</style>" << std::endl << "</head>" << std::endl << std::endl;

	ss << "<body>" << std::endl;

	//ss << "<header>" << std::endl << "<h1>YafaRay Image HTML file</h1>" << std::endl << "</header>" << std::endl;

	std::string ext_lower_case = img_extension;
	std::transform(ext_lower_case.begin(), ext_lower_case.end(), ext_lower_case.begin(), ::tolower);

	if(!image_path_.empty() && (ext_lower_case == "jpg" || ext_lower_case == "jpeg" || ext_lower_case == "png")) ss << "<a href=\"" << base_img_file_name << "." << img_extension << "\" target=\"_blank\">" << "<img src=\"" << base_img_file_name << "." << img_extension << "\" width=\"768\" alt=\"" << base_img_file_name << "." << img_extension << "\"/></a>" << std::endl;

	ss << "<p /><table id=\"yafalog\">" << std::endl;
	ss << "<tr><th>Image file:</th><td><a href=\"" << base_img_file_name << "." << img_extension << "\" target=\"_blank\"</a>" << base_img_file_name << "." << img_extension << "</td></tr>" << std::endl;
	if(!title_.empty()) ss << "<tr><th>Title:</th><td>" << title_ << "</td></tr>" << std::endl;
	if(!author_.empty()) ss << "<tr><th>Author:</th><td>" << author_ << "</td></tr>" << std::endl;
	if(!custom_icon_.empty()) ss << "<tr><th></th><td><a href=\"" << custom_icon_ << "\" target=\"_blank\">" << "<img src=\"" << custom_icon_ << "\" width=\"80\" alt=\"" << custom_icon_ << "\"/></a></td></tr>" << std::endl;
	if(!contact_.empty()) ss << "<tr><th>Contact:</th><td>" << contact_ << "</td></tr>" << std::endl;
	if(!comments_.empty()) ss << "<tr><th>Comments:</th><td>" << comments_ << "</td></tr>" << std::endl;
	ss << "</table>" << std::endl;

	ss << "<p /><table id=\"yafalog\">" << std::endl;
	ss << "<tr><th>Render Information:</th><td><p>" << render_info_ << "</p><p>" << render_settings_ << "</p></td></tr>" << std::endl;
	ss << "<tr><th>AA/Noise Control Settings:</th><td>" << aa_noise_settings_ << "</td></tr>" << std::endl;
	ss << "</table>" << std::endl;

	if(!memory_log_.empty())
	{
		ss << "<p /><table id=\"yafalog\"><th>Date</th><th>Time</th><th>Dur.</th><th>Verbosity</th><th>Description</th>" << std::endl;

		for(auto it = memory_log_.begin() ; it != memory_log_.end(); ++it)
		{
			ss << "<tr><td>" << printDate(it->event_date_time_) << "</td><td>" << printTime(it->event_date_time_) << "</td><td>" << printDuration(it->event_duration_) << "</td>";

			switch(it->verbosity_level_)
			{
				case VlDebug:		ss << "<td BGCOLOR=#ff80ff>DEBUG: "; break;
				case VlVerbose:	ss << "<td BGCOLOR=#80ff80>VERB: "; break;
				case VlInfo:		ss << "<td BGCOLOR=#40ff40>INFO: "; break;
				case VlParams:		ss << "<td BGCOLOR=#80ffff>PARM: "; break;
				case VlWarning:	ss << "<td BGCOLOR=#ffff00>WARNING: "; break;
				case VlError:		ss << "<td BGCOLOR=#ff4040>ERROR: "; break;
				default:			ss << "<td>LOG: "; break;
			}

			ss << "</td><td>" << it->event_description_ << "</td></tr>";
		}
		ss << std::endl << "</table></body></html>" << std::endl;
	}

	File log_file(name);
	log_file.save(ss.str(), true);
}

void Logger::clearMemoryLog()
{
	memory_log_.clear();
}

void Logger::clearAll()
{
	clearMemoryLog();
	statsClear();
	image_path_ = "";
	title_ = "";
	author_ = "";
	contact_ = "";
	comments_ = "";
	custom_icon_ = "";
	aa_noise_settings_ = "";
	render_settings_ = "";
}

Logger &Logger::out(int verbosity_level)
{
#if !defined(_WIN32) || defined(__MINGW32__)
	mutx_.lock();	//Don't lock if building with Visual Studio because it cause hangs when executing YafaRay in Windows 7 for some weird reason!
#else
#endif

	verbosity_level_ = verbosity_level;

	std::time_t current_datetime = std::time(nullptr);

	if(verbosity_level_ <= log_master_verbosity_level_)
	{
		if(previous_log_event_date_time_ == 0) previous_log_event_date_time_ = current_datetime;
		double duration = std::difftime(current_datetime, previous_log_event_date_time_);

		memory_log_.push_back(LogEntry(current_datetime, duration, verbosity_level_, ""));

		previous_log_event_date_time_ = current_datetime;
	}

	if(verbosity_level_ <= console_master_verbosity_level_)
	{
		if(previous_console_event_date_time_ == 0) previous_console_event_date_time_ = current_datetime;
		double duration = std::difftime(current_datetime, previous_console_event_date_time_);

		if(console_log_colors_enabled_)
		{
			switch(verbosity_level_)
			{
				case VlDebug:		std::cout << SetColor(Magenta) << "[" << printTime(current_datetime) << "] DEBUG"; break;
				case VlVerbose:	std::cout << SetColor(Green) << "[" << printTime(current_datetime) << "] VERB"; break;
				case VlInfo:		std::cout << SetColor(Green) << "[" << printTime(current_datetime) << "] INFO"; break;
				case VlParams:		std::cout << SetColor(Cyan) << "[" << printTime(current_datetime) << "] PARM"; break;
				case VlWarning:	std::cout << SetColor(Yellow) << "[" << printTime(current_datetime) << "] WARNING"; break;
				case VlError:		std::cout << SetColor(Red) << "[" << printTime(current_datetime) << "] ERROR"; break;
				default:			std::cout << SetColor(White) << "[" << printTime(current_datetime) << "] LOG"; break;
			}
		}
		else
		{
			switch(verbosity_level_)
			{
				case VlDebug:		std::cout << "[" << printTime(current_datetime) << "] DEBUG"; break;
				case VlVerbose:	std::cout << "[" << printTime(current_datetime) << "] VERB"; break;
				case VlInfo:		std::cout << "[" << printTime(current_datetime) << "] INFO"; break;
				case VlParams:		std::cout << "[" << printTime(current_datetime) << "] PARM"; break;
				case VlWarning:	std::cout << "[" << printTime(current_datetime) << "] WARNING"; break;
				case VlError:		std::cout << "[" << printTime(current_datetime) << "] ERROR"; break;
				default:			std::cout << "[" << printTime(current_datetime) << "] LOG"; break;
			}
		}

		if(duration == 0) std::cout << ": ";
		else std::cout << " (" << printDurationSimpleFormat(duration) << "): ";

		if(console_log_colors_enabled_) std::cout << SetColor();

		previous_console_event_date_time_ = current_datetime;
	}

	mutx_.unlock();

	return *this;
}

int Logger::vlevelFromString(std::string str_v_level) const
{
	int vlevel;

	if(str_v_level == "debug") vlevel = VlDebug;
	else if(str_v_level == "verbose") vlevel = VlVerbose;
	else if(str_v_level == "info") vlevel = VlInfo;
	else if(str_v_level == "params") vlevel = VlParams;
	else if(str_v_level == "warning") vlevel = VlWarning;
	else if(str_v_level == "error") vlevel = VlError;
	else if(str_v_level == "mute") vlevel = VlMute;
	else if(str_v_level == "disabled") vlevel = VlMute;
	else vlevel = VlVerbose;

	return vlevel;
}

void Logger::setConsoleMasterVerbosity(const std::string &str_v_level)
{
	int vlevel = vlevelFromString(str_v_level);
	console_master_verbosity_level_ = std::max((int)VlMute, std::min(vlevel, (int)VlDebug));
}

void Logger::setLogMasterVerbosity(const std::string &str_v_level)
{
	int vlevel = vlevelFromString(str_v_level);
	log_master_verbosity_level_ = std::max((int)VlMute, std::min(vlevel, (int)VlDebug));
}

std::string Logger::printTime(std::time_t datetime) const
{
	char mbstr[20];
	std::strftime(mbstr, sizeof(mbstr), "%H:%M:%S", std::localtime(&datetime));
	return std::string(mbstr);
}

std::string Logger::printDate(std::time_t datetime) const
{
	char mbstr[20];
	std::strftime(mbstr, sizeof(mbstr), "%Y-%m-%d", std::localtime(&datetime));
	return std::string(mbstr);
}

std::string Logger::printDuration(double duration) const
{
	std::ostringstream str_dur;

	int duration_int = (int) duration;
	int hours = duration_int / 3600;
	int minutes = (duration_int % 3600) / 60;
	int seconds = duration_int % 60;

	if(hours == 0) str_dur << "     ";
	else str_dur << "+" << std::setw(3) << hours << "h";

	if(hours == 0 && minutes == 0) str_dur << "    ";
	else if(hours == 0 && minutes != 0) str_dur << "+" << std::setw(2) << minutes << "m";
	else str_dur << " " << std::setw(2) << minutes << "m";

	if(hours == 0 && minutes == 0 && seconds == 0) str_dur << "    ";
	else if(hours == 0 && minutes == 0 && seconds != 0) str_dur << "+" << std::setw(2) << seconds << "s";
	else str_dur << " " << std::setw(2) << seconds << "s";

	return std::string(str_dur.str());
}

std::string Logger::printDurationSimpleFormat(double duration) const
{
	std::ostringstream str_dur;

	int duration_int = (int) duration;
	int hours = duration_int / 3600;
	int minutes = (duration_int % 3600) / 60;
	int seconds = duration_int % 60;

	if(hours == 0) str_dur << "";
	else str_dur << "+" << std::setw(2) << hours << "h";

	if(hours == 0 && minutes == 0) str_dur << "";
	else if(hours == 0 && minutes != 0) str_dur << "+" << std::setw(2) << minutes << "m";
	else str_dur << "" << std::setw(2) << minutes << "m";

	if(hours == 0 && minutes == 0 && seconds == 0) str_dur << "";
	else if(hours == 0 && minutes == 0 && seconds != 0) str_dur << "+" << std::setw(2) << seconds << "s";
	else str_dur << "" << std::setw(2) << seconds << "s";

	return std::string(str_dur.str());
}

void Logger::appendAaNoiseSettings(const std::string &aa_noise_settings)
{
	aa_noise_settings_ += aa_noise_settings;
}

void Logger::appendRenderSettings(const std::string &render_settings)
{
	render_settings_ += render_settings;
}

void Logger::splitPath(const std::string &full_file_path, std::string &base_path, std::string &base_file_name, std::string &extension)
{
	//DEPRECATED: use path_t instead
	Path full_path {full_file_path };
	base_path = full_path.getDirectory();
	base_file_name = full_path.getBaseName();
	extension = full_path.getExtension();
}

void Logger::setParamsBadgePosition(const std::string &badge_position)
{
	if(badge_position == "top")
	{
		draw_params_ = true;
		params_badge_top_ = true;
	}
	else if(badge_position == "bottom")
	{
		draw_params_ = true;
		params_badge_top_ = false;
	}
	else
	{
		draw_params_ = false;
		params_badge_top_ = false;
	}
}


int Logger::getBadgeHeight() const
{
	int badge_height = 0;
	if(draw_aa_noise_settings_ && draw_render_settings_) badge_height = 150;
	else if(!draw_aa_noise_settings_ && !draw_render_settings_) badge_height = 70;
	else badge_height = 110;

	badge_height = (int) std::ceil(badge_height * font_size_factor_);

	return badge_height;
}


void Logger::statsPrint(bool sorted) const
{
	std::cout << "name, index, value" << std::endl;
	std::vector<std::pair<std::string, double>> vector_print(diagnostics_stats_.begin(), diagnostics_stats_.end());
	if(sorted) std::sort(vector_print.begin(), vector_print.end());
	for(auto &it : vector_print) std::cout << std::setprecision(std::numeric_limits<double>::digits10 + 1) << it.first << it.second << std::endl;
}

void Logger::statsSaveToFile(std::string file_path, bool sorted) const
{
	//FIXME: migrate to new file_t class
	std::ofstream stats_file;
	stats_file.open(file_path);
	stats_file << "name, index, value" << std::endl;
	std::vector<std::pair<std::string, double>> vector_print(diagnostics_stats_.begin(), diagnostics_stats_.end());
	if(sorted) std::sort(vector_print.begin(), vector_print.end());
	for(auto &it : vector_print) stats_file << std::setprecision(std::numeric_limits<double>::digits10 + 1) << it.first << it.second << std::endl;
	stats_file.close();
}

void Logger::statsAdd(std::string stat_name, double stat_value, double index)
{
	std::stringstream ss;
	ss << stat_name << ", " << std::fixed << std::setfill('0') << std::setw(std::numeric_limits<int>::digits10 + 1 + std::numeric_limits<double>::digits10 + 1) << std::setprecision(std::numeric_limits<double>::digits10) << index << ", ";
#if !defined(_WIN32) || defined(__MINGW32__)
	mutx_.lock();	//Don't lock if building with Visual Studio because it cause hangs when executing YafaRay in Windows 7 for some weird reason!
#else
#endif
	diagnostics_stats_[ss.str()] += stat_value;
	mutx_.unlock();
}

void Logger::statsIncrementBucket(std::string stat_name, double stat_value, double bucket_precision_step, double increment_amount)
{
	double index = floor(stat_value / bucket_precision_step) * bucket_precision_step;
	statsAdd(stat_name, increment_amount, index);
}

END_YAFARAY

