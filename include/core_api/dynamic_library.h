#pragma once
/****************************************************************************
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
 */

#ifndef YAFARAY_DYNAMIC_LIBRARY_H
#define YAFARAY_DYNAMIC_LIBRARY_H

#include <yafray_constants.h>

#ifdef WIN32
#include <io.h>
#include <windows.h>
#endif

#include <list>
#include <string>

BEGIN_YAFRAY

class YAFRAYCORE_EXPORT DynamicLoadedLibrary final
{
	public:
		DynamicLoadedLibrary();
		DynamicLoadedLibrary(const std::string &library);
		DynamicLoadedLibrary(const DynamicLoadedLibrary &src);
		~DynamicLoadedLibrary();

		bool isOpen();
		void *getSymbol(const char *name);

	private:
		void open(const std::string &library);
		void close();
		void addReference() { (*refcount_)++; };
		void removeReference() { (*refcount_)--; };
		bool isUsed() const {return ((*refcount_) > 0);};

		int *refcount_;
#ifdef WIN32
		HINSTANCE handle_;
#else
		void *handle_;
#endif
};

END_YAFRAY

#endif
