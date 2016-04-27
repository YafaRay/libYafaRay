/****************************************************************************
 *      sysinfo.h: YafaRay System Information
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
 
#ifndef Y_SYSINFO_H
#define Y_SYSINFO_H

#include <iostream>
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <boost/predef.h>

__BEGIN_YAFRAY

extern YAFRAYCORE_EXPORT std::string sysInfoGetArchitecture();
extern YAFRAYCORE_EXPORT std::string sysInfoGetCompiler();
extern YAFRAYCORE_EXPORT std::string sysInfoGetOS();
extern YAFRAYCORE_EXPORT std::string sysInfoGetPlatform();
extern YAFRAYCORE_EXPORT std::string sysInfoGetHW();
extern YAFRAYCORE_EXPORT std::string sysInfoGetRuntimeInformation();

__END_YAFRAY

#endif
