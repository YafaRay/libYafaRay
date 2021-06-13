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

#include "yafaray_conf.h"
#include "color/color_console.h"
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <mutex>

BEGIN_YAFARAY

class PhotonMap;
class Badge;
struct ConsoleColor;
struct BsdfFlags;
class RenderControl;

class LogEntry final
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

class Logger final
{
	public:
		Logger(const ::yafaray_LoggerCallback_t logger_callback = nullptr, void *callback_user_data = nullptr, ::yafaray_DisplayConsole_t logger_display_console = YAFARAY_DISPLAY_CONSOLE_NORMAL) : logger_callback_(logger_callback), callback_user_data_(callback_user_data), logger_display_console_(logger_display_console) { }
		Logger(const Logger &) = delete; //deleting copy constructor so we can use a std::mutex as a class member (not copiable)

		::yafaray_LogLevel_t getMaxLogLevel() const;
		bool isVerbose() const { return getMaxLogLevel() >= ::YAFARAY_LOG_LEVEL_VERBOSE; }
		bool isDebug() const { return getMaxLogLevel() >= ::YAFARAY_LOG_LEVEL_DEBUG; }

		void enablePrintDateTime(bool value) { print_datetime_ = value; }
		void setConsoleMasterVerbosity(const ::yafaray_LogLevel_t &log_level);
		void setLogMasterVerbosity(const ::yafaray_LogLevel_t &log_level);

		void setImagePath(const std::string &path) { image_path_ = path; }
		void setConsoleLogColorsEnabled(bool console_log_colors_enabled) { console_log_colors_enabled_ = console_log_colors_enabled; }

		bool getSaveStats() const { return !statsEmpty(); }
		bool getConsoleLogColorsEnabled() const { return console_log_colors_enabled_; }

		void saveTxtLog(const std::string &name, const Badge &badge, const RenderControl &render_control);
		void saveHtmlLog(const std::string &name, const Badge &badge, const RenderControl &render_control);
		void clearMemoryLog();
		void clearAll();

		void stringstreamBuilder(std::stringstream &ss) { }
		template <class Arg0, class ...Args> void stringstreamBuilder(std::stringstream &ss, const Arg0 &arg_0, const Args &...args);
		template <class ...Args> void log(int verbosity_level, const Args &...args);
		template <class ...Args> void logDebug(const Args &...args) { log(::YAFARAY_LOG_LEVEL_DEBUG, args...); }
		template <class ...Args> void logVerbose(const Args &...args) { log(::YAFARAY_LOG_LEVEL_VERBOSE, args...); }
		template <class ...Args> void logInfo(const Args &...args) { log(::YAFARAY_LOG_LEVEL_INFO, args...); }
		template <class ...Args> void logParams(const Args &...args) { log(::YAFARAY_LOG_LEVEL_PARAMS, args...); }
		template <class ...Args> void logWarning(const Args &...args) { log(::YAFARAY_LOG_LEVEL_WARNING, args...); }
		template <class ...Args> void logError(const Args &...args) { log(::YAFARAY_LOG_LEVEL_ERROR, args...); }

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
		static std::string logLevelStringFromLevel(int v_level);
		static ConsoleColor consoleColorFromLevel(int v_level);
		static std::string htmlColorFromLevel(int v_level);

	private:
		int console_master_verbosity_level_ = YAFARAY_LOG_LEVEL_INFO;
		int log_master_verbosity_level_ = YAFARAY_LOG_LEVEL_VERBOSE;
		bool print_datetime_ = true;
		std::vector<LogEntry> memory_log_;	//Log entries stored in memory
		std::string image_path_ = "";
		bool console_log_colors_enabled_ = true;	//If false, will supress the colors from the Console log, to help some 3rd party software that cannot handle properly the color ANSI codes
		std::time_t previous_console_event_date_time_ = 0;
		std::time_t previous_log_event_date_time_ = 0;
		const yafaray_LoggerCallback_t logger_callback_ = nullptr;
		void *callback_user_data_ = nullptr;
		::yafaray_DisplayConsole_t logger_display_console_;
		std::unordered_map <std::string, double> diagnostics_stats_;
		std::mutex mutx_;  //To try to avoid garbled output when there are several threads trying to output data to the log
};

template <class Arg0, class ...Args> void Logger::stringstreamBuilder(std::stringstream &ss, const Arg0 &arg_0, const Args &...args)
{
	ss << arg_0;
	stringstreamBuilder(ss, args...);
}



template <class ...Args> void Logger::log(int verbosity_level, const Args &...args)
{
	if(verbosity_level > getMaxLogLevel()) return;
	const std::time_t current_datetime = std::time(nullptr);
	const std::string time_of_day = Logger::printTime(current_datetime);
	std::stringstream ss;
	std::lock_guard<std::mutex> lock_guard(mutx_);
	stringstreamBuilder(ss, args...);
	const std::string description = ss.str();

	if(verbosity_level <= log_master_verbosity_level_)
	{
		if(previous_log_event_date_time_ == 0) previous_log_event_date_time_ = current_datetime;
		const double duration = std::difftime(current_datetime, previous_log_event_date_time_);
		memory_log_.push_back(LogEntry(current_datetime, duration, verbosity_level, description));
		previous_log_event_date_time_ = current_datetime;
	}

	if(logger_callback_)
	{
		logger_callback_(static_cast<yafaray_LogLevel_t>(verbosity_level), current_datetime, time_of_day.c_str(), description.c_str(), callback_user_data_);
	}

	if(logger_display_console_ == YAFARAY_DISPLAY_CONSOLE_NORMAL && verbosity_level <= console_master_verbosity_level_)
	{
		if(previous_console_event_date_time_ == 0) previous_console_event_date_time_ = current_datetime;
		const double duration = std::difftime(current_datetime, previous_console_event_date_time_);
		std::string date_time_str;
		if(print_datetime_) date_time_str = "[" + Logger::printTime(current_datetime) + "] ";
		if(console_log_colors_enabled_) std::cout << ConsoleColor(Logger::consoleColorFromLevel(verbosity_level));
		std::cout << date_time_str << Logger::logLevelStringFromLevel(verbosity_level);
		if(duration == 0) std::cout << ": ";
		else std::cout << " (" << Logger::printDurationSimpleFormat(duration) << "): ";
		if(console_log_colors_enabled_) std::cout << ConsoleColor();
		std::cout << description << std::endl;
		previous_console_event_date_time_ = current_datetime;
	}
}

END_YAFARAY

#endif
