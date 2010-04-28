
PREFIX = '/usr/local'
CCFLAGS = '-Wall'
REL_CCFLAGS = '-O3 -ffast-math'
DEBUG_CCFLAGS = '-ggdb'
YF_LIBOUT = '${PREFIX}/lib'
YF_PLUGINPATH = '${PREFIX}/lib/yafaray'
YF_BINPATH = '${PREFIX}/bin'
YF_BINDINGS = '${PREFIX}/share/yafaray/blender'

BASE_LPATH = '/usr/lib'
BASE_IPATH = '/usr/include'

### pthreads
WITH_YF_PTHREAD = 'true'
YF_PTHREAD_LIB = 'pthread'

### OpenEXR
WITH_YF_EXR = 'true'
YF_EXR_INC = '${BASE_IPATH}/OpenEXR'
YF_EXR_LIB = 'Half Iex Imath IlmImf'
YF_EXR_LIBPATH = '${BASE_LPATH}'

### libXML
YF_XML_INC = '${BASE_IPATH}/libxml2'
YF_XML_LIB = 'xml2'

### JPEG
WITH_YF_JPEG = 'true'
YF_JPEG_INC = ''
YF_JPEG_LIB = 'jpeg'

### PNG
WITH_YF_PNG = 'true'
YF_PNG_INC = ''
YF_PNG_LIB = 'png'

### TIFF
WITH_YF_TIFF = 'true'
YF_TIFF_INC = ''
YF_TIFF_LIB = 'tiff'

### zlib
WITH_YF_ZLIB = 'true'
YF_ZLIB_INC = ''
YF_ZLIB_LIB = 'z'

### Freetype 2
WITH_YF_FREETYPE = 'true'
YF_FREETYPE_INC = '${BASE_IPATH}/freetype2'
YF_FREETYPE_LIB = 'freetype'

### Miscellaneous
YF_MISC_LIB = 'dl'

# Qt
WITH_YF_QT='true'
#YF_QTDIR='/usr/lib/qt4'

# Python
#YF_PYTHON = '/usr'
#YF_PYTHON_VERSION = '3.1'
#YF_PYTHON_INC = '${YF_PYTHON}/include/python${YF_PYTHON_VERSION}'
#YF_PYTHON_LIBPATH = '${YF_PYTHON}/lib/python${YF_PYTHON_VERSION}'
