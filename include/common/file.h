#pragma once
/****************************************************************************
 *
 *      file.h: File handling classes with unicode paths
 *      This is part of the libYafaRay package
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

#ifndef YAFARAY_FILE_H
#define YAFARAY_FILE_H

#include <string>
#include <vector>

namespace yafaray {

class Path final
{
	public:
		explicit Path(const std::string &full_path);
		Path(std::string directory, std::string base_name, std::string extension);
		std::string getDirectory() const { return directory_; }
		std::string getBaseName() const { return base_name_; }
		std::string getExtension() const { return extension_; }
		std::string getParentDirectory() const;
		std::string getFullPath() const;
		void setDirectory(const std::string &dir) { directory_ = dir; }
		void setBaseName(const std::string &name) { base_name_ = name; }
		void setExtension(const std::string &ext) { extension_ = ext; }
		static std::string getParent(const std::string &path);

	private:
		std::string directory_;
		std::string base_name_;
		std::string extension_;
};


class File final
{
	public:
		explicit File(const std::string &path);
		explicit File(Path path);
		~File();
		bool save(const std::string &str, bool with_tmp);
		static std::FILE *open(const std::string &path, const std::string &access_mode);
		static std::FILE *open(const Path &path, const std::string &access_mode);
		static int close(std::FILE *fp);
		static bool exists(const std::string &path, bool files_only);
		static bool remove(const std::string &path, bool files_only);
		static bool rename(const std::string &path_old, const std::string &path_new, bool overwrite, bool files_only);
		static std::vector<std::string> listFiles(const std::string &directory);
		bool open(const std::string &access_mode);
		int close();
		bool read(std::string &str) const;
		template <typename T> bool read(T &value) const;
		bool append(const std::string &str);
		template <typename T> bool append(const T &value);

	private:
		bool save(const char *buffer, size_t size, bool with_temp);
		bool read(char *buffer, size_t size) const;
		bool append(const char *buffer, size_t size);
		Path path_;
		std::FILE *fp_ = nullptr;
};

template <typename T> bool File::read(T &value) const
{
	static_assert(std::is_standard_layout<T>::value && std::is_trivial<T>::value, "T must be a plain old data (POD) type like char, int32_t, float, etc");
	return File::read((char *)&value, sizeof(T));
}

template <typename T> bool File::append(const T &value)
{
	static_assert(std::is_standard_layout<T>::value && std::is_trivial<T>::value, "T must be a plain old data (POD) type like char, int32_t, float, etc");
	return File::append((const char *)&value, sizeof(T));
}


} //namespace yafaray

#endif //YAFARAY_FILE_H
