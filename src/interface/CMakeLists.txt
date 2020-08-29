include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(yafaray4_plugin SHARED yafrayinterface.cc xmlinterface.cc)
target_link_libraries(yafaray4_plugin yafaray4_core)

if(APPLE) # set rpath - Jens
	add_custom_command(TARGET yafaray4_plugin POST_BUILD COMMAND install_name_tool -add_rpath @loader_path/ libyafaray4_plugin.dylib)
endif(APPLE)

install (TARGETS yafaray4_plugin ${YAF_TARGET_TYPE} DESTINATION ${YAF_LIB_DIR})
