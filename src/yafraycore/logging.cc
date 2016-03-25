/****************************************************************************
 *      logging.cc: YafaRay Logging control
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
#include <yafray_config.h>
#include <algorithm>

__BEGIN_YAFRAY

// Initialization of the master instance of yafLog
yafarayLog_t yafLog = yafarayLog_t();


// Definition of the logging functions

void yafarayLog_t::saveTxtLog(const std::string &name)
{
	if(!mSaveLog) return;
	
	std::ofstream txtLogFile;
	txtLogFile.open(name.c_str());

	txtLogFile << "YafaRay Image Log file " << std::endl << std::endl;

	txtLogFile << "Image: \"" << mImagePath << "\"" << std::endl << std::endl;
	
	if(!mLoggingTitle.empty()) txtLogFile << "Title: \"" << mLoggingTitle << "\"" << std::endl;
	if(!mLoggingAuthor.empty()) txtLogFile << "Author: \"" << mLoggingAuthor << "\"" <<  std::endl;
	if(!mLoggingContact.empty()) txtLogFile << "Contact: \"" << mLoggingContact << "\"" <<  std::endl;
	if(!mLoggingComments.empty()) txtLogFile << "Comments: \"" << mLoggingComments << "\"" <<  std::endl;

	txtLogFile << std::endl << "Render Settings:" << std::endl << "  " << mRenderSettings << std::endl;
	txtLogFile << std::endl << "AA and Noise Control Settings:" << std::endl << "  " << mAANoiseSettings << std::endl;

	if(!m_MemoryLog.empty()) 
	{
		txtLogFile << std::endl;
		
		for (std::vector<logEntry_t>::iterator it = m_MemoryLog.begin() ; it != m_MemoryLog.end(); ++it)
		{
			txtLogFile << "[" << printDate(it->eventDateTime) << " " << printTime(it->eventDateTime) << "] ";

			switch(it->mVerbLevel)
			{
				case VL_DEBUG:		txtLogFile << "DEBUG: "; break;
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

	std::string baseImgPath, baseImgFileName, imgExtension;

	splitPath(mImagePath, baseImgPath, baseImgFileName, imgExtension);
	
	htmlLogFile << "<!DOCTYPE html>" << std::endl;
	htmlLogFile << "<html lang=\"en\">" << std::endl << "<head>" << std::endl << "<meta charset=\"UTF-8\">" << std::endl;
	
	htmlLogFile << "<title>YafaRay Log: " << baseImgFileName << imgExtension << "</title>" << std::endl;
	
	htmlLogFile << "<!--[if lt IE 9]>" << std::endl << "<script src=\"http://html5shiv.googlecode.com/svn/trunk/html5.js\">" << std::endl << "</script>" << std::endl << "<![endif]-->" << std::endl << std::endl;

	htmlLogFile << "<style>" << std::endl << "body {font-family: Verdana, sans-serif; font-size:0.8em;}" << std::endl << "header, nav, section, article, footer" << std::endl << "{border:1px solid grey; margin:5px; padding:8px;}" << std::endl << "nav ul {margin:0; padding:0;}" << std::endl << "nav ul li {display:inline; margin:5px;}" << std::endl;
	
	htmlLogFile << "table {" << std::endl;
	htmlLogFile << "    width:100%;" << std::endl;
	htmlLogFile << "}" << std::endl;
	htmlLogFile << "table, th, td {" << std::endl;
	htmlLogFile << "    border: 1px solid black;" << std::endl;
	htmlLogFile << "    border-collapse: collapse;" << std::endl;
	htmlLogFile << "}" << std::endl;
	htmlLogFile << "th:first-child{" << std::endl;
	htmlLogFile << "    width:1%;" << std::endl;
	htmlLogFile << "    white-space:nowrap;" << std::endl;
	htmlLogFile << "}" << std::endl;
	htmlLogFile << "th, td {" << std::endl;
	htmlLogFile << "    padding: 5px;" << std::endl;
	htmlLogFile << "    text-align: left;" << std::endl;
	htmlLogFile << "}" << std::endl;
	htmlLogFile << "table#yafalog tr:nth-child(even) {" << std::endl;
	htmlLogFile << "    background-color: #eee;" << std::endl;
	htmlLogFile << "}" << std::endl;
	htmlLogFile << "table#yafalog tr:nth-child(odd) {" << std::endl;
	htmlLogFile << "   background-color:#fff;" << std::endl;
	htmlLogFile << "}" << std::endl;
	htmlLogFile << "table#yafalog th	{" << std::endl;
	htmlLogFile << "    background-color: black;" << std::endl;
	htmlLogFile << "    color: white;" << std::endl;
	htmlLogFile << "}" << std::endl;

	htmlLogFile << "</style>" << std::endl << "</head>" << std::endl << std::endl;

	htmlLogFile << "<body>" << std::endl;

	//htmlLogFile << "<header>" << std::endl << "<h1>YafaRay Image HTML file</h1>" << std::endl << "</header>" << std::endl;
	
	std::string extLowerCase = imgExtension;
	std::transform(extLowerCase.begin(), extLowerCase.end(),extLowerCase.begin(), ::tolower);
	
	if(!mImagePath.empty() && (extLowerCase == ".jpg" || extLowerCase == ".jpeg" || extLowerCase == ".png")) htmlLogFile << "<a href=\"" << baseImgFileName << imgExtension << "\" target=\"_blank\">" << "<img src=\"" << baseImgFileName << imgExtension << "\" width=\"768\" alt=\"" << baseImgFileName << imgExtension << "\"/></a>" << std::endl;

	htmlLogFile << "<p /><table id=\"yafalog\">" << std::endl;
	htmlLogFile << "<tr><th>Image file:</th><td><a href=\"" << baseImgFileName << imgExtension << "\" target=\"_blank\"</a>" << baseImgFileName << imgExtension << "</td></tr>" << std::endl;
	if(!mLoggingTitle.empty()) htmlLogFile << "<tr><th>Title:</th><td>" << mLoggingTitle << "</td></tr>" << std::endl;
	if(!mLoggingAuthor.empty()) htmlLogFile << "<tr><th>Author:</th><td>" << mLoggingAuthor << "</td></tr>" << std::endl;
	if(!mLoggingCustomIcon.empty()) htmlLogFile << "<tr><th></th><td><a href=\"" << mLoggingCustomIcon << "\" target=\"_blank\">" << "<img src=\"" << mLoggingCustomIcon << "\" width=\"80\" alt=\"" << mLoggingCustomIcon <<"\"/></a></td></tr>" << std::endl;
	if(!mLoggingContact.empty()) htmlLogFile << "<tr><th>Contact:</th><td>" << mLoggingContact << "</td></tr>" << std::endl;
	if(!mLoggingComments.empty()) htmlLogFile << "<tr><th>Comments:</th><td>" << mLoggingComments << "</td></tr>" << std::endl;
	htmlLogFile << "</table>" << std::endl;

	htmlLogFile << "<p /><table id=\"yafalog\">" << std::endl;
	htmlLogFile << "<tr><th>Render Settings:</th><td>" << mRenderSettings << "</td></tr>" << std::endl;
	htmlLogFile << "<tr><th>AA and Noise Control Settings:</th><td>" << mAANoiseSettings << "</td></tr>" << std::endl;
	htmlLogFile << "</table>" << std::endl;

	if(!m_MemoryLog.empty()) 
	{
		htmlLogFile << "<p /><table id=\"yafalog\"><th>Date</th><th>Time</th><th>Verbosity</th><th>Description</th>" << std::endl;

		for (std::vector<logEntry_t>::iterator it = m_MemoryLog.begin() ; it != m_MemoryLog.end(); ++it)
		{
			htmlLogFile << "<tr><td>" << printDate(it->eventDateTime) << "</td><td>" << printTime(it->eventDateTime) << "</td>";

			switch(it->mVerbLevel)
			{
				case VL_DEBUG:		htmlLogFile << "<td BGCOLOR=#ff80ff>DEBUG: "; break;
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
}

void yafarayLog_t::clearAll()
{
	clearMemoryLog();
	mImagePath = "";
	mLoggingTitle = "";
	mLoggingAuthor = "";
	mLoggingContact = "";
	mLoggingComments = "";
	mLoggingCustomIcon = "";
	mAANoiseSettings = "";
	mRenderSettings = "";
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
			case VL_DEBUG:		std::cout << setColor(Magenta) << "[" << printTime(current_datetime) << "] DEBUG: " << setColor(); break;
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

void yafarayLog_t::appendAANoiseSettings(const std::string &aa_noise_settings)
{
	mAANoiseSettings += aa_noise_settings;
}

void yafarayLog_t::appendRenderSettings(const std::string &render_settings)
{
	mRenderSettings += render_settings;
}

void yafarayLog_t::splitPath(const std::string &fullFilePath, std::string &basePath, std::string &baseFileName, std::string &extension)
{
    std::string fullFileName;

    size_t sep = fullFilePath.find_last_of("\\/");
    if (sep != std::string::npos)
        fullFileName = fullFilePath.substr(sep + 1, fullFilePath.size() - sep - 1);

    basePath = fullFilePath.substr(0, sep+1);

    if(basePath == "") fullFileName = fullFilePath;

    size_t dot = fullFileName.find_last_of(".");

    if (dot != std::string::npos)
    {
        baseFileName = fullFileName.substr(0, dot);
        extension  = fullFileName.substr(dot, fullFileName.size() - dot);
    }
    else
    {
        baseFileName = fullFileName;
        extension  = "";
    }
    
    Y_DEBUG << "fullFilePath='"<<fullFilePath<<"', basePath='"<<basePath<<"', baseFileName='"<<baseFileName<<"', extension='"<<extension<<"'"<<yendl;
}               

void yafarayLog_t::setParamsBadgePosition(const std::string &badgePosition)
{ 
	if(badgePosition == "top")
	{
		mDrawParams = true;
		mParamsBadgeTop = true;
	}
	else if(badgePosition == "bottom")
	{
		mDrawParams = true;
		mParamsBadgeTop = false;
	}
	else
	{
		mDrawParams = false;
		mParamsBadgeTop = false;
	}
}

__END_YAFRAY

