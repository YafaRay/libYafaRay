#!/usr/bin/env python

import os
import sys

import tools.buildtools
import tools.platform
import tools.writeconfig
buildtools = tools.buildtools
configio = tools.writeconfig

configurations = {
	'darwin' : 'config/darwin-config.py',
	'linux2' : 'config/linux2-config.py',
	'win32' : 'config/win32-vc-config.py',
	'sunos5' : 'config/sunos-settings.py',
	'freebsd4' : 'config/freebsd-config.py',
	'freebsd5' : 'config/freebsd-config.py',
	'freebsd6' : 'config/freebsd-config.py',
	'freebsd7' : 'config/freebsd-config.py',
}

configfile = configurations[sys.platform]
if configfile == None:
	print 'error: no platform config available'
	Exit(1)
if not os.path.exists(configfile):
	print configfile + ' is not available.'
	print 'the yafray project will gladly accept contributions...'
	Exit(1)
print 'using configuration from ' + configfile

optfiles = [configfile]
if os.path.exists('user-config.py'):
	print "Using config file: " + 'user-config.py'
	optfiles += ['user-config.py']
else:
	print 'user-config.py not available, doing no user overrides'

opts = buildtools.read_opts(optfiles, ARGUMENTS)

common_env = Environment(ENV=os.environ, options = opts)
Help(opts.GenerateHelpText(common_env))
## set version information
common_env['YF_VERSION'] = '0.1.1'

release = ARGUMENTS.get('release', 0)
if release:
	common_env.Append (CPPDEFINES= ['RELEASE'])

yafqt_font_embed = ARGUMENTS.get('qt_font_embed', 0)
if yafqt_font_embed:
	common_env.Append (CPPDEFINES= ['YAFQT_EMBEDED_FONT'])

if not buildtools.check_config(common_env):
	print 'Error: not all enabled libraries are available!'
	print 'if they are installed, make sure the paths are setup correctly'
	print 'if you don\'t have them, try disabling with the appropriate WITH_YF_* option'
	Exit(1)
if common_env['YF_DEBUG']:
	common_env.Append(CPPFLAGS = common_env['DEBUG_CCFLAGS'])
else:
	common_env.Append(CPPFLAGS = common_env['REL_CCFLAGS'])

tools.buildtools.append_defines(common_env, ['PTHREAD','EXR','XML','JPEG','PNG','ZLIB','FREETYPE','QT'])
qt_env = common_env.Clone(tools=['default','qt'], QTDIR=common_env['YF_QTDIR'], QT_LIB=None)
qt_env.Replace(CCFLAGS = common_env['CCFLAGS'])
common_env.Append (SHLINKFLAGS = common_env['YF_SHLINKFLAGS'], CPPPATH = [ '#', '#/include' ])
qt_env.Append (SHLINKFLAGS = common_env['YF_SHLINKFLAGS'], CPPPATH = [ '#', '#/include' ])
common_env.SConsignFile('#.sconsign')


## create config headers
configio.write_conf(common_env)
configio.write_rev(common_env)

## creating environments
shared_env = common_env.Clone()
## setup program environment ##
program_env=common_env.Clone()
static_env=common_env.Clone()

if sys.platform == 'win32':
	tools.platform.tweak_w32(shared_env, program_env, static_env)
	tools.platform.tweak_w32(qt_env, None, None)
elif sys.platform == 'linux2': tools.platform.tweak_gnu(shared_env, program_env, static_env)

## setup plugin environment ##
plugin_env = shared_env.Clone()
plugin_env.Append (LIBS = common_env['YF_CORELIB'])
plugin_env.Append (LIBPATH = ['../yafraycore'])
plugin_env.Append (CPPDEFINES= ['BUILDING_YAFRAYPLUGIN'])


Export('shared_env')
Export('plugin_env')
Export('program_env')
Export('static_env')
Export('qt_env')
append_lib = tools.buildtools.append_lib
Export('append_lib')
append_includes = tools.buildtools.append_includes
Export('append_includes')

BuildDir(common_env['YF_BUILDPATH'] + '/yafraycore', 'src/yafraycore', duplicate=0)
SConscript([common_env['YF_BUILDPATH'] + '/yafraycore/SConscript'])

BuildDir(common_env['YF_BUILDPATH'] + '/cameras', 'src/cameras', duplicate=0)
SConscript([common_env['YF_BUILDPATH'] + '/cameras/SConscript'])

BuildDir(common_env['YF_BUILDPATH'] + '/lights', 'src/lights', duplicate=0)
SConscript([common_env['YF_BUILDPATH'] + '/lights/SConscript'])

BuildDir(common_env['YF_BUILDPATH'] + '/volumes', 'src/volumes', duplicate=0)
SConscript([common_env['YF_BUILDPATH'] + '/volumes/SConscript'])

BuildDir(common_env['YF_BUILDPATH'] + '/backgrounds', 'src/backgrounds', duplicate=0)
SConscript([common_env['YF_BUILDPATH'] + '/backgrounds/SConscript'])

BuildDir(common_env['YF_BUILDPATH'] + '/materials', 'src/materials', duplicate=0)
SConscript([common_env['YF_BUILDPATH'] + '/materials/SConscript'])

BuildDir(common_env['YF_BUILDPATH'] + '/textures', 'src/textures', duplicate=0)
SConscript([common_env['YF_BUILDPATH'] + '/textures/SConscript'])

BuildDir(common_env['YF_BUILDPATH'] + '/integrators', 'src/integrators', duplicate=0)
SConscript([common_env['YF_BUILDPATH'] + '/integrators/SConscript'])

BuildDir(common_env['YF_BUILDPATH'] + '/interface', 'src/interface', duplicate=0)
SConscript([common_env['YF_BUILDPATH'] + '/interface/SConscript'])

BuildDir(common_env['YF_BUILDPATH'] + '/xml_loader', 'src/xml_loader', duplicate=0)
SConscript([common_env['YF_BUILDPATH'] + '/xml_loader/SConscript'])

if common_env['WITH_YF_QT']:
	BuildDir(common_env['YF_BUILDPATH'] + '/gui', 'src/gui', duplicate=0)
	SConscript([common_env['YF_BUILDPATH'] + '/gui/SConscript'])

if 'debian' in COMMAND_LINE_TARGETS:
	SConscript("tools/debian/SConscript")

if 'swig' in COMMAND_LINE_TARGETS:
	BuildDir(common_env['YF_BUILDPATH'] + '/bindings', 'bindings', duplicate=0)
	SConscript(common_env['YF_BUILDPATH'] + "/bindings/SConscript")

if 'swig_install' in COMMAND_LINE_TARGETS:
	BuildDir(common_env['YF_BUILDPATH'] + '/bindings', 'bindings', duplicate=0)
	SConscript(common_env['YF_BUILDPATH'] + "/bindings/SConscript")

Alias('install',['install_core',
				 'install_xml_loader',
				 'install_cameras',
				 'install_lights',
				 'install_volumes',
				 'install_mat',
				 'install_tex',
				 'install_integr',
				 'install_interf'
	])

Default(common_env['YF_BUILDPATH'])

## test...dump environment for review...
#env_txt = open("env.txt", "w")
#env_txt.write( common_env.Dump() )

