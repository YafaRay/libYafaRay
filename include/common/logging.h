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

#ifndef YAFARAY_LOGGING_H
#define YAFARAY_LOGGING_H

#include "constants.h"
#include "thread.h"
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>

//for Y_DEBUG printing of variable name + value. For example:  Y_DEBUG PRTEXT(Integration1) PR(color) PR(ray.dir) PREND;
#define PRPREC(precision) << std::setprecision(precision)
#define PRTEXT(text) << ' ' << #text
#define PR(var) << ' ' << #var << '=' << var
#define PREND << YENDL

BEGIN_YAFARAY

class PhotonMap;
enum class BsdfFlags : unsigned int;

class LogEntry
{
		friend class Logger;

	public:
		LogEntry(std::time_t datetime, double duration, int verb_level, std::string description): event_date_time_(datetime), event_duration_(duration), verbosity_level_(verb_level), event_description_(description) {}

	protected:
		std::time_t event_date_time_;
		double event_duration_;
		int verbosity_level_;
		std::string event_description_;
};

inline std::ostream &operator<<(std::ostream &os, const BsdfFlags &f)
{
	os << std::hex << (unsigned int) f;
	return os;
}

class LIBYAFARAY_EXPORT Logger
{
	public:
		enum { VlMute = 0, VlError, VlWarning, VlParams, VlInfo, VlVerbose, VlDebug, };
		Logger();
		Logger(const Logger &);	//customizing copy constructor so we can use a std::mutex as a class member (not copiable)

		~Logger();

		void setConsoleMasterVerbosity(const std::string &str_v_level);
		void setLogMasterVerbosity(const std::string &str_v_level);

		void setSaveLog(bool save_log) { save_log_ = save_log; }
		void setSaveHtml(bool save_html) { save_html_ = save_html; }
		void setParamsBadgePosition(const std::string &badge_position);
		void setLoggingTitle(const std::string &title) { title_ = title; }
		void setLoggingAuthor(const std::string &author) { author_ = author; }
		void setLoggingContact(const std::string &contact) { contact_ = contact; }
		void setLoggingComments(const std::string &comments) { comments_ = comments; }
		void setLoggingCustomIcon(const std::string &icon_path) { custom_icon_ = icon_path; }
		void setLoggingFontPath(const std::string &font_path) { font_path_ = font_path; }
		void setLoggingFontSizeFactor(float &font_size_factor) { font_size_factor_ = font_size_factor; }
		void setImagePath(const std::string &path) { image_path_ = path; }
		void appendAaNoiseSettings(const std::string &aa_noise_settings);
		void appendRenderSettings(const std::string &render_settings);
		void setRenderInfo(const std::string &render_info) { render_info_ = render_info; }
		void setDrawAaNoiseSettings(bool draw_noise_settings) { draw_aa_noise_settings_ = draw_noise_settings; }
		void setDrawRenderSettings(bool draw_render_settings) { draw_render_settings_ = draw_render_settings; }
		void setConsoleLogColorsEnabled(bool console_log_colors_enabled) { console_log_colors_enabled_ = console_log_colors_enabled; }

		bool getSaveLog() const { return save_log_; }
		bool getSaveHtml() const { return save_html_; }
		bool getSaveStats() const { return !statsEmpty(); }

		bool getUseParamsBadge() { return draw_params_; }
		bool isParamsBadgeTop() { return (draw_params_ && params_badge_top_); }
		std::string getLoggingTitle() const { return title_; }
		std::string getLoggingAuthor() const { return author_; }
		std::string getLoggingContact() const { return contact_; }
		std::string getLoggingComments() const { return comments_; }
		std::string getLoggingCustomIcon() const { return custom_icon_; }
		std::string getLoggingFontPath() const { return font_path_; }
		float getLoggingFontSizeFactor() const { return font_size_factor_; }
		std::string getAaNoiseSettings() const { return aa_noise_settings_; }
		std::string getRenderSettings() const { return render_settings_; }
		bool getDrawAaNoiseSettings() { return draw_aa_noise_settings_; }
		bool getDrawRenderSettings() { return draw_render_settings_; }
		int getBadgeHeight() const;
		bool getConsoleLogColorsEnabled() const { return console_log_colors_enabled_; }

