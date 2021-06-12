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
#include "yafaray_build_info.h"

BEGIN_YAFARAY

std::string buildinfo::getVersion() { return YAFARAY_VERSION; }
int buildinfo::getVersionMajor() { return YAFARAY_VERSION_MAJOR; }
int buildinfo::getVersionMinor() { return YAFARAY_VERSION_MINOR; }
int buildinfo::getVersionPatch() { return YAFARAY_VERSION_PATCH; }
std::string buildinfo::getVersionState() { return YAFARAY_VERSION_STATE; }
std::string buildinfo::getVersionStateDescription() { return YAFARAY_VERSION_STATE_DESCRIPTION; }
std::string buildinfo::getGit() { return YAFARAY_VERSION_GIT; }
std::string buildinfo::getGitTag() { return YAFARAY_VERSION_GIT_TAG; }
std::string buildinfo::getGitBranch() { return YAFARAY_VERSION_GIT_BRANCH; }
std::string buildinfo::getGitDirty() { return YAFARAY_VERSION_GIT_DIRTY; }
std::string buildinfo::getGitCommit() { return YAFARAY_VERSION_GIT_COMMIT; }
std::string buildinfo::getGitCommitDateTime() { return YAFARAY_VERSION_GIT_COMMIT_DATETIME; }
std::string buildinfo::getCommitsSinceTag() { return YAFARAY_VERSION_GIT_COMMITS_SINCE_TAG; }
std::string buildinfo::getBuildArchitectureBits() { return YAFARAY_BUILD_ARCHITECTURE_BITS; }
std::string buildinfo::getBuildCompiler() { return YAFARAY_BUILD_COMPILER; }
std::string buildinfo::getBuildOs() { return YAFARAY_BUILD_OS; }
std::string buildinfo::getBuildPlatform() { return YAFARAY_BUILD_PLATFORM; }
std::string buildinfo::getBuildType() { return YAFARAY_BUILD_TYPE; }
std::string buildinfo::getBuildDebug() { return YAFARAY_BUILD_TYPE_DEBUG; }
std::string buildinfo::getBuildOptions() { return YAFARAY_BUILD_OPTIONS_COMPILE_DEFINITIONS; }

END_YAFARAY
