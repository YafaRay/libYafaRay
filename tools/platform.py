## linux specific tweaks
import os

def tweak_gnu(shared_env, program_env, static_env):
	if static_env is None: return
	arch = os.uname()[4]
	if arch == 'x86_64':
		static_env.Append(CCFLAGS = ' -fPIC')


## windows specific tweaks

def msvc8(env):
	if env.has_key('MSVS'):
		ver = env['MSVS']['VERSION']
		if ver.startswith('8.'): return True
	return False

def tweak_w32(shared_env, program_env, static_env):
	if msvc8(shared_env):
		print 'Visual Studio 8.x Detected...('+shared_env['MSVS']['VERSION']+')'

		shared_env['WINDOWS_INSERT_MANIFEST'] = True
		shared_env['SHLINKCOM']=[shared_env['SHLINKCOM'], 'mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:${TARGET};2']
		if program_env is not None:
			program_env.Replace(LINKCOM  = [program_env['LINKCOM'], 'mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:$TARGET;1'])

