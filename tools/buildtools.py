import SCons.Variables
import SCons.Variables.BoolVariable

Options = SCons.Variables
BoolOption = SCons.Variables.BoolVariable
Split = SCons.Util.Split

def read_opts(cfiles, cargs):
	opts = Options.Variables(cfiles, cargs)
	opts.AddVariables(
		('PREFIX', 'prefix for default installation paths', ''),
		('CCFLAGS', 'C(++) compiler flags', ''),
		('REL_CCFLAGS', 'Additional flags f. release build', ''),
		('DEBUG_CCFLAGS', 'Additional flags f. debug build', ''),
		('YF_SHLINKFLAGS', 'Additional linker flags for dynamic libraries', ''),
		('YF_LIBOUT', 'YafaRay library installation dir', '${PREFIX}'),
		('YF_PLUGINPATH', 'YafaRay plugin installation dir', '${PREFIX}/plugins'),
		('YF_BINPATH', 'YafaRay binary installation dir', '${PREFIX}'),
		('YF_PACKPATH', 'YafaRay package root dir', '#debian/yafaray'),
		('YF_BUILDPATH', 'YafaRay build dir', '#build'),
		('YF_BINDINGS', 'Python bindings installation dir', ''),
		('YF_CORELIB', 'name of YafaRay dynamic core library', 'yafaraycore'),
		(BoolOption('YF_DEBUG', 'create a debug build', 'false')),
		('BASE_LPATH', '', ''),
		('BASE_IPATH', '', ''),
	### OpenEXR
		(BoolOption('WITH_YF_EXR', 'enable OpenEXR support', 'true')),
		('YF_EXR_INC', 'OpenEXR include path', ''),
		('YF_EXR_LIBPATH', 'OpenEXR library path', ''),
		('YF_EXR_LIB', 'OpenEXR libraries', ''),
	### libXML
		(BoolOption('WITH_YF_XML', 'enable XML support', 'true')),
		('YF_XML_INC', 'libXML include path', ''),
		('YF_XML_LIBPATH', 'libXML library path', ''),
		('YF_XML_LIB', 'libXML libraries', ''),
		('YF_XML_DEF', 'libXML additional defines', None),
	### JPEG
		(BoolOption('WITH_YF_JPEG', 'enable JPEG support', 'true')),
		('YF_JPEG_INC', 'JPEG include path', ''),
		('YF_JPEG_LIBPATH', 'JPEG library path', ''),
		('YF_JPEG_LIB', 'JPEG library files', ''),
	### PNG
		(BoolOption('WITH_YF_PNG', 'enable PNG support', 'true')),
		('YF_PNG_INC', 'PNG library path', ''),
		('YF_PNG_LIBPATH', 'PNG library path', ''),
		('YF_PNG_LIB', 'PNG library files', ''),
	### libTiff
		(BoolOption('WITH_YF_TIFF', 'enable TIFF support', 'true')),
		('YF_TIFF_INC', 'TIFF include path', ''),
		('YF_TIFF_LIBPATH', 'TIFF library path', ''),
		('YF_TIFF_LIB', 'TIFF libraries', ''),
	### zlib
		(BoolOption('WITH_YF_ZLIB', 'use zlib', 'true')),
		('YF_ZLIB_INC', 'zlib include path', ''),
		('YF_ZLIB_LIBPATH', 'zlib library path', ''),
		('YF_ZLIB_LIB', 'zlib library files', ''),
	### Freetype 2
		(BoolOption('WITH_YF_FREETYPE', 'use freetype', 'true')),
		('YF_FREETYPE_INC', 'freetype include path', ''),
		('YF_FREETYPE_LIBPATH', 'freetype library path', ''),
		('YF_FREETYPE_LIB', 'freetype library files', ''),
	### pthread
		(BoolOption('WITH_YF_PTHREAD', 'use pthreads', 'true')),
		('YF_PTHREAD_INC', 'freetype include path', ''),
		('YF_PTHREAD_LIBPATH', 'freetype library path', ''),
		('YF_PTHREAD_LIB', 'freetype library files', ''),
	### miscellaneous, platform dependant required stuff
		(BoolOption('WITH_YF_MISC', 'just do not touch...', 'true')),
		('YF_MISC_INC', 'miscellaneous required include paths', ''),
		('YF_MISC_LIBPATH', 'miscellaneous required library paths', ''),
		('YF_MISC_LIB', 'miscellaneous required library files', ''),
	### Qt
		(BoolOption('WITH_YF_QT', 'enable Qt GUI support', 'true')),
		('YF_QTDIR', 'Qt base path getting used for QTDIR environment var', '/usr/share/qt4'),
		('YF_QT4_LIB', 'Qt library files', 'QtGui QtCore'),

		('YF_PYTHON', 'Python prefix', ''),
		('YF_PYTHON_VERSION', 'Python version', ''),
		('YF_PYTHON_INC', 'Python include dir', ''),
		('YF_PYTHON_LIBPATH', 'Python lib dir', '')
	);
	return opts

