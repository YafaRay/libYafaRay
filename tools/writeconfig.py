import os
import sys

package='YafaRay'
version='0.1.1'
config_file='yafray_config.h'
user_config_file='yafray_user_config.h'


def write_conf(env):
	double_coords=0
	yafray_namespace='yafray'
	if double_coords :
		min_raydist="0.000000000005"
	else:
		min_raydist="0.00005"
	
	print "Creating config file:"+config_file
	config=open(config_file,'w')
	config.write("//Config file header created by scons\n\n")
	config.write("#ifndef Y_CONFIG_H\n")
	config.write("#define Y_CONFIG_H\n")
	config.write("#include \"yafray_constants.h\"\n\n")
	config.write("#define MIN_RAYDIST %s\n"%(min_raydist))
	config.write("#define YAF_SHADOW_BIAS 0.0005\n")
	config.write("\n")
	config.write("#define Y_DEBUG(DebugLevel) if (DebugLevel <= env->Debug) std::cout << \"DEBUG[\"<< DebugLevel << \"]: \"\n")
	config.write("#define Y_INFO std::cout << \"INFO: \"\n")
	config.write("#define Y_WARNING std::cout << \"WARNING: \"\n")
	config.write("#define Y_ERROR std::cout << \"ERROR: \"\n")
	config.write("\n")

	config.write("__BEGIN_YAFRAY\n");
	config.write("typedef float CFLOAT;\n");
	config.write("typedef float GFLOAT;\n");
	if double_coords:
		config.write("typedef double PFLOAT;\n");
	else:
		config.write("typedef float PFLOAT;\n");
	config.write("__END_YAFRAY\n");
	if os.path.exists(user_config_file):
		print "Using user config file: " + user_config_file
	else:
		print "Creating user config file: " + user_config_file
		config_user=open(user_config_file,'w')
		config_user.write("// Add extra information to the version string of this build\n")
		config_user.write("#define YAF_EXTRA_VERSION \n\n")
		config_user.write("// put code/defines outside of the yafray namespace here\n\n")
		config_user.write("__BEGIN_YAFRAY\n")
		config_user.write("// put code/defines inside the yafray namespace here\n\n")
		config_user.write("__END_YAFRAY\n\n")

	config.write("#include \"" + user_config_file + "\"\n");

	#if sys.platform == 'win32' :
	#	config.write("#ifdef BUILDING_YAFRAYCORE\n")
	#	config.write("#define YAFRAYCORE_EXPORT __declspec(dllexport)\n")
	#	config.write("#else \n")
	#	config.write("#define YAFRAYCORE_EXPORT __declspec(dllimport)\n")
	#	config.write("#endif \n")
	#
	#	config.write("#ifdef BUILDING_YAFRAYPLUGIN\n")
	#	config.write("#define YAFRAYPLUGIN_EXPORT __declspec(dllexport)\n")
	#	config.write("#else \n")
	#	config.write("#define YAFRAYPLUGIN_EXPORT __declspec(dllimport)\n")
	#	config.write("#endif \n")
	#else :
	#	config.write("#define YAFRAYPLUGIN_EXPORT\n")
	#	config.write("#define YAFRAYCORE_EXPORT\n")

	config.write("#endif\n");
	config.close()

def get_extra_version():
	extra_version = ""
	if os.path.exists(user_config_file):
		data = open(user_config_file).readlines()
		for line in data:
			strings = line.split()
			if "YAF_EXTRA_VERSION" in strings:
				if len(strings) > 2:
					for i in range(2,len(strings)):
						extra_version += " " + strings[i]
					return extra_version
	return ""

def write_rev(env):
	rev = os.popen('svnversion').readline().strip()
	if rev == 'exported' or rev == '': rev = "N/A"
	rev_string = rev

	#extra_version = get_extra_version()

	#if (extra_version != ''):
	#	rev_string += extra_version
	rev_string += get_extra_version()
	
	rev_file=open('yaf_revision.h','w')
	rev_file.write("#ifndef Y_REV_H\n")
	rev_file.write("#define Y_REV_H\n")
	rev_file.write("#define YAF_SVN_REV \"" + rev_string +"\"\n")
	
	rev_file.write("#define LIBPATH \"%s\"\n"%( env.subst('$YF_LIBOUT') ) )
	rev_file.write("#define Y_PLUGINPATH \"%s\"\n#endif\n"%( env.subst('$YF_PLUGINPATH') ) )
	rev_file.close()

