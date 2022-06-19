/****************************************************************************
 *
 *      file.cc: File handling classes with unicode paths
 *      This is part of the libYafaRay package
 *      Copyright (C) 2020  David Bluecame
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

#include "common/file.h"
#include "common/logger.h"
#include <sys/stat.h>
#if defined(_WIN32)
#include "common/string.h"
#include <sstream>
#include <windows.h>
#else //defined(_WIN32)
#include <dirent.h>
#endif //defined(_WIN32)
#include <iostream>
#include <ctime>
#include <utility>

namespace yafaray {

Path::Path(std::string directory, std::string base_name, std::string extension) : directory_(std::move(directory)), base_name_(std::move(base_name)), extension_(std::move(extension))
{
	//// if(logger_.isDebug())logger_.logDebug("Directory: " << directory);
	//// if(logger_.isDebug())logger_.logDebug("Base name: " << baseName);
	//// if(logger_.isDebug())logger_.logDebug("Extension: " << extension);
}

Path::Path(const std::string &full_path)
{
	std::string full_name;
	const size_t sep = full_path.find_last_of("\\/");
	if(sep != std::string::npos)
	{
		full_name = full_path.substr((sep + 1), full_path.size() - (sep + 1));
		directory_ = full_path.substr(0, sep);
	}
	else directory_ = full_path.substr(0, sep + 1);

	if(directory_.empty()) full_name = full_path;

	const size_t dot = full_name.find_last_of('.');

	if(dot != std::string::npos)
	{
		base_name_ = full_name.substr(0, dot);
		extension_ = full_name.substr(dot + 1, full_name.size() - (dot + 1));
	}
	else
	{
		base_name_ = full_name;
		extension_  = "";
	}
	//// if(logger_.isDebug())logger_.logDebug("Directory: " << directory);
	//// if(logger_.isDebug())logger_.logDebug("Base name: " << baseName);
	//// if(logger_.isDebug())logger_.logDebug("Extension: " << extension);
}

std::string Path::getParent(const std::string &path)
{
	std::string parent;
	const size_t sep = path.find_last_of("\\/");
	if(sep != std::string::npos) parent = path.substr(0, sep);
	else parent = path.substr(0, sep + 1);
	return parent;
}
std::string Path::getParentDirectory() const
{
	return getParent(directory_);
}

std::string Path::getFullPath() const
{
	////// if(logger_.isDebug()) Y_DEBUG PR(directory) PR(baseName) PR(extension");
	std::string full_path;
	if(!directory_.empty()) full_path += directory_ + "/";
	full_path += base_name_;
	if(!extension_.empty()) full_path += "." + extension_;
	return full_path;
}


File::File(const std::string &path) : path_(path)
{
}

File::File(Path path) : path_(std::move(path))
{
}

File::~File()
{
	// if(logger_.isDebug()) logger.logDebug("File::~File closing) PR(fp_) PR(path_.getFullPath()");
	File::close();
}

bool File::open(const std::string &access_mode)
{
	if(fp_) return false; //file already open
	fp_ = File::open(path_, access_mode);
	if(fp_) return true;
	else return false;
}

bool File::save(const std::string &str, bool with_temp)
{
	return File::save(str.data(), str.size(), with_temp);
}

bool File::save(const char *buffer, size_t size, bool with_temp)
{
	File::close();
	bool result = true;
	if(with_temp)
	{
		const std::string path_full = path_.getFullPath();
		const std::string path_tmp = path_full + ".tmp";
		File tmp = File(path_tmp);
		result &= tmp.save(buffer, size, /*with_temp=*/false);
		if(result) result &= File::rename(path_tmp, path_full, /*overwrite=*/true, /*files_only=*/true);
	}
	else
	{
		result &= File::open("wb");
		result &= File::append(buffer, size);
		File::close();
	}
	return result;
}


bool File::remove(const std::string &path, bool files_only)
{
	if(files_only && !File::exists(path, files_only)) return false;
#if defined(_WIN32)
	return ::_wremove(string::utf8ToWutf16Le(path).c_str()) == 0;
#else //defined(_WIN32)
	return ::remove(path.c_str()) == 0;
#endif //defined(_WIN32)
}

bool File::rename(const std::string &path_old, const std::string &path_new, bool overwrite, bool files_only)
{
	if(files_only && !File::exists(path_old, files_only)) return false;
	if(overwrite) File::remove(path_new, files_only);
#if defined(_WIN32)
	return ::_wrename(string::utf8ToWutf16Le(path_old).c_str(), string::utf8ToWutf16Le(path_new).c_str()) == 0;
#else //defined(_WIN32)
	return ::rename(path_old.c_str(), path_new.c_str()) == 0;
#endif //defined(_WIN32)
}

