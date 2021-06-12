#pragma once
/****************************************************************************
 *
 *      This is part of the libYafaRay package
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
 *
 */

#ifndef LIBYAFARAY_VERSION_BUILD_INFO_H
#define LIBYAFARAY_VERSION_BUILD_INFO_H

#include "yafaray_conf.h"
#include <string>

BEGIN_YAFARAY

namespace buildinfo
{
	std::string getVersion();
	int getVersionMajor();
	int getVersionMinor();
	int getVersionPatch();
	std::string getVersionState();
	std::string getVersionStateDescription();
	std::string getGit();
	std::string getGitTag();
	std::string getGitBranch();
	std::string getGitDirty();
	std::string getGitCommit();
	std::string getGitCommitDateTime();
	std::string getCommitsSinceTag();
	std::string getBuildArchitectureBits();
	std::string getBuildCompiler();
	std::string getBuildOs();
	std::string getBuildPlatform();
	std::string getBuildType();
	std::string getBuildDebug();
	std::string getBuildOptions();
}

END_YAFARAY

#endif //LIBYAFARAY_VERSION_BUILD_INFO_H
