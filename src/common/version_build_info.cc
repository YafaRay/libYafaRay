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

#include "common/version_build_info.h"
#include "yafaray_version_build_info.h"

namespace yafaray {

std::string buildinfo::getVersionString() { return YAFARAY_VERSION_STRING + getGitLine(false); }
std::string buildinfo::getVersionDescription() { return YAFARAY_VERSION_DESCRIPTION; }
int buildinfo::getVersionMajor() { return YAFARAY_VERSION_MAJOR; }
int buildinfo::getVersionMinor() { return YAFARAY_VERSION_MINOR; }
int buildinfo::getVersionPatch() { return YAFARAY_VERSION_PATCH; }
std::string buildinfo::getVersionPreRelease() { return YAFARAY_VERSION_PRE_RELEASE; }
std::string buildinfo::getVersionPreReleaseDescription() { return YAFARAY_VERSION_PRE_RELEASE_DESCRIPTION; }
std::string buildinfo::getGitDescribe() { return YAFARAY_VERSION_GIT; }
std::string buildinfo::getGitTag() { return YAFARAY_VERSION_GIT_TAG; }
std::string buildinfo::getGitBranch() { return YAFARAY_VERSION_GIT_BRANCH; }
std::string buildinfo::getGitDirty() { return YAFARAY_VERSION_GIT_DIRTY; }
std::string buildinfo::getGitCommit() { return YAFARAY_VERSION_GIT_COMMIT; }
std::string buildinfo::getGitCommitDateTime() { return YAFARAY_VERSION_GIT_COMMIT_DATETIME; }
std::string buildinfo::getCommitsSinceTag() { return YAFARAY_VERSION_GIT_COMMITS_SINCE_TAG; }
std::string buildinfo::getBuildArchitectureBits() { return YAFARAY_BUILD_ARCHITECTURE_BITS; }
std::string buildinfo::getBuildCompiler() { return YAFARAY_BUILD_COMPILER; }
std::string buildinfo::getBuildCompilerVersion() { return YAFARAY_BUILD_COMPILER_VERSION; }
std::string buildinfo::getBuildOs() { return YAFARAY_BUILD_OS; }
std::string buildinfo::getBuildType() { return YAFARAY_BUILD_TYPE; }
std::string buildinfo::getBuildOptions() { return YAFARAY_BUILD_OPTIONS; }
std::string buildinfo::getBuildFlags() { return YAFARAY_BUILD_FLAGS; }
std::string buildinfo::getBuildTypeSuffix()
{
	const std::string build_type = getBuildType();
	if(build_type.empty() || build_type == "RELEASE") return "";
	else return "/" + build_type;
}

std::string buildinfo::getGitLine(bool long_line)
{
	// If no Git data or it's just master branch without any changes since last tag, then return an empty string
	std::string branch = getGitBranch();
	if(branch == "master") branch.clear();
	std::string commits_since_tag = getCommitsSinceTag();
	if(commits_since_tag == "0") commits_since_tag.clear();
	const std::string dirty = getGitDirty();

	if(branch.empty() && dirty.empty() && commits_since_tag.empty()) return "";
	std::string result = "+git";
	if(long_line)
	{
		if(!branch.empty()) result += "." + branch;
		result += "." + getGitTag();
		if(!commits_since_tag.empty()) result += ".+" + commits_since_tag;
	}
	result += ".g" + getGitCommit().substr(0, 8);
	if(!dirty.empty()) result += ".dirty";
	return result;
}

std::vector<std::string> buildinfo::getAllBuildDetails()
{
	std::vector<std::string> result;
	result.emplace_back("Version = '" + getVersionString() + "'");
	result.emplace_back("VersionDescription = '" + getVersionDescription() + "'");
	result.emplace_back("VersionState = '" + getVersionPreRelease() + "'");
	result.emplace_back("VersionStateDescription = '" + getVersionPreReleaseDescription() + "'");
	result.emplace_back("GitDescribe = '" + getGitDescribe() + "'");
	result.emplace_back("GitShortLine = '" + getGitLine(false) + "'");
	result.emplace_back("GitLongLine = '" + getGitLine(true) + "'");
	result.emplace_back("GitTag = '" + getGitTag() + "'");
	result.emplace_back("GitBranch = '" + getGitBranch() + "'");
	result.emplace_back("GitDirty = '" + getGitDirty() + "'");
	result.emplace_back("GitCommit = '" + getGitCommit() + "'");
	result.emplace_back("GitCommitDateTime = '" + getGitCommitDateTime() + "'");
	result.emplace_back("CommitsSinceTag = '" + getCommitsSinceTag() + "'");
	result.emplace_back("BuildArchitectureBits = '" + getBuildArchitectureBits() + "'");
	result.emplace_back("BuildCompiler = '" + getBuildCompiler() + "'");
	result.emplace_back("BuildCompilerVersion = '" + getBuildCompilerVersion() + "'");
	result.emplace_back("BuildOs = '" + getBuildOs() + "'");
	result.emplace_back("BuildType = '" + getBuildType() + "'");
	result.emplace_back("BuildOptions = '" + getBuildOptions() + "'");
	result.emplace_back("BuildCompilerFlags = '" + getBuildFlags() + "'");
	return result;
}

} //namespace yafaray
