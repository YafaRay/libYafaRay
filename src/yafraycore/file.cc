/****************************************************************************
 *
 *      file.cc: File handling classes with unicode paths
 *      This is part of the yafray package
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
 
#include <core_api/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <utilities/stringUtils.h>
#include <sstream>
#include <windows.h>
#else //defined(_WIN32)
#include <dirent.h>
#endif //defined(_WIN32)
#include <iostream> 
#include <ctime>

__BEGIN_YAFRAY

path_t::path_t(const std::string &directory, const std::string &baseName, const std::string &extension) : directory(directory), baseName(baseName), extension(extension)
{
	//Y_DEBUG << "Directory: " << directory << yendl;
	//Y_DEBUG << "Base name: " << baseName << yendl;
	//Y_DEBUG << "Extension: " << extension << yendl;
}

path_t::path_t(const std::string &fullPath)
{
	std::string fullName;
	const size_t sep = fullPath.find_last_of("\\/");
	if (sep != std::string::npos)
	{
		fullName = fullPath.substr((sep+1), fullPath.size() - (sep+1));
		directory = fullPath.substr(0, sep);
	}
	else directory = fullPath.substr(0, sep+1);

	if(directory.empty()) fullName = fullPath;

	const size_t dot = fullName.find_last_of(".");

	if (dot != std::string::npos)
	{
		baseName = fullName.substr(0, dot);
		extension = fullName.substr(dot+1, fullName.size() - (dot+1));
	}
	else
	{
		baseName = fullName;
		extension  = "";
	}
	//Y_DEBUG << "Directory: " << directory << yendl;
	//Y_DEBUG << "Base name: " << baseName << yendl;
	//Y_DEBUG << "Extension: " << extension << yendl;
}

std::string path_t::getParent(const std::string &path)
{
	std::string parent;
	const size_t sep = path.find_last_of("\\/");
	if (sep != std::string::npos) parent = path.substr(0, sep);
	else parent = path.substr(0, sep+1);
	return parent;
}
std::string path_t::getParentDirectory() const
{
	return getParent(directory);
}

std::string path_t::getFullPath() const
{
	////Y_DEBUG PR(directory) PR(baseName) PR(extension) PREND;
	std::string fullPath;
	if(!directory.empty()) fullPath += directory + "/";
	fullPath += baseName;
	if(!extension.empty()) fullPath += "." + extension;
	return fullPath;
}



file_t::file_t(const std::string &path) : path(path)
{
}

file_t::file_t(const path_t &path) : path(path)
{
}

file_t::~file_t()
{
	file_t::close();
}

bool file_t::open(const std::string &accessMode)
{
	if(fp) return false; //file already open
	fp = file_t::open(path, accessMode);
	if(fp) return true;
	else return false;
}

bool file_t::save(const std::string &str, bool with_temp)
{
	return file_t::save(str.data(), str.size(), with_temp);
}

bool file_t::save(const char *buffer, size_t size, bool with_temp)
{
	file_t::close();
	bool result = true;
	if(with_temp)
	{
		const std::string pathFull = path.getFullPath();
		const std::string pathTmp =  pathFull + ".tmp";
		file_t tmp = file_t(pathTmp);
		result &= tmp.save(buffer, size, /*with_temp=*/false);
		if(result) result &= file_t::rename(pathTmp, pathFull, /*overwrite=*/true, /*files_only=*/true);
	}
	else
	{
		result &= file_t::open("wb");
		result &= file_t::append(buffer, size);
		file_t::close();
	}
	return result;
}


bool file_t::remove(const std::string &path, bool files_only)
{
	if(files_only && !file_t::exists(path, files_only)) return false;
#if defined(_WIN32)
	return ::_wremove(utf8_to_wutf16le(path).c_str()) == 0;
#else //defined(_WIN32)
	return ::remove(path.c_str()) == 0;
#endif //defined(_WIN32)
}

bool file_t::rename(const std::string &pathOld, const std::string &pathNew, bool overwrite, bool files_only)
{
	if(files_only && !file_t::exists(pathOld, files_only)) return false;
	if(overwrite) file_t::remove(pathNew, files_only);
#if defined(_WIN32)
	return ::_wrename(utf8_to_wutf16le(pathOld).c_str(), utf8_to_wutf16le(pathNew).c_str()) == 0;
#else //defined(_WIN32)
	return ::rename(pathOld.c_str(), pathNew.c_str()) == 0;
#endif //defined(_WIN32)
}

