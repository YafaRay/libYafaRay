/****************************************************************************
 *      build_info.cc: YafaRay information about build
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
#include <core_api/build_info.h>
#include <yaf_build.h>

__BEGIN_YAFRAY

std::string buildInfoGetArchitecture()
{
    return YAFARAY_BUILD_ARCHITECTURE;
}

std::string buildInfoGetCompiler()
{
    return YAFARAY_BUILD_COMPILER;
}

std::string buildInfoGetOS()
{
    return YAFARAY_BUILD_OS;
}

std::string buildInfoGetPlatform()
{
    return YAFARAY_BUILD_PLATFORM;
}

__END_YAFRAY

