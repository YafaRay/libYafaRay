
PREFIX = '#win32pak'
CCFLAGS = '/DWIN32 /D_WIN32 /D_USE_MATH_DEFINES /EHsc /MD /nologo'
REL_CCFLAGS = ' /O2'
DEBUG_CCFLAGS = ' /Zi /GS /RTC1 /Fd'
YF_LIBOUT = '${PREFIX}'
YF_PLUGINPATH = '${PREFIX}/plugins'
YF_BINPATH = '${PREFIX}'
YF_CORELIB = 'yafraycore'

BASE_LPATH = '#../libs/msvc'
BASE_IPATH = '#../libs/msvc'

### pthreads
YF_PTHREAD_INC = '${BASE_IPATH}/pthreads/include'
YF_PTHREAD_LIBPATH = '${BASE_LPATH}/pthreads/lib'
YF_PTHREAD_LIB = 'pthreadVC2'

### OpenEXR
YF_EXR_INC = '${BASE_IPATH}/openexr_static/include/OpenEXR'
YF_EXR_LIBPATH = '${BASE_LPATH}/openexr_static/lib'
YF_EXR_LIB = 'Half IlmImf Iex Imath IlmThread'

### libXML
YF_XML_INC = '${BASE_IPATH}/libxml2/include'
YF_XML_LIBPATH = '${BASE_LPATH}/libxml2/lib'
YF_XML_LIB = 'libxml2_a'
YF_XM_DEF = 'LIBXML_STATIC'

### JPEG
YF_JPEG_INC = '${BASE_IPATH}/jpeg/include'
YF_JPEG_LIBPATH = '${BASE_LPATH}/jpeg/lib'
YF_JPEG_LIB = 'libjpeg'

### PNG
WITH_YF_PNG = 'true'
YF_PNG_INC = '${BASE_IPATH}/png/include'
YF_PNG_LIBPATH = '${BASE_LPATH}/png/lib'
YF_PNG_LIB = 'libpng'

### zlib
WITH_YF_ZLIB = 'true'
YF_ZLIB_INC = '${BASE_IPATH}/zlib/include'
YF_ZLIB_LIBPATH = '${BASE_LPATH}/zlib/lib'
YF_ZLIB_LIB = 'zlib'

### Freetype 2
WITH_YF_FREETYPE = 'true'
YF_FREETYPE_INC = '${BASE_IPATH}/freetype2/include'
YF_FREETYPE_LIBPATH = '${BASE_LPATH}/freetype2/lib'
YF_FREETYPE_LIB = 'freetype'

### Miscellaneous
YF_MISC_LIB = 'Advapi32'

### Python
YF_PYTHON_LIBPATH = 'C:\Python25\libs'

YF_QT4_LIB = 'QtGui4 QtCore4'
