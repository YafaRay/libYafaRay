#pragma once
/****************************************************************************
 *      logging.h: YafaRay Logging control
 *      This is part of the yafray package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez for original Console_Verbosity file
 *		Copyright (C) 2016 David Bluecame for all changes to convert original
 * 		console output classes/objects into full Logging classes/objects
 * 		and the Log and HTML file saving.
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
 
#ifndef Y_CONSOLE_VERBOSITY_H
#define Y_CONSOLE_VERBOSITY_H

#include <yafray_constants.h>
#include <utilities/threadUtils.h>
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
#define PREND << yendl

__BEGIN_YAFRAY

class photonMap_t;

enum
{
	VL_MUTE = 0,
	VL_ERROR,
	VL_WARNING,
	VL_PARAMS,
	VL_INFO,
	VL_VERBOSE,
	VL_DEBUG,
};

class YAFRAYCORE_EXPORT logEntry_t
{
	friend class yafarayLog_t;
	
	public:
		logEntry_t(std::time_t datetime, double duration, int verb_level, std::string description):eventDateTime(datetime),eventDuration(duration),mVerbLevel(verb_level),eventDescription(description) {}

	protected:
		std::time_t eventDateTime;
		double eventDuration;
		int mVerbLevel;
		std::string eventDescription;
};
	

class YAFRAYCORE_EXPORT yafarayLog_t
{
	public:
		yafarayLog_t();
		yafarayLog_t(const yafarayLog_t&);	//customizing copy constructor so we can use a std::mutex as a class member (not copiable)
		
		~yafarayLog_t();

		void setConsoleMasterVerbosity(const std::string &strVLevel);
		void setLogMasterVerbosity(const std::string &strVLevel);

		void setSaveLog(bool save_log) { mSaveLog = save_log; }
		void setSaveHTML(bool save_html) { mSaveHTML = save_html; }
		void setParamsBadgePosition(const std::string &badgePosition);
		void setLoggingTitle(const std::string &title) { mLoggingTitle = title; }
		void setLoggingAuthor(const std::string &author) { mLoggingAuthor = author; }
		void setLoggingContact(const std::string &contact) { mLoggingContact = contact; }
		void setLoggingComments(const std::string &comments) { mLoggingComments = comments; }
		void setLoggingCustomIcon(const std::string &iconPath) { mLoggingCustomIcon = iconPath; }
		void setLoggingFontPath(const std::string &fontPath) { mLoggingFontPath = fontPath; }
		void setLoggingFontSizeFactor(float &fontSizeFactor) { mLoggingFontSizeFactor = fontSizeFactor; }
		void setImagePath(const std::string &path) { mImagePath = path; }
		void appendAANoiseSettings(const std::string &aa_noise_settings);
		void appendRenderSettings(const std::string &render_settings);
		void setRenderInfo(const std::string &render_info) { mRenderInfo = render_info; }
		void setDrawAANoiseSettings(bool draw_noise_settings) { drawAANoiseSettings = draw_noise_settings; }
		void setDrawRenderSettings(bool draw_render_settings) { drawRenderSettings = draw_render_settings; }
		void setConsoleLogColorsEnabled(bool console_log_colors_enabled) { mConsoleLogColorsEnabled = console_log_colors_enabled; }

		bool getSaveLog() const { return mSaveLog; }
		bool getSaveHTML() const { return mSaveHTML; }
		bool getSaveStats() const { return !statsEmpty(); }

		bool getUseParamsBadge() { return mDrawParams; }
		bool isParamsBadgeTop() { return (mDrawParams && mParamsBadgeTop); }
		std::string getLoggingTitle() const { return mLoggingTitle; }
		std::string getLoggingAuthor() const { return mLoggingAuthor; }
		std::string getLoggingContact() const { return mLoggingContact; }
		std::string getLoggingComments() const { return mLoggingComments; }
		std::string getLoggingCustomIcon() const { return mLoggingCustomIcon; }
		std::string getLoggingFontPath() const { return mLoggingFontPath; }
		float getLoggingFontSizeFactor() const { return mLoggingFontSizeFactor; }
		std::string getAANoiseSettings() const { return mAANoiseSettings; }
		std::string getRenderSettings() const { return mRenderSettings; }
		bool getDrawAANoiseSettings() { return drawAANoiseSettings; }
		bool getDrawRenderSettings() { return drawRenderSettings; }
		int getBadgeHeight() const;
		bool getConsoleLogColorsEnabled() const { return mConsoleLogColorsEnabled; }
		
		void saveTxtLog(const std::string &name);
		void saveHtmlLog(const std::string &name);
		void clearMemoryLog();
		void clearAll();
		void splitPath(const std::string &fullFilePath, std::string &basePath, std::string &baseFileName, std::string &extension);
		yafarayLog_t &out(int verbosity_level);
		void setConsoleMasterVerbosity(int vlevel);
		void setLogMasterVerbosity(int vlevel);
		std::string printTime(std::time_t datetime) const;
		std::string printDuration(double duration) const;
		std::string printDurationSimpleFormat(double duration) const;
		std::string printDate(std::time_t datetime) const;
		int vlevel_from_string(std::string strVLevel) const;
		
		void statsClear() { mDiagStats.clear(); }
		void statsPrint(bool sorted=false) const;
		void statsSaveToFile(std::string filePath, bool sorted=false) const;
		size_t statsSize() const { return mDiagStats.size(); }
		bool statsEmpty() const { return mDiagStats.empty(); }

		void statsAdd(std::string statName, int statValue, double index=0.0) { statsAdd(statName, (double) statValue, index); }
		void statsAdd(std::string statName, float statValue, double index=0.0) { statsAdd(statName, (double) statValue, index); }
		void statsAdd(std::string statName, double statValue, double index=0.0);

		void statsIncrementBucket(std::string statName, int statValue, double bucketPrecisionStep=1.0, double incrementAmount=1.0) { statsIncrementBucket(statName, (double) statValue, bucketPrecisionStep, incrementAmount); }
		void statsIncrementBucket(std::string statName, float statValue, double bucketPrecisionStep=1.0, double incrementAmount=1.0) { statsIncrementBucket(statName, (double) statValue, bucketPrecisionStep, incrementAmount); }
		void statsIncrementBucket(std::string statName, double statValue, double bucketPrecisionStep=1.0, double incrementAmount=1.0);

		std::mutex mutx;  //To try to avoid garbled output when there are several threads trying to output data to the log

		template <typename T>
		yafarayLog_t & operator << ( const T &obj )
		{
			std::ostringstream tmpStream;
			tmpStream << obj;

			if(mVerbLevel <= mConsoleMasterVerbLevel) std::cout << obj;
			if(mVerbLevel <= mLogMasterVerbLevel && !m_MemoryLog.empty()) m_MemoryLog.back().eventDescription += tmpStream.str();
			return *this;
		}

		yafarayLog_t & operator << ( std::ostream& (obj)(std::ostream&) )
		{
			std::ostringstream tmpStream;
			tmpStream << obj;

			if(mVerbLevel <= mConsoleMasterVerbLevel) std::cout << obj;
			if(mVerbLevel <= mLogMasterVerbLevel && !m_MemoryLog.empty()) m_MemoryLog.back().eventDescription += tmpStream.str();
			return *this;
		}

	protected:
		int mVerbLevel = VL_INFO;
		int mConsoleMasterVerbLevel = VL_INFO;
		int mLogMasterVerbLevel = VL_VERBOSE;
		std::vector<logEntry_t> m_MemoryLog;	//Log entries stored in memory
		std::string mImagePath = "";
		bool mParamsBadgeTop = true;//If enabled, draw badge in the top of the image instead of the bottom
		bool mDrawParams = false;	//Enable/disable drawing params badge in exported images
		bool mSaveLog = false;		//Enable/disable text log file saving with exported images
		bool mSaveHTML = false;		//Enable/disable HTML file saving with exported images
		std::string mLoggingTitle;
		std::string mLoggingAuthor;
		std::string mLoggingContact;
		std::string mLoggingComments;
		std::string mLoggingCustomIcon;
		std::string mLoggingFontPath;
		float mLoggingFontSizeFactor = 1.f;
		std::string mAANoiseSettings;
		std::string mRenderSettings;
		std::string mRenderInfo;
		bool drawAANoiseSettings = true;
		bool drawRenderSettings = true;
		bool mConsoleLogColorsEnabled = true;	//If false, will supress the colors from the Console log, to help some 3rd party software that cannot handle properly the color ANSI codes
		std::time_t previousConsoleEventDateTime = 0;
		std::time_t previousLogEventDateTime = 0;
		std::unordered_map <std::string,double> mDiagStats;
};

extern YAFRAYCORE_EXPORT yafarayLog_t yafLog;

#define Y_DEBUG yafLog.out(VL_DEBUG)
#define Y_VERBOSE yafLog.out(VL_VERBOSE)
#define Y_INFO yafLog.out(VL_INFO)
#define Y_PARAMS yafLog.out(VL_PARAMS)
#define Y_WARNING yafLog.out(VL_WARNING)
#define Y_ERROR yafLog.out(VL_ERROR)
#define yendl std::endl

__END_YAFRAY

#endif
