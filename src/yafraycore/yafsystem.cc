
#include <core_api/yafsystem.h>

#ifdef WIN32

//#include <io.h>
//#include <windows.h>

#else

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

void sharedlibrary_t::open(const std::string &lib) 
{
	handle = LoadLibrary(lib.c_str());

	if (handle == NULL) {
		DWORD err_code = GetLastError();
		const size_t err_buf_size = 64000;
		char err_buf[err_buf_size] = {0};
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, err_code, 0, err_buf, err_buf_size, 0);

		cerr << "failed to open " << lib << ", LoadLibrary error: ";
		cerr << err_buf <<  " (" << err_code << ")" << endl;
	} else
		refcount=new int(1);
}

void sharedlibrary_t::close() 
{
	if (handle != NULL) 
	{
		FreeLibrary(handle);
		handle = NULL;
		delete refcount;
	}
}

void * sharedlibrary_t::getSymbol(const char *name) 
{
	if (handle != NULL) 
	{
		void *func = (void*)GetProcAddress(handle, name); //added explicit cast to enable mingw32 compilation (DarkTide)
		if (func == NULL)
			cerr << "GetProcAddress error: " << GetLastError() << endl;
		return func;
	} 
	else 
	{
		return NULL;
	}
}

#else

void sharedlibrary_t::open(const std::string &lib) 
{
	handle = dlopen(lib.c_str(),RTLD_NOW);
	if (handle == NULL) 
		cerr << "dlerror: " << dlerror() << endl;
	else
		refcount=new int(1);
}

void sharedlibrary_t::close() 
{
	if (handle != NULL) 
	{
		dlclose(handle);
		handle = NULL;
		delete refcount;
	}
}

void * sharedlibrary_t::getSymbol(const char *name) 
{
	if (handle != NULL) 
	{
		void *func = dlsym(handle, name);
		if (func == NULL) 
			cerr<<"dlerror: "<<dlerror()<<endl;
		return func;
	} 
	else 
		return NULL;
}

#endif

bool sharedlibrary_t::isOpen() 
{
	return handle != NULL;
}

sharedlibrary_t::sharedlibrary_t() 
{
  handle = NULL;
}

sharedlibrary_t::sharedlibrary_t(const std::string &library) 
  : handle(NULL)
{
  open(library);
}

sharedlibrary_t::sharedlibrary_t(const sharedlibrary_t &src) 
{
  handle = src.handle;
  if (isOpen())
	{
		refcount=src.refcount;
    addReference();
	}
}

sharedlibrary_t::~sharedlibrary_t() 
{
	if(isOpen())
	{
		removeReference();
		if(!isUsed())
			close();
  }
}


const std::list<std::string> & listDir(const std::string &dir)
{
	static std::list<std::string> lista;
	lista.clear();

#if defined(WIN32)
  std::string pattern = dir + "/*.dll";

  // replace all the "/" with "\"
  for (int i = 0; i < (int)pattern.length(); ++i) 
    if (pattern[i] == '/') pattern[i] = '\\';

  _finddata_t    FindData;
  int            hFind, Result;

  hFind = _findfirst(pattern.c_str(), &FindData);
  if (hFind != -1)
    Result = 0;
  else
    Result = 1;

	while (Result == 0)
	{
		lista.push_back(dir+"/"+FindData.name);
		Result = _findnext(hFind, &FindData);
	}

  _findclose(hFind);

#else

	DIR *directorio;
	struct dirent *entrada;
	struct stat estado;

	directorio=opendir(dir.c_str());
	if(directorio==NULL) return lista;

	entrada=readdir(directorio);
	while(entrada!=NULL) 
	{
		string full=dir+"/"+entrada->d_name;
		stat(full.c_str(),&estado);
		if(S_ISREG(estado.st_mode))
			lista.push_back(full);
    entrada=readdir(directorio);
  }
  closedir(directorio);
#endif
  return lista;
}

__END_YAFRAY