def add_lib_paths(lenv):
	incpaths = []
	libpaths = []
	if lenv['WITH_YF_EXR']:
		incpaths += Split(lenv['YF_EXR_INC'])
		libpaths += Split(lenv['YF_EXR_LIBPATH'])
	if lenv['WITH_YF_XML']:
		incpaths += Split(lenv['YF_XML_INC'])
		libpaths += Split(lenv['YF_XML_LIBPATH'])
	if lenv['WITH_YF_JPEG']:
		incpaths += Split(lenv['YF_JPEG_INC'])
		libpaths += Split(lenv['YF_JPEG_LIBPATH'])
	if lenv['WITH_YF_PNG']:
		incpaths += Split(lenv['YF_PNG_INC'])
		libpaths += Split(lenv['YF_PNG_LIBPATH'])
	if lenv['WITH_YF_TIFF']:
		incpaths += Split(lenv['YF_TIFF_INC'])
		libpaths += Split(lenv['YF_TIFF_LIBPATH'])
	if lenv['WITH_YF_ZLIB']:
		incpaths += Split(lenv['YF_ZLIB_INC'])
		libpaths += Split(lenv['YF_ZLIB_LIBPATH'])
	if lenv['WITH_YF_FREETYPE']:
		incpaths += Split(lenv['YF_FREETYPE_INC'])
		libpaths += Split(lenv['YF_FREETYPE_LIBPATH'])
	if lenv['WITH_YF_PTHREAD']:
		incpaths += Split(lenv['YF_PTHREAD_INC'])
		libpaths += Split(lenv['YF_PTHREAD_LIBPATH'])
	lenv.Append(INCLUDE=incpaths, LIBPATH=libpaths)

def config_lib(conf, names, cpp=False):
	libs = Split(names)
	lang = "C"
	if cpp: lang = "C++"
	for i in libs:
		if not conf.CheckLib(i, None, None, lang):
			return False
	return True

# check if all activated libraries really are available
def check_config(env):
	common_env = env.Clone();
	add_lib_paths(common_env)
	conf = common_env.Configure()
	ok = True
	if common_env['WITH_YF_PTHREAD']:
		if not config_lib(conf, common_env['YF_PTHREAD_LIB']):
			ok = False
	else:	print "skipping pthreads..."
	if common_env['WITH_YF_ZLIB']:
		if not config_lib(conf, common_env['YF_ZLIB_LIB']):
			ok = False
	else:	print "skipping zlib..."
	if common_env['WITH_YF_EXR']:
		if not config_lib(conf, common_env['YF_EXR_LIB'], True):
			ok = False
	else:	print "skipping OpenEXR..."
	if common_env['WITH_YF_XML']:
		if not config_lib(conf, common_env['YF_XML_LIB']):
			ok = False
	else:	print "skipping libXML..."
	if common_env['WITH_YF_JPEG']:
		if not config_lib(conf, common_env['YF_JPEG_LIB']):
			ok = False
	else:	print "skipping JPEG..."
	if common_env['WITH_YF_PNG']:
		if not config_lib(conf, common_env['YF_PNG_LIB']):
			ok = False
	else:	print "skipping PNG..."
	if common_env['WITH_YF_TIFF']:
		if not config_lib(conf, common_env['YF_TIFF_LIB']):
			ok = False
	else:	print "skipping TIFF..."
	if common_env['WITH_YF_FREETYPE']:
		if not config_lib(conf, common_env['YF_FREETYPE_LIB']):
			ok = False
	else:	print "skipping freetype..."
	return ok

## append lib to environment
## if name is FOO, required variables are WITH_YF_FOO,
## YF_FOO_LIB, YF_FOO_INC and YF_FOO_LIBPATH.
## in addition, HAVE_FOO will be added to defines.
def append_lib(env, libs):
	for name in libs:
		bname = 'YF_'+name
		if not env['WITH_'+bname]: continue
		lib_name = Split(env[bname + '_LIB'])
		lib_path = Split(env[bname + '_LIBPATH'])
		lib_inc = Split(env[bname + '_INC'])
#		lib_def = ['HAVE_' + name]
#		if env.has_key(bname + '_DEF'): lib_def += env[bname + '_DEF']
#		env.Append(CPPPATH = lib_inc, LIBPATH = lib_path, CPPDEFINES = lib_def, LIBS = lib_name )
		env.Append(CPPPATH = lib_inc, LIBPATH = lib_path, LIBS = lib_name )

def append_defines(env, libs):
	lib_def = []
	for name in libs:
		bname = 'YF_'+name
		if not env['WITH_'+bname]: continue
		lib_def += ['HAVE_' + name]
		if env.has_key(bname + '_DEF'): lib_def += env[bname + '_DEF']	
	env.Append(CPPDEFINES = lib_def)

def append_includes(env, libs):
	lib_inc = []
	for name in libs:
		bname = 'YF_'+name
		if not env['WITH_'+bname]: continue
		lib_inc = Split(env[bname + '_INC'])
	env.Append(CPPPATH = lib_inc)

