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
#include "yafaray_conf.h"
#include "common/logger.h"
#include "common/file.h"
#include "common/badge.h"
#include "color/color_console.h"
#include <algorithm>
#include <cmath>

BEGIN_YAFARAY

void Logger::saveTxtLog(const std::string &name, const Badge &badge, const RenderControl &render_control)
{
	std::stringstream ss;

	ss << "YafaRay Image Log file " << std::endl << std::endl;

	ss << "Image: \"" << image_path_ << "\"" << std::endl << std::endl;

	if(!badge.getTitle().empty()) ss << "Title: \"" << badge.getTitle() << "\"" << std::endl;
	if(!badge.getAuthor().empty()) ss << "Author: \"" << badge.getAuthor() << "\"" << std::endl;
	if(!badge.getContact().empty()) ss << "Contact: \"" << badge.getContact() << "\"" << std::endl;
	if(!badge.getComments().empty()) ss << "Comments: \"" << badge.getComments() << "\"" << std::endl;

	ss << std::endl << "Render Information:" << std::endl << "  " << badge.getRenderInfo(render_control) << std::endl << "  " << render_control.getRenderInfo() << std::endl;
	ss << std::endl << "AA/Noise Control Settings:" << std::endl << "  " << render_control.getAaNoiseInfo() << std::endl;

	if(!memory_log_.empty())
	{
		ss << std::endl;

		for(const auto &log_entry : memory_log_)
		{
			if(print_datetime_) ss << "[" << Logger::printDate(log_entry.date_time_) << " " << Logger::printTime(log_entry.date_time_);
			ss << " (" << Logger::printDuration(log_entry.duration_) << ")";
			if(print_datetime_) ss << "] ";

			switch(log_entry.verbosity_level_)
			{
				case VlDebug: ss << "DEBUG: "; break;
				case VlVerbose: ss << "VERB: "; break;
				case VlInfo: ss << "INFO: "; break;
				case VlParams: ss << "PARM: "; break;
				case VlWarning: ss << "WARNING: "; break;
				case VlError: ss << "ERROR: "; break;
				default: ss << "LOG: "; break;
			}
			ss << log_entry.description_;
		}
	}
	File log_file(name);
	log_file.save(ss.str(), true);
}