bool File::read(std::string &str) const
{
	str.clear();
	char ch;
	do
	{
		read(ch);
		if(ch == 0x00) break;
		else str += ch;
	}
	while(true);
	return !str.empty();
}

bool File::read(char *buffer, size_t size) const
{
	if(!fp_) return false;
	::fread(buffer, 1, size, fp_);
	return true;
}

bool File::append(const std::string &str)
{
	const char zero = 0x00;
	return File::append(str.data(), str.size()) && File::append(&zero, sizeof(char));
}

bool File::append(const char *buffer, size_t size)
{
	if(!fp_) return false;
	std::fwrite(buffer, 1, size, fp_);
	return true;
}

int File::close()
{
	// if(logger_.isDebug()) logger.logDebug("File::close) PR(fp_) PR(path_.getFullPath()");
	if(!fp_) return false;
	int result = std::fclose(fp_);
	fp_ = nullptr;
	return result;
}

std::FILE *File::open(const std::string &path, const std::string &access_mode)
{
	std::FILE *fp = nullptr;
#if defined(_WIN32)
	const std::wstring w_path = string::utf8ToWutf16Le(path);
	std::wstringstream w_access_mode;
	w_access_mode << access_mode.c_str();
	//fp = ::_wfopen(wPath.c_str(), wAccessMode.str().c_str());	//Windows needs the path in UTF16LE (unicode UTF-16, little endian) so we have to convert the UTF8 path to UTF16
	/* ::errno_t err = */ ::_wfopen_s(&fp, w_path.c_str(), w_access_mode.str().c_str());	//Windows needs the path in UTF16LE (unicode UTF-16, little endian) so we have to convert the UTF8 path to UTF16
#else //_WIN32
	fp = std::fopen(path.c_str(), access_mode.c_str());
#endif //_WIN32
	// if(logger_.isDebug()) logger.logDebug("File::open) PR(path) PR(access_mode) PR(fp");
	return fp;
}

std::FILE *File::open(const Path &path, const std::string &access_mode)
{
	return File::open(path.getFullPath(), access_mode);
}

int File::close(std::FILE *fp)
{
	// if(logger_.isDebug()) logger.logDebug("File::close execution) PR(fp");
	return std::fclose(fp);
}

bool File::exists(const std::string &path, bool files_only)
{
#if defined(_WIN32)
	const std::wstring wPath = string::utf8ToWutf16Le(path);
	struct _stat buf;
	errno = 0;
	/*const int result = */::_wstat(wPath.c_str(), &buf);
	const int errsav = errno;
	char timebuf[26];
	/* const errno_t err = */ ::ctime_s(timebuf, 26, &buf.st_mtime);
	//// if(logger_.isDebug())logger_.logDebug( "Time modified : " << timebuf);
#else //_WIN32
	struct ::stat buf;
	errno = 0;
	/*const int result = */::lstat(path.c_str(), &buf);   //use regular stat or else use lstat so it does not follow symlinks?
	const int errsav = errno;
#endif //_WIN32
	//// if(logger_.isDebug())logger_.logDebug("Result stat: " << result);
	//// if(logger_.isDebug())logger_.logDebug("File size     : " << buf.st_size);
	//// if(logger_.isDebug())logger_.logDebug("Drive         : " << buf.st_dev + 'A');
	//// if(logger_.isDebug())logger_.logDebug("Type:         : " << buf.st_mode);
	//// if(logger_.isDebug())logger_.logDebug("S_ISREG(buf.st_mode): " << ((buf.st_mode & S_IFMT) == S_IFREG));
	//// if(logger_.isDebug())logger_.logDebug("errsav: " << errsav << ", ENOENT=" << ENOENT);
	//// if(logger_.isDebug())logger_.logDebug("path: " << path);
	if(files_only) return errsav != ENOENT && ((buf.st_mode & S_IFMT) == S_IFREG);
	else return errno != ENOENT;
}

std::vector<std::string> File::listFiles(const std::string &directory)
{
	std::vector<std::string> files;
#if defined(_WIN32)
	::WIN32_FIND_DATAW findData;
	::HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = ::FindFirstFileW(string::utf8ToWutf16Le(directory + "/*").c_str(), &findData);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(findData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE))
			{
				files.emplace_back(string::wutf16LeToUtf8(std::wstring(findData.cFileName)));
			}
		}
		while(::FindNextFileW(hFind, &findData) != 0);
	}
	::FindClose(hFind);
	return files;

#else //_WIN32
	errno = 0;
	::DIR *dirp = opendir(directory.c_str());
	struct ::dirent *dir = nullptr;
	if(errno == 0)
	{
		while((dir = readdir(dirp)))
		{
			if(dir->d_type == DT_REG) files.emplace_back(dir->d_name);
		}
		closedir(dirp);
	}
#endif //_WIN32
	return files;
}

} //namespace yafaray
