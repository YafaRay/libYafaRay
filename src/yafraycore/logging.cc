/****************************************************************************
 *      logging.cc: YafaRay Logging control
 *      This is part of the yafray package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez for original Console_Verbosity file
 *		Copyright (C) 2016 David Bluecame for all changes to convert original
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
#include <yafray_config.h>

__BEGIN_YAFRAY

// Initialization of the master instance of yafLog
yafarayLog_t yafLog = yafarayLog_t();


// Definition of the logging functions

void yafarayLog_t::saveTxtLog(const std::string &name)
{
	if(!mSaveLog) return;
	
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


void yafarayLog_t::saveHtmlLog(const std::string &name)
{
	if(!mSaveHTML) return;
	
	std::ofstream htmlLogFile;
	htmlLogFile.open(name.c_str());

	if(!m_MemoryLog.empty()) 
	{
		htmlLogFile << "<!DOCTYPE html>" << std::endl;
		htmlLogFile << "<html><head><meta charset=\"UTF-8\">" << std::endl;
		htmlLogFile << "<title>YafaRay Log: " << name << "</title></head>" << std::endl;
		htmlLogFile << "<body>" << std::endl;
		if(!mImagePath.empty()) htmlLogFile << "<img src=\"" << mImagePath << "\" width=\"500\"/>" << std::endl;
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

void yafarayLog_t::clearMemoryLog()
{
	m_MemoryLog.clear();
	mImagePath = "";
}

yafarayLog_t & yafarayLog_t::out(int verbosity_level)
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

int yafarayLog_t::vlevel_from_string(std::string strVLevel) const
{
	int vlevel;
	
	if(strVLevel == "debug") vlevel = VL_DEBUG;
	else if(strVLevel == "verbose") vlevel = VL_VERBOSE;
	else if(strVLevel == "info") vlevel = VL_INFO;
	else if(strVLevel == "params") vlevel = VL_PARAMS;
	else if(strVLevel == "warning") vlevel = VL_WARNING;
	else if(strVLevel == "error") vlevel = VL_ERROR;
	else if(strVLevel == "mute") vlevel = VL_MUTE;
	else if(strVLevel == "disabled") vlevel = VL_MUTE;
	else vlevel = VL_VERBOSE;
	
	return vlevel;
}

void yafarayLog_t::setConsoleMasterVerbosity(const std::string &strVLevel)
{
	int vlevel = vlevel_from_string(strVLevel);
	mConsoleMasterVerbLevel = std::max( (int)VL_MUTE , std::min( vlevel, (int)VL_DEBUG ) );
}

void yafarayLog_t::setLogMasterVerbosity(const std::string &strVLevel)
{
	int vlevel = vlevel_from_string(strVLevel);
	mLogMasterVerbLevel = std::max( (int)VL_MUTE , std::min( vlevel, (int)VL_DEBUG ) );
}

std::string yafarayLog_t::printTime(std::time_t datetime) const
{
	char mbstr[20];
	std::strftime( mbstr, sizeof(mbstr), "%H:%M:%S", std::localtime(&datetime) );
	return std::string(mbstr);
}

std::string yafarayLog_t::printDate(std::time_t datetime) const
{
	char mbstr[20];
	std::strftime( mbstr, sizeof(mbstr), "%F", std::localtime(&datetime) );
	return std::string(mbstr);
}

void yafarayLog_t::setAASettings(const std::string &aa_settings)
{
	mAASettings = aa_settings;
}

void yafarayLog_t::setIntegratorSettings(const std::string &integ_settings)
{
	mIntegratorSettings = integ_settings;
}

__END_YAFRAY

