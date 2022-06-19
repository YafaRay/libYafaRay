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

#ifndef YAFARAY_VERSION_BUILD_INFO_H
#define YAFARAY_VERSION_BUILD_INFO_H

#include <string>
#include <vector>

namespace yafaray::buildinfo
{
	std::string getVersionString();
	std::string getVersionDescription();
	int getVersionMajor();
	int getVersionMinor();
	int getVersionPatch();
	std::string getVersionPreRelease();
	std::string getVersionPreReleaseDescription();
	std::string getGitDescribe();
	std::string getGitLine(bool long_line);
	std::string getGitTag();
	std::string getGitBranch();
	std::string getGitDirty();
	std::string getGitCommit();
	std::string getGitCommitDateTime();
	std::string getCommitsSinceTag();
	std::string getBuildArchitectureBits();
	std::string getBuildCompiler();
	std::string getBuildCompilerVersion();
	std::string getBuildOs();
	std::string getBuildType();
	std::string getBuildTypeSuffix();
	std::string getBuildOptions();
	std::string getBuildFlags();
	std::vector<std::string> getAllBuildDetails();

} //namespace yafaray::buildinfo

#endif //LIBYAFARAY_VERSION_BUILD_INFO_H
