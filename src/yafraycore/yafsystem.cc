
#include <core_api/yafsystem.h>

#ifdef WIN32

//#include <io.h>
//#include <windows.h>

#elif defined(__APPLE__)

#include <mach-o/dyld.h>
//#include <dlfcn.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 4

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

	if (handle == NULL)
		cerr << "LoadLibrary error: " << GetLastError() << endl;
	else
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

#elif defined(__APPLE__)

/* CODE TOOK FROM BLENDER DYNLIB:C SOURCE FILE */

#define ERR_STR_LEN 256

char *osxerror(int setget, const char *str, ...)
{ 
  static char errstr[ERR_STR_LEN];
  static int err_filled = 0;
  char *retval;
  NSLinkEditErrors ler; 
  int lerno;
  const char *dylderrstr;
  const char *file;
  va_list arg;
  if (setget <= 0)
  {
    va_start(arg, str);
    strncpy(errstr, "dlsimple: ", ERR_STR_LEN); 
    vsnprintf(errstr + 10, ERR_STR_LEN - 10, str, arg);
    va_end(arg);
  /* We prefer to use the dyld error string if setget is 0 */
    if (setget == 0) {
      NSLinkEditError(&ler, &lerno, &file, &dylderrstr);
      printf("dyld: %s\n",dylderrstr);
      if (dylderrstr && strlen(dylderrstr))
        strncpy(errstr,dylderrstr,ERR_STR_LEN);
    }
    err_filled = 1;
    retval = NULL;
  }
  else
  {
    if (!err_filled)
      retval = NULL;
    else
      retval = errstr;
    err_filled = 0;
  }
  return retval;
}

void *osxdlopen(const char *path, int mode)
{
  NSModule module = 0;
  NSObjectFileImage ofi = 0;
  NSObjectFileImageReturnCode ofirc;
  static int (*make_private_module_public) (NSModule module) = 0;
  unsigned int flags =  NSLINKMODULE_OPTION_RETURN_ON_ERROR | NSLINKMODULE_OPTION_PRIVATE;

  /* If we got no path, the app wants the global namespace, use -1 as the marker
     in this case */
  if (!path)
    return (void *)-1;

  /* Create the object file image, works for things linked with the -bundle arg to ld */
  ofirc = NSCreateObjectFileImageFromFile(path, &ofi);
  switch (ofirc)
  {
    case NSObjectFileImageSuccess:
      /* It was okay, so use NSLinkModule to link in the image */
      if (!(mode & RTLD_LAZY)) flags += NSLINKMODULE_OPTION_BINDNOW;
      module = NSLinkModule(ofi, path,flags);
      /* Don't forget to destroy the object file image, unless you like leaks */
      NSDestroyObjectFileImage(ofi);
      /* If the mode was global, then change the module, this avoids
         multiply defined symbol errors to first load private then make
         global. Silly, isn't it. */
      if ((mode & RTLD_GLOBAL))
      {
        if (!make_private_module_public)
        {
          _dyld_func_lookup("__dyld_NSMakePrivateModulePublic",
        (void**)&make_private_module_public);
        }
        make_private_module_public(module);
      }
      break;
    case NSObjectFileImageInappropriateFile:
      /* It may have been a dynamic library rather than a bundle, try to load it */
      module = (NSModule)NSAddImage(path, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
      break;
    case NSObjectFileImageFailure:
      osxerror(0,"Object file setup failure :  \"%s\"", path);
      return 0;
    case NSObjectFileImageArch:
      osxerror(0,"No object for this architecture :  \"%s\"", path);
      return 0;
    case NSObjectFileImageFormat:
      osxerror(0,"Bad object file format :  \"%s\"", path);
      return 0;
    case NSObjectFileImageAccess:
      osxerror(0,"Can't read object file :  \"%s\"", path);
      return 0;
  }
  if (!module)
    osxerror(0, "Can not open \"%s\"", path);
  return (void*)module;
}

void *osxdlsym(void *handle, const char *symname)
{
  int sym_len = strlen(symname);
  void *value = NULL;
  char *malloc_sym = NULL;
  NSSymbol nssym = 0;
  malloc_sym = (char *)malloc(sym_len + 2);
  if (malloc_sym)
  {
    sprintf(malloc_sym, "_%s", symname);
    /* If the handle is -1, if is the app global context */
    if (handle == (void *)-1)
    {
      /* Global context, use NSLookupAndBindSymbol */
      if (NSIsSymbolNameDefined(malloc_sym))
      {
        nssym = NSLookupAndBindSymbol(malloc_sym);
      }
    }
    /* Now see if the handle is a struch mach_header* or not, use NSLookupSymbol in image
       for libraries, and NSLookupSymbolInModule for bundles */
    else
    {
      /* Check for both possible magic numbers depending on x86/ppc byte order */
      if ((((struct mach_header *)handle)->magic == MH_MAGIC) ||
        (((struct mach_header *)handle)->magic == MH_CIGAM))
      {
        if (NSIsSymbolNameDefinedInImage((struct mach_header *)handle, malloc_sym))
        {
          nssym = NSLookupSymbolInImage((struct mach_header *)handle,
                          malloc_sym,
                          NSLOOKUPSYMBOLINIMAGE_OPTION_BIND
                          | NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
        }

      }
      else
      {
        nssym = NSLookupSymbolInModule((NSModule)handle, malloc_sym);
      }
    }
    if (!nssym)
    {
      osxerror(0, "symname \"%s\" Not found", symname);
    }
    value = NSAddressOfSymbol(nssym);
    free(malloc_sym);
  }
  else
  {
    osxerror(-1, "Unable to allocate memory");
  }
  return value;
}

void osxdlclose(void *handle)
{
  if ((((struct mach_header *)handle)->magic == MH_MAGIC) ||
    (((struct mach_header *)handle)->magic == MH_CIGAM))
  {
    osxerror(-1, "Can't remove dynamic libraries on darwin");
  }
  if (!NSUnLinkModule((NSModule)handle, 0))
  {
    osxerror(0, "unable to unlink module %s", NSNameOfModule((NSModule)handle));
  }
}

char *osxdlerror()
{
  return osxerror(1, (char *)NULL);
}

void sharedlibrary_t::open(const std::string &lib) 
{
	handle = osxdlopen(lib.c_str(),RTLD_NOW);
	if (handle == NULL) 
		cerr << "dlerror: " << osxdlerror() << endl;
	else
		refcount=new int(1);
}

void sharedlibrary_t::close() 
{
	if (handle != NULL) 
	{
		osxdlclose(handle);
		handle = NULL;
		delete refcount;
	}
}

void * sharedlibrary_t::getSymbol(const char *name) 
{
	if (handle != NULL) 
	{
		void *func = osxdlsym(handle, name);
		if (func == NULL) 
			cerr<<"dlerror: "<<osxdlerror()<<endl;
		return func;
	} 
	else 
		return NULL;
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

