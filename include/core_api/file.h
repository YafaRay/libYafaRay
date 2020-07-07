/****************************************************************************
 *
 *      file.h: File handling classes with unicode paths
 *      This is part of the yafray package
 *      Copyright (C) 2016-2020  David Bluecame
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

#ifndef Y_FILE_H
#define Y_FILE_H

#include <yafray_constants.h>
#include <string>
#include <vector>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT path_t
{
public:
	path_t(const std::string &fullPath);
	path_t(const std::string &directory, const std::string &baseName, const std::string &extension);
	std::string getDirectory() const { return directory; }
	std::string getBaseName() const { return baseName; }
	std::string getExtension() const { return extension; }
	std::string getParentDirectory() const;
	std::string getFullPath() const;
	void setDirectory(const std::string &dir) { directory = dir; }
	void setBaseName(const std::string &name) { baseName = name; }
	void setExtension(const std::string &ext) { extension = ext; }
	static std::string getParent(const std::string &path);
protected:
	std::string directory;
	std::string baseName;
	std::string extension;
};


class YAFRAYCORE_EXPORT file_t
{
public:
	file_t(const std::string &path);
	file_t(const path_t &path);
	~file_t();
	bool save(const std::string &str, bool with_tmp);
	static FILE * open(const std::string &path, const std::string &accessMode);
	static FILE * open(const path_t &path, const std::string &accessMode);
	static int close(FILE * fp);
	static bool exists(const std::string &path, bool files_only);
	static bool remove(const std::string &path, bool files_only);
	static bool rename(const std::string &pathOld, const std::string &pathNew, bool overwrite, bool files_only);
	static std::vector<std::string> listFiles(const std::string &directory);
	bool open(const std::string &accessMode);
	int close();
	bool read(std::string &str) const;
	bool read(char &c) const;
	bool read(int &i) const;
	bool read(unsigned int &u) const;
	bool read(size_t &s) const;
	bool read(float &f) const;
	bool append(const std::string &str);
	bool append(char c);
	bool append(int i);
	bool append(unsigned int u);
	bool append(size_t s);
	bool append(float f);
protected:
	bool save(const char *buffer, size_t size, bool with_temp);
	bool read(char *buffer, size_t size) const;
	bool append(const char *buffer, size_t size);
	path_t path;
	FILE *fp = nullptr;
};

__END_YAFRAY

#endif //Y_FILE_H