		void saveTxtLog(const std::string &name);
		void saveHtmlLog(const std::string &name);
		void clearMemoryLog();
		void clearAll();
		void splitPath(const std::string &full_file_path, std::string &base_path, std::string &base_file_name, std::string &extension);
		Logger &out(int verbosity_level);
		void setConsoleMasterVerbosity(int vlevel);
		void setLogMasterVerbosity(int vlevel);
		std::string printTime(std::time_t datetime) const;
		std::string printDuration(double duration) const;
		std::string printDurationSimpleFormat(double duration) const;
		std::string printDate(std::time_t datetime) const;
		int vlevelFromString(std::string str_v_level) const;

		void statsClear() { diagnostics_stats_.clear(); }
		void statsPrint(bool sorted = false) const;
		void statsSaveToFile(std::string file_path, bool sorted = false) const;
		size_t statsSize() const { return diagnostics_stats_.size(); }
		bool statsEmpty() const { return diagnostics_stats_.empty(); }

		void statsAdd(std::string stat_name, int stat_value, double index = 0.0) { statsAdd(stat_name, (double) stat_value, index); }
		void statsAdd(std::string stat_name, float stat_value, double index = 0.0) { statsAdd(stat_name, (double) stat_value, index); }
		void statsAdd(std::string stat_name, double stat_value, double index = 0.0);

		void statsIncrementBucket(std::string stat_name, int stat_value, double bucket_precision_step = 1.0, double increment_amount = 1.0) { statsIncrementBucket(stat_name, (double) stat_value, bucket_precision_step, increment_amount); }
		void statsIncrementBucket(std::string stat_name, float stat_value, double bucket_precision_step = 1.0, double increment_amount = 1.0) { statsIncrementBucket(stat_name, (double) stat_value, bucket_precision_step, increment_amount); }
		void statsIncrementBucket(std::string stat_name, double stat_value, double bucket_precision_step = 1.0, double increment_amount = 1.0);
		template <typename T>
		Logger &operator << (const T &obj);
		Logger &operator << (std::ostream & (obj)(std::ostream &));

		std::mutex mutx_;  //To try to avoid garbled output when there are several threads trying to output data to the log

	protected:
		int verbosity_level_ = VlInfo;
		int console_master_verbosity_level_ = VlInfo;
		int log_master_verbosity_level_ = VlVerbose;
		std::vector<LogEntry> memory_log_;	//Log entries stored in memory
		std::string image_path_ = "";
		bool params_badge_top_ = true;//If enabled, draw badge in the top of the image instead of the bottom
		bool draw_params_ = false;	//Enable/disable drawing params badge in exported images
		bool save_log_ = false;		//Enable/disable text log file saving with exported images
		bool save_html_ = false;		//Enable/disable HTML file saving with exported images
		std::string title_;
		std::string author_;
		std::string contact_;
		std::string comments_;
		std::string custom_icon_;
		std::string font_path_;
		float font_size_factor_ = 1.f;
		std::string aa_noise_settings_;
		std::string render_settings_;
		std::string render_info_;
		bool draw_aa_noise_settings_ = true;
		bool draw_render_settings_ = true;
		bool console_log_colors_enabled_ = true;	//If false, will supress the colors from the Console log, to help some 3rd party software that cannot handle properly the color ANSI codes
		std::time_t previous_console_event_date_time_ = 0;
		std::time_t previous_log_event_date_time_ = 0;
		std::unordered_map <std::string, double> diagnostics_stats_;
};

template<typename T>
inline Logger &Logger::operator<<(const T &obj) {
	std::ostringstream tmp_stream;
	tmp_stream << obj;

	if(verbosity_level_ <= console_master_verbosity_level_) std::cout << obj;
	if(verbosity_level_ <= log_master_verbosity_level_ && !memory_log_.empty()) memory_log_.back().event_description_ += tmp_stream.str();
	return *this;
}

inline Logger &Logger::operator<<(std::ostream &(*obj)(std::ostream &)) {
	std::ostringstream tmp_stream;
	tmp_stream << obj;

	if(verbosity_level_ <= console_master_verbosity_level_) std::cout << obj;
	if(verbosity_level_ <= log_master_verbosity_level_ && !memory_log_.empty()) memory_log_.back().event_description_ += tmp_stream.str();
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
