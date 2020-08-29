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

BEGIN_YAFRAY

#ifdef WIN32

void DynamicLoadedLibrary::open(const std::string &lib)
{
	handle_ = LoadLibrary(lib.c_str());

	if(handle_ == nullptr)
	{
		DWORD err_code = GetLastError();
		const size_t err_buf_size = 64000;
		char err_buf[err_buf_size] = {0};
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, err_code, 0, err_buf, err_buf_size, 0);

		cerr << "failed to open " << lib << ", LoadLibrary error: ";
		cerr << err_buf <<  " (" << err_code << ")" << endl;
	}
	else
		refcount_ = new int(1);
}

void DynamicLoadedLibrary::close()
{
	if(handle_ != nullptr)
	{
		FreeLibrary(handle_);
		handle_ = nullptr;
		delete refcount_;
	}
}

void *DynamicLoadedLibrary::getSymbol(const char *name)
{
	if(handle_ != nullptr)
	{
		void *func = (void *)GetProcAddress(handle_, name); //added explicit cast to enable mingw32 compilation (DarkTide)
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

void DynamicLoadedLibrary::open(const std::string &lib)
{
	handle_ = dlopen(lib.c_str(), RTLD_NOW);
	if(handle_ == nullptr)
		cerr << "dlerror: " << dlerror() << endl;
	else
		refcount_ = new int(1);
}

void DynamicLoadedLibrary::close()
{
	if(handle_ != nullptr)
	{
		dlclose(handle_);
		handle_ = nullptr;
		delete refcount_;
	}
}

void *DynamicLoadedLibrary::getSymbol(const char *name)
{
	if(handle_ != nullptr)
	{
		void *func = dlsym(handle_, name);
		if(func == nullptr)
			cerr << "dlerror: " << dlerror() << endl;
		return func;
	}
	else
		return nullptr;
}

#endif

bool DynamicLoadedLibrary::isOpen()
{
	return handle_ != nullptr;
}

DynamicLoadedLibrary::DynamicLoadedLibrary()
{
	handle_ = nullptr;
}

DynamicLoadedLibrary::DynamicLoadedLibrary(const std::string &library)
	: handle_(nullptr)
{
	open(library);
}

DynamicLoadedLibrary::DynamicLoadedLibrary(const DynamicLoadedLibrary &src)
{
	handle_ = src.handle_;
	if(isOpen())
	{
		refcount_ = src.refcount_;
		addReference();
	}
}

DynamicLoadedLibrary::~DynamicLoadedLibrary()
{
	if(isOpen())
	{
		removeReference();
		if(!isUsed())
			close();
	}
}

END_YAFRAY