void Logger::saveHtmlLog(const std::string &name, const Badge &badge, const RenderControl &render_control)
{
	const Path image_path(image_path_);
	const std::string base_img_path = image_path.getDirectory();
	const std::string base_img_file_name = image_path.getBaseName();
	const std::string img_extension = image_path.getExtension();

	std::stringstream ss;

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
	if(!badge.getTitle().empty()) ss << "<tr><th>Title:</th><td>" << badge.getTitle() << "</td></tr>" << std::endl;
	if(!badge.getAuthor().empty()) ss << "<tr><th>Author:</th><td>" << badge.getAuthor() << "</td></tr>" << std::endl;
	if(!badge.getIconPath().empty()) ss << "<tr><th></th><td><a href=\"" << badge.getIconPath() << "\" target=\"_blank\">" << "<img src=\"" << badge.getIconPath() << "\" width=\"80\" alt=\"" << badge.getIconPath() << "\"/></a></td></tr>" << std::endl;
	if(!badge.getContact().empty()) ss << "<tr><th>Contact:</th><td>" << badge.getContact() << "</td></tr>" << std::endl;
	if(!badge.getComments().empty()) ss << "<tr><th>Comments:</th><td>" << badge.getComments() << "</td></tr>" << std::endl;
	ss << "</table>" << std::endl;

	ss << "<p /><table id=\"yafalog\">" << std::endl;
	ss << "<tr><th>Render Information:</th><td><p>" << badge.getRenderInfo(render_control) << "</p><p>" << render_control.getRenderInfo() << "</p></td></tr>" << std::endl;
	ss << "<tr><th>AA/Noise Control Settings:</th><td>" << render_control.getAaNoiseInfo() << "</td></tr>" << std::endl;
	ss << "</table>" << std::endl;

	if(!memory_log_.empty())
	{
		ss << "<p /><table id=\"yafalog\">";
		if(print_datetime_) ss << "<th>Date</th><th>Time</th>";
		ss << "<th>Dur.</th><th>Verbosity</th><th>Description</th>" << std::endl;

		for(const auto &log_entry : memory_log_)
		{
			if(print_datetime_) ss << "<tr><td>" << Logger::printDate(log_entry.date_time_) << "</td><td>" << Logger::printTime(log_entry.date_time_) << "</td>";
			ss << "<td>" << Logger::printDuration(log_entry.duration_) << "</td>";

			switch(log_entry.verbosity_level_)
			{
				case VlDebug: ss << "<td BGCOLOR=#ff80ff>DEBUG: "; break;
				case VlVerbose: ss << "<td BGCOLOR=#80ff80>VERB: "; break;
				case VlInfo: ss << "<td BGCOLOR=#40ff40>INFO: "; break;
				case VlParams: ss << "<td BGCOLOR=#80ffff>PARM: "; break;
				case VlWarning: ss << "<td BGCOLOR=#ffff00>WARNING: "; break;
				case VlError: ss << "<td BGCOLOR=#ff4040>ERROR: "; break;
				default: ss << "<td>LOG: "; break;
			}
			ss << "</td><td>" << log_entry.description_ << "</td></tr>";
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
	image_path_.clear();
}

Logger &Logger::out(int verbosity_level)
{
#if !defined(_WIN32) || defined(__MINGW32__)
	//Don't lock if building with Visual Studio because it cause hangs when executing YafaRay in Windows 7 for some weird reason!
	std::lock_guard<std::mutex> lock_guard(mutx_);
#else
#endif
	verbosity_level_ = verbosity_level;
	const std::time_t current_datetime = std::time(nullptr);

	if(verbosity_level_ <= log_master_verbosity_level_)
	{
		if(previous_log_event_date_time_ == 0) previous_log_event_date_time_ = current_datetime;
		const double duration = std::difftime(current_datetime, previous_log_event_date_time_);
		memory_log_.push_back(LogEntry(current_datetime, duration, verbosity_level_, ""));
		previous_log_event_date_time_ = current_datetime;
	}

	if(verbosity_level_ <= console_master_verbosity_level_)
	{
		if(previous_console_event_date_time_ == 0) previous_console_event_date_time_ = current_datetime;
		const double duration = std::difftime(current_datetime, previous_console_event_date_time_);
		std::string date_time_str;
		if(print_datetime_) date_time_str = "[" + Logger::printTime(current_datetime) + "] ";
		if(console_log_colors_enabled_)
		{
			switch(verbosity_level_)
			{
				case VlDebug: std::cout << ConsoleColor(ConsoleColor::Magenta) << date_time_str << "DEBUG"; break;
				case VlVerbose: std::cout << ConsoleColor(ConsoleColor::Green) << date_time_str << "VERB"; break;
				case VlInfo: std::cout << ConsoleColor(ConsoleColor::Green) << date_time_str << "INFO"; break;
				case VlParams: std::cout << ConsoleColor(ConsoleColor::Cyan) << date_time_str << "PARM"; break;
				case VlWarning: std::cout << ConsoleColor(ConsoleColor::Yellow) << date_time_str << "WARNING"; break;
				case VlError: std::cout << ConsoleColor(ConsoleColor::Red) << date_time_str << "ERROR"; break;
				default: std::cout << ConsoleColor(ConsoleColor::White) << date_time_str << "LOG"; break;
			}
		}
		else
		{
			switch(verbosity_level_)
			{
				case VlDebug: std::cout << date_time_str << "DEBUG"; break;
				case VlVerbose: std::cout << date_time_str << "VERB"; break;
				case VlInfo: std::cout << date_time_str << "INFO"; break;
				case VlParams: std::cout << date_time_str << "PARM"; break;
				case VlWarning: std::cout << date_time_str << "WARNING"; break;
				case VlError: std::cout << date_time_str << "ERROR"; break;
				default: std::cout << date_time_str << "LOG"; break;
			}
		}
		if(duration == 0) std::cout << ": ";
		else std::cout << " (" << Logger::printDurationSimpleFormat(duration) << "): ";
		if(console_log_colors_enabled_) std::cout << ConsoleColor();
		previous_console_event_date_time_ = current_datetime;
	}
	return *this;
}

int Logger::vlevelFromString(std::string str_v_level)
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
	const int vlevel = Logger::vlevelFromString(str_v_level);
	console_master_verbosity_level_ = std::max((int)VlMute, std::min(vlevel, (int)VlDebug));
}

void Logger::setLogMasterVerbosity(const std::string &str_v_level)
{
	const int vlevel = Logger::vlevelFromString(str_v_level);
	log_master_verbosity_level_ = std::max((int)VlMute, std::min(vlevel, (int)VlDebug));
}

std::string Logger::printTime(const std::time_t &datetime)
{
	char mbstr[20];
	std::strftime(mbstr, sizeof(mbstr), "%H:%M:%S", std::localtime(&datetime));
	return std::string(mbstr);
}

std::string Logger::printDate(const std::time_t &datetime)
{
	char mbstr[20];
	std::strftime(mbstr, sizeof(mbstr), "%Y-%m-%d", std::localtime(&datetime));
	return std::string(mbstr);
}

std::string Logger::printDuration(double duration)
{
	std::stringstream str_dur;
	const int duration_int = (int) duration;
	const int hours = duration_int / 3600;
	const int minutes = (duration_int % 3600) / 60;
	const int seconds = duration_int % 60;

	if(hours == 0) str_dur << "     ";
	else str_dur << "+" << std::setw(3) << hours << "h";

	if(hours == 0 && minutes == 0) str_dur << "    ";
	else if(hours == 0) str_dur << "+" << std::setw(2) << minutes << "m";
	else str_dur << " " << std::setw(2) << minutes << "m";

	if(hours == 0 && minutes == 0 && seconds == 0) str_dur << "    ";
	else if(hours == 0 && minutes == 0 && seconds != 0) str_dur << "+" << std::setw(2) << seconds << "s";
	else str_dur << " " << std::setw(2) << seconds << "s";

	return std::string(str_dur.str());
}

std::string Logger::printDurationSimpleFormat(double duration)
{
	std::ostringstream str_dur;
	const int duration_int = (int) duration;
	const int hours = duration_int / 3600;
	const int minutes = (duration_int % 3600) / 60;
	const int seconds = duration_int % 60;

	if(hours == 0) str_dur << "";
	else str_dur << "+" << std::setw(2) << hours << "h";

	if(hours == 0 && minutes == 0) str_dur << "";
	else if(hours == 0) str_dur << "+" << std::setw(2) << minutes << "m";
	else str_dur << "" << std::setw(2) << minutes << "m";

	if(hours == 0 && minutes == 0 && seconds == 0) str_dur << "";
	else if(hours == 0 && minutes == 0 && seconds != 0) str_dur << "+" << std::setw(2) << seconds << "s";
	else str_dur << "" << std::setw(2) << seconds << "s";

	return std::string(str_dur.str());
}

void Logger::statsPrint(bool sorted) const
{
	std::cout << "name, index, value" << std::endl;
	std::vector<std::pair<std::string, double>> vector_print(diagnostics_stats_.begin(), diagnostics_stats_.end());
	if(sorted) std::sort(vector_print.begin(), vector_print.end());
	for(auto &it : vector_print) std::cout << std::setprecision(std::numeric_limits<double>::digits10 + 1) << it.first << it.second << std::endl;
}

void Logger::statsSaveToFile(const std::string &file_path, bool sorted) const
{
	File file(file_path);
	std::stringstream ss;
	ss << "name, index, value" << std::endl;
	std::vector<std::pair<std::string, double>> vector_print(diagnostics_stats_.begin(), diagnostics_stats_.end());
	if(sorted) std::sort(vector_print.begin(), vector_print.end());
	for(const auto &it : vector_print) ss << std::setprecision(std::numeric_limits<double>::digits10 + 1) << it.first << it.second << std::endl;
	file.save(ss.str(), true);
}

void Logger::statsAdd(const std::string &stat_name, double stat_value, double index)
{
	std::stringstream ss;
	ss << stat_name << ", " << std::fixed << std::setfill('0') << std::setw(std::numeric_limits<int>::digits10 + 1 + std::numeric_limits<double>::digits10 + 1) << std::setprecision(std::numeric_limits<double>::digits10) << index << ", ";
#if !defined(_WIN32) || defined(__MINGW32__)
	//Don't lock if building with Visual Studio because it cause hangs when executing YafaRay in Windows 7 for some weird reason!
	std::lock_guard<std::mutex> lock_guard(mutx_);
#endif
	diagnostics_stats_[ss.str()] += stat_value;
}

void Logger::statsIncrementBucket(std::string stat_name, double stat_value, double bucket_precision_step, double increment_amount)
{
	const double index = floor(stat_value / bucket_precision_step) * bucket_precision_step;
	statsAdd(stat_name, increment_amount, index);
}

Logger::LogLevel Logger::getMaxLogLevel() const
{
	return static_cast<LogLevel>(std::max(console_master_verbosity_level_, log_master_verbosity_level_));
}

END_YAFARAY

