/****************************************************************************
 *      sysinfo.cc: YafaRay System Information
 *      This is part of the yafray package
 *		Copyright (C) 2016 David Bluecame
 * 		System Information, compilation information, etc
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

//std::string sysInfoGetArchitecture();
//std::string sysInfoGetCompiler();
std::string sysInfoGetOS()
{
#ifdef BOOST_OS_LINUX
	return "Linux";
#elif BOOST_OS_MACOS
	return "MacOSX";
#elif BOOST_OS_WINDOWS
	return "Windows";
#else
	return "Other OS";
#endif
}

//std::string sysInfoGetPlatform();
//std::string sysInfoGetHW();
//std::string sysInfoGetRuntimeInformation();

__END_YAFRAY

