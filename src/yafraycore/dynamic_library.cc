#include <core_api/dynamic_library.h>

#ifndef WIN32
#include <dirent.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <iostream>

using namespace std;

__BEGIN_YAFRAY

#ifdef WIN32

void dynamicLoadedLibrary_t::open(const std::string &lib)
{
	handle = LoadLibrary(lib.c_str());

	if(handle == nullptr)
	{
		DWORD err_code = GetLastError();
		const size_t err_buf_size = 64000;
		char err_buf[err_buf_size] = {0};
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, err_code, 0, err_buf, err_buf_size, 0);

		cerr << "failed to open " << lib << ", LoadLibrary error: ";
		cerr << err_buf <<  " (" << err_code << ")" << endl;
	}
	else
		refcount = new int(1);
}

void dynamicLoadedLibrary_t::close()
{
	if(handle != nullptr)
	{
		FreeLibrary(handle);
		handle = nullptr;
		delete refcount;
	}
}

void *dynamicLoadedLibrary_t::getSymbol(const char *name)
{
	if(handle != nullptr)
	{
		void *func = (void *)GetProcAddress(handle, name); //added explicit cast to enable mingw32 compilation (DarkTide)
		if(func == nullptr)
			cerr << "GetProcAddress error: " << GetLastError() << endl;
		return func;
	}
	else
	{
		return nullptr;
	}
}

#else

void dynamicLoadedLibrary_t::open(const std::string &lib)
{
	handle = dlopen(lib.c_str(), RTLD_NOW);
	if(handle == nullptr)
		cerr << "dlerror: " << dlerror() << endl;
	else
		refcount = new int(1);
}

void dynamicLoadedLibrary_t::close()
{
	if(handle != nullptr)
	{
		dlclose(handle);
		handle = nullptr;
		delete refcount;
	}
}

void *dynamicLoadedLibrary_t::getSymbol(const char *name)
{
	if(handle != nullptr)
	{
		void *func = dlsym(handle, name);
		if(func == nullptr)
			cerr << "dlerror: " << dlerror() << endl;
		return func;
	}
	else
		return nullptr;
}

#endif

bool dynamicLoadedLibrary_t::isOpen()
{
	return handle != nullptr;
}

dynamicLoadedLibrary_t::dynamicLoadedLibrary_t()
{
	handle = nullptr;
}

dynamicLoadedLibrary_t::dynamicLoadedLibrary_t(const std::string &library)
	: handle(nullptr)
{
	open(library);
}

dynamicLoadedLibrary_t::dynamicLoadedLibrary_t(const dynamicLoadedLibrary_t &src)
{
	handle = src.handle;
	if(isOpen())
	{
		refcount = src.refcount;
		addReference();
	}
}

dynamicLoadedLibrary_t::~dynamicLoadedLibrary_t()
{
	if(isOpen())
	{
		removeReference();
		if(!isUsed())
			close();
	}
}

__END_YAFRAY