bool file_t::read(std::string &str) const
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

bool file_t::read(char *buffer, size_t size) const
{
	if(!fp) return false;
	::fread(buffer, 1, size, fp);
	return true;
}

bool file_t::append(const std::string &str)
{
	const char zero = 0x00;
	return file_t::append(str.data(), str.size()) && file_t::append(&zero, sizeof(char));
}

bool file_t::append(const char *buffer, size_t size)
{
	if(!fp) return false;
	::fwrite(buffer, 1, size, fp);
	return true;
}

int file_t::close()
{
	if(!fp) return false;
	int result = ::fclose(fp);
	fp = nullptr;
	return result;
}

FILE * file_t::open(const std::string &path, const std::string &accessMode)
{
	FILE *fp = nullptr;
#if defined(_WIN32)
	const std::wstring wPath = utf8_to_wutf16le(path);
	std::wstringstream wAccessMode;
	wAccessMode << accessMode.c_str();
	//fp = ::_wfopen(wPath.c_str(), wAccessMode.str().c_str());	//Windows needs the path in UTF16LE (unicode UTF-16, little endian) so we have to convert the UTF8 path to UTF16
	::errno_t err = ::_wfopen_s(&fp, wPath.c_str(), wAccessMode.str().c_str());	//Windows needs the path in UTF16LE (unicode UTF-16, little endian) so we have to convert the UTF8 path to UTF16
#else //_WIN32
	fp = ::fopen(path.c_str(), accessMode.c_str());
#endif //_WIN32
	return fp;
}

FILE * file_t::open(const path_t &path, const std::string &accessMode)
{
	return file_t::open(path.getFullPath(), accessMode);
}

int file_t::close(FILE * fp)
{
	return ::fclose(fp);
}

bool file_t::exists(const std::string &path, bool files_only)
{
#if defined(_WIN32)
	const std::wstring wPath = utf8_to_wutf16le(path);
	struct _stat buf;
	errno = 0;
	/*const int result = */::_wstat( wPath.c_str(), &buf );
	const int errsav = errno;
	char timebuf[26];
	const errno_t err = ::ctime_s(timebuf, 26, &buf.st_mtime);
	//Y_DEBUG <<  "Time modified : " << timebuf << yendl;
#else //_WIN32
	struct stat buf;
	errno = 0;
	/*const int result = */::lstat( path.c_str(), &buf ); //use regular stat or else use lstat so it does not follow symlinks?
	const int errsav = errno;
#endif //_WIN32
	//Y_DEBUG << "Result stat: " << result << yendl;
	//Y_DEBUG << "File size     : " << buf.st_size << yendl;
	//Y_DEBUG << "Drive         : " << buf.st_dev + 'A' << yendl;
	//Y_DEBUG << "Type:         : " << buf.st_mode << yendl;
	//Y_DEBUG << "S_ISREG(buf.st_mode): " << ((buf.st_mode & S_IFMT) == S_IFREG) << yendl;
	//Y_DEBUG << "errsav: " << errsav << ", ENOENT=" << ENOENT << yendl;
	//Y_DEBUG << "path: " << path << yendl;
	if(files_only) return errsav != ENOENT && ((buf.st_mode & S_IFMT) == S_IFREG);
	else return errno != ENOENT;
}

std::vector<std::string> file_t::listFiles(const std::string &directory)
{
	std::vector<std::string> files;
#if defined(_WIN32)
   ::WIN32_FIND_DATAW findData;
   ::HANDLE hFind = INVALID_HANDLE_VALUE;
   hFind = ::FindFirstFileW(utf8_to_wutf16le(directory + "/*").c_str(), &findData);
   if(hFind != INVALID_HANDLE_VALUE)
   {
	    do
		{
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
			{
			 files.push_back(wutf16le_to_utf8(std::wstring(findData.cFileName)));
			}
		}
		while (::FindNextFileW(hFind, &findData) != 0);
   }
   ::FindClose(hFind);
   return files;
   
#else //_WIN32
	errno = 0;
	::DIR *dirp = opendir(directory.c_str());
	struct dirent *dir = nullptr;
	if(errno == 0)
	{
		while((dir = readdir(dirp)))
		{
			if (dir->d_type == DT_REG) files.push_back(std::string(dir->d_name));
		}
		closedir(dirp);
	}
#endif //_WIN32
	return files;
}

__END_YAFRAY
