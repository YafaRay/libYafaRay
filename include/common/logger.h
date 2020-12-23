#pragma once
/****************************************************************************
 *      logging.h: YafaRay Logging control
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

#ifndef YAFARAY_LOGGER_H
#define YAFARAY_LOGGER_H

#include "constants.h"
#include "thread.h"
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <render/render_control.h>

//for Y_DEBUG printing of variable name + value. For example:  Y_DEBUG PRTEXT(Integration1) PR(color) PR(ray.dir) PREND;
#define PRPREC(precision) << std::setprecision(precision)
#define PRTEXT(text) << ' ' << #text
#define PR(var) << ' ' << #var << '=' << var
#define PREND << YENDL

BEGIN_YAFARAY

class PhotonMap;
class Badge;
struct BsdfFlags;

class LogEntry
{
		friend class Logger;

	public:
		LogEntry(std::time_t datetime, double duration, int verb_level, std::string description): date_time_(datetime), duration_(duration), verbosity_level_(verb_level), description_(description) {}

	protected:
		std::time_t date_time_;
		double duration_;
		int verbosity_level_;
		std::string description_;
};

class LIBYAFARAY_EXPORT Logger
{
	public:
		enum { VlMute = 0, VlError, VlWarning, VlParams, VlInfo, VlVerbose, VlDebug, };
		Logger() = default;
		Logger(const Logger &) = delete; //deleting copy constructor so we can use a std::mutex as a class member (not copiable)

		void enablePrintDateTime(bool value) { print_datetime_ = value; }
		void setConsoleMasterVerbosity(const std::string &str_v_level);
		void setLogMasterVerbosity(const std::string &str_v_level);

		void setImagePath(const std::string &path) { image_path_ = path; }
		void setConsoleLogColorsEnabled(bool console_log_colors_enabled) { console_log_colors_enabled_ = console_log_colors_enabled; }

		bool getSaveStats() const { return !statsEmpty(); }
		bool getConsoleLogColorsEnabled() const { return console_log_colors_enabled_; }

		void saveTxtLog(const std::string &name, const Badge &badge, const RenderControl &render_control);
		void saveHtmlLog(const std::string &name, const Badge &badge, const RenderControl &render_control);
		void clearMemoryLog();
		void clearAll();
		Logger &out(int verbosity_level);

		void statsClear() { diagnostics_stats_.clear(); }
		void statsPrint(bool sorted = false) const;
		void statsSaveToFile(const std::string &file_path, bool sorted = false) const;
		size_t statsSize() const { return diagnostics_stats_.size(); }
		bool statsEmpty() const { return diagnostics_stats_.empty(); }

		void statsAdd(const std::string &stat_name, int stat_value, double index = 0.0) { statsAdd(stat_name, (double) stat_value, index); }
		void statsAdd(const std::string &stat_name, float stat_value, double index = 0.0) { statsAdd(stat_name, (double) stat_value, index); }
		void statsAdd(const std::string &stat_name, double stat_value, double index = 0.0);

		void statsIncrementBucket(std::string stat_name, int stat_value, double bucket_precision_step = 1.0, double increment_amount = 1.0) { statsIncrementBucket(stat_name, (double) stat_value, bucket_precision_step, increment_amount); }
		void statsIncrementBucket(std::string stat_name, float stat_value, double bucket_precision_step = 1.0, double increment_amount = 1.0) { statsIncrementBucket(stat_name, (double) stat_value, bucket_precision_step, increment_amount); }
		void statsIncrementBucket(std::string stat_name, double stat_value, double bucket_precision_step = 1.0, double increment_amount = 1.0);

		static std::string printTime(const std::time_t &datetime);
		static std::string printDuration(double duration);
		static std::string printDurationSimpleFormat(double duration);
		static std::string printDate(const std::time_t &datetime);
		static int vlevelFromString(std::string str_v_level);

		template <typename T> Logger &operator << (const T &obj);
		Logger &operator << (std::ostream & (obj)(std::ostream &));

		std::mutex mutx_;  //To try to avoid garbled output when there are several threads trying to output data to the log

	protected:
		int verbosity_level_ = VlInfo;
		int console_master_verbosity_level_ = VlInfo;
		int log_master_verbosity_level_ = VlVerbose;
		bool print_datetime_ = true;
		std::vector<LogEntry> memory_log_;	//Log entries stored in memory
		std::string image_path_ = "";
		bool console_log_colors_enabled_ = true;	//If false, will supress the colors from the Console log, to help some 3rd party software that cannot handle properly the color ANSI codes
		std::time_t previous_console_event_date_time_ = 0;
		std::time_t previous_log_event_date_time_ = 0;
		std::unordered_map <std::string, double> diagnostics_stats_;
};

template<typename T>
inline Logger &Logger::operator<<(const T &obj) {
	std::ostringstream tmp_stream;
	tmp_stream << obj;

	if(verbosity_level_ <= console_master_verbosity_level_) std::cout << tmp_stream.str();
	if(verbosity_level_ <= log_master_verbosity_level_ && !memory_log_.empty()) memory_log_.back().description_ += tmp_stream.str();
	return *this;
}

inline Logger &Logger::operator<<(std::ostream &(*obj)(std::ostream &)) {
	std::ostringstream tmp_stream;
	tmp_stream << obj;

	if(verbosity_level_ <= console_master_verbosity_level_) std::cout << tmp_stream.str();
	if(verbosity_level_ <= log_master_verbosity_level_ && !memory_log_.empty()) memory_log_.back().description_ += tmp_stream.str();
	return *this;
}


extern LIBYAFARAY_EXPORT Logger logger__;

#define Y_DEBUG logger__.out(yafaray4::Logger::VlDebug)
#define Y_VERBOSE logger__.out(yafaray4::Logger::VlVerbose)
#define Y_INFO logger__.out(yafaray4::Logger::VlInfo)
#define Y_PARAMS logger__.out(yafaray4::Logger::VlParams)
#define Y_WARNING logger__.out(yafaray4::Logger::VlWarning)
#define Y_ERROR logger__.out(yafaray4::Logger::VlError)
#define YENDL std::endl

END_YAFARAY

#endif
