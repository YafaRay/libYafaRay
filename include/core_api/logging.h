/****************************************************************************
 *      logging.h: YafaRay Logging control
 *      This is part of the yafray package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
 *		Copyright (C) 2016 David Bluecame for changes to convert original
 * 		console output classes/objects into full Logging classes/objects
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

#include <iostream>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

__BEGIN_YAFRAY

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
public:
	logEntry_t(std::time_t datetime, int verb_level, std::string description):eventDateTime(datetime),mVerbLevel(verb_level),eventDescription(description) {}
	std::time_t eventDateTime;
	int mVerbLevel;
	std::string eventDescription;
};
	

class YAFRAYCORE_EXPORT yafarayLog_t
{
	int mVerbLevel;
	int mConsoleMasterVerbLevel;
	int mLogMasterVerbLevel;
	std::vector<logEntry_t> m_MemoryLog;	//Log entries stored in memory
	std::string imagePath;

public:

	yafarayLog_t(): mVerbLevel(VL_INFO), mConsoleMasterVerbLevel(VL_INFO), mLogMasterVerbLevel(VL_VERBOSE), imagePath("") {}
	
	~yafarayLog_t() {}

	void setImagePath(const std::string &path)
	{
		imagePath = path;
	}

	void saveTxtLog(const std::string &name)
	{
		std::ofstream txtLogFile;
		txtLogFile.open(name.c_str());

		if(!m_MemoryLog.empty()) 
		{
			for (std::vector<logEntry_t>::iterator it = m_MemoryLog.begin() ; it != m_MemoryLog.end(); ++it)
			{
				txtLogFile << "[" << printDate(it->eventDateTime) << " " << printTime(it->eventDateTime) << "] ";

				switch(it->mVerbLevel)
				{
					case VL_DEBUG:		txtLogFile << "**DEBUG**: "; break;
					case VL_VERBOSE:	txtLogFile << "VERB: "; break;
					case VL_INFO:		txtLogFile << "INFO: "; break;
					case VL_PARAMS:		txtLogFile << "PARM: "; break;
					case VL_WARNING:	txtLogFile << "WARNING: "; break;
					case VL_ERROR:		txtLogFile << "ERROR: "; break;
					default:			txtLogFile << "LOG: "; break;
				}

				txtLogFile << it->eventDescription;
			}
		}
	}


	void saveHtmlLog(const std::string &name)
	{
		std::ofstream htmlLogFile;
		htmlLogFile.open(name.c_str());

		if(!m_MemoryLog.empty()) 
		{
			htmlLogFile << "<!DOCTYPE html>" << std::endl;
			htmlLogFile << "<html><head><meta charset=\"UTF-8\">" << std::endl;
			htmlLogFile << "<title>YafaRay Log: " << name << "</title></head>" << std::endl;
			htmlLogFile << "<body>" << std::endl;
			if(!imagePath.empty()) htmlLogFile << "<img src=\"" << imagePath << "\" width=\"500\"/>" << std::endl;
			htmlLogFile << "<table><th>Date</th><th>Time</th><th>Severity</th><th>Description</th>" << std::endl;
			
			for (std::vector<logEntry_t>::iterator it = m_MemoryLog.begin() ; it != m_MemoryLog.end(); ++it)
			{
				htmlLogFile << "<tr><td>" << printDate(it->eventDateTime) << "</td><td>" << printTime(it->eventDateTime) << "</td>";

				switch(it->mVerbLevel)
				{
					case VL_DEBUG:		htmlLogFile << "<td BGCOLOR=#ff80ff>**DEBUG**: "; break;
					case VL_VERBOSE:	htmlLogFile << "<td BGCOLOR=#80ff80>VERB: "; break;
					case VL_INFO:		htmlLogFile << "<td BGCOLOR=#40ff40>INFO: "; break;
					case VL_PARAMS:		htmlLogFile << "<td BGCOLOR=#80ffff>PARM: "; break;
					case VL_WARNING:	htmlLogFile << "<td BGCOLOR=#ffff00>WARNING: "; break;
					case VL_ERROR:		htmlLogFile << "<td BGCOLOR=#ff4040>ERROR: "; break;
					default:			htmlLogFile << "<td>LOG: "; break;
				}

				htmlLogFile << "</td><td>" << it->eventDescription << "</td></tr>";
			}
			htmlLogFile << std::endl << "</table></body></html>" << std::endl;
		}
	}

	void clearMemoryLog()
	{
		m_MemoryLog.clear();
		imagePath = "";
	}

	yafarayLog_t &out(int verbosity_level)
	{
		mVerbLevel = verbosity_level;
		
		std::time_t current_datetime = std::time(NULL);
		if(mVerbLevel <= mLogMasterVerbLevel) m_MemoryLog.push_back(logEntry_t(current_datetime, mVerbLevel, ""));
		
		if(mVerbLevel <= mConsoleMasterVerbLevel) 
		{
			switch(mVerbLevel)
			{
				case VL_DEBUG:		std::cout << setColor(Magenta) << "[" << printTime(current_datetime) << "] **DEBUG**: " << setColor(); break;
				case VL_VERBOSE:	std::cout << setColor(Green) << "[" << printTime(current_datetime) << "] VERB: " << setColor(); break;
				case VL_INFO:		std::cout << setColor(Green) << "[" << printTime(current_datetime) << "] INFO: " << setColor(); break;
				case VL_PARAMS:		std::cout << setColor(Cyan) << "[" << printTime(current_datetime) << "] PARM: " << setColor(); break;
				case VL_WARNING:	std::cout << setColor(Yellow) << "[" << printTime(current_datetime) << "] WARNING: " << setColor(); break;
				case VL_ERROR:		std::cout << setColor(Red) << "[" << printTime(current_datetime) << "] ERROR: " << setColor(); break;
				default:			std::cout << setColor(White) << "[" << printTime(current_datetime) << "] LOG: " << setColor(); break;
			}
		}
		return *this;
	}
	
	void setMasterVerbosity(int vlevel)
	{
		mConsoleMasterVerbLevel = std::max( (int)VL_MUTE , std::min( vlevel, (int)VL_DEBUG ) );
	}

	template <typename T>
	yafarayLog_t &operator << ( const T &obj )
	{
		std::ostringstream tmpStream;
		tmpStream << obj;

		if(mVerbLevel <= mConsoleMasterVerbLevel) std::cout << obj;
		//if(mLogOutput && mVerbLevel <= mLogMasterVerbLevel) (*mLogOutput) << obj;
		if(mVerbLevel <= mLogMasterVerbLevel && !m_MemoryLog.empty()) m_MemoryLog.back().eventDescription += tmpStream.str();
		return *this;
	}

	yafarayLog_t &operator << ( std::ostream& (obj)(std::ostream&) )
	{
		std::ostringstream tmpStream;
		tmpStream << obj;

		if(mVerbLevel <= mConsoleMasterVerbLevel) std::cout << obj;
		//if(mLogOutput && mVerbLevel <= mLogMasterVerbLevel) (*mLogOutput) << obj;
		if(mVerbLevel <= mLogMasterVerbLevel && !m_MemoryLog.empty()) m_MemoryLog.back().eventDescription += tmpStream.str();
		return *this;
	}
	
	std::string printTime(std::time_t datetime)
	{
		char mbstr[20];
		std::strftime( mbstr, sizeof(mbstr), "%H:%M:%S", std::localtime(&datetime) );
		return std::string(mbstr);
	}

	std::string printDate(std::time_t datetime)
	{
		char mbstr[20];
		std::strftime( mbstr, sizeof(mbstr), "%F", std::localtime(&datetime) );
		return std::string(mbstr);
	}

};

extern YAFRAYCORE_EXPORT yafarayLog_t yafLog;

__END_YAFRAY

#endif
