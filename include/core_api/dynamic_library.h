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
