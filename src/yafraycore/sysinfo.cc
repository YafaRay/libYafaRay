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

std::string sysInfoGetArchitecture()
{
#if BOOST_ARCH_X86_32
	return "32bit ";
#elif BOOST_ARCH_X86_64
	return "64bit ";
#else
	return "";
#endif
}

std::string sysInfoGetCompiler()
{
#if BOOST_COMP_CLANG
	return "Clang";
#elif BOOST_COMP_GNUC
	return "GCC";
#elif BOOST_COMP_INTEL
	return "ICC";
#elif BOOST_COMP_LLVM
	return "LLVM";
#elif BOOST_COMP_MSVC
	std::stringstream compiler;
	compiler << "VS";
	if(_MSC_VER == 1800) compiler << "2013";
	else if(_MSC_VER == 1900) compiler << "2015";
	else if(_MSC_VER == 1910) compiler << "2017";
	return compiler.str();
#else
	return "";
#endif
}

std::string sysInfoGetOS()
{
#if BOOST_OS_MACOS
	return "MacOSX ";
#elif BOOST_OS_LINUX
	return "Linux ";
#elif BOOST_OS_WINDOWS
	return "Windows ";
#else
	return "";
#endif
}

std::string sysInfoGetPlatform()
{
#if BOOST_PLAT_MINGW
	return "MinGW ";
#else
	return "";
#endif
}

//std::string sysInfoGetHW();
//std::string sysInfoGetRuntimeInformation();

__END_YAFRAY

