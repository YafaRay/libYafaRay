#ifndef __YAFSYSTEM_H
#define __YAFSYSTEM_H

#include <yafray_constants.h>

#ifdef WIN32
	#include <io.h>
	#include <windows.h>
#endif

#include <list>
#include <string>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT dynamicLoadedLibrary_t
{
	public:
	  dynamicLoadedLibrary_t();
	  dynamicLoadedLibrary_t(const std::string &library);
	  dynamicLoadedLibrary_t(const dynamicLoadedLibrary_t &src);
	  ~dynamicLoadedLibrary_t();

	  bool isOpen();
	  void* getSymbol(const char *name);

	protected:

	  void open(const std::string &library);
	  void close();
  	void addReference() { (*refcount)++; };
  	void removeReference() { (*refcount)--; };
	bool isUsed()const {return ((*refcount)>0);}; 


	int *refcount;
#ifdef WIN32
	HINSTANCE handle;
#else
	void *handle;
#endif
};

__END_YAFRAY

#endif
