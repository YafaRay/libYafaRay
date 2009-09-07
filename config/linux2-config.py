
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
YF_PTHREAD_LIB = 'pthread'

### OpenEXR
YF_EXR_INC = '${BASE_IPATH}/OpenEXR'
YF_EXR_LIB = 'Half Iex Imath IlmImf'
YF_EXR_LIBPATH = '${BASE_LPATH}'

### libXML
YF_XML_INC = '${BASE_IPATH}/libxml2'
YF_XML_LIB = 'xml2'

### JPEG
YF_JPEG_INC = ''
YF_JPEG_LIB = 'jpeg'

### PNG
WITH_YF_PNG = 'true'
YF_PNG_INC = ''
YF_PNG_LIB = 'png'

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

WITH_YF_QT='false'
