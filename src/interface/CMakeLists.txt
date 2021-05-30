include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFARAYPLUGIN)

add_library(yafaray_plugin SHARED interface.cc export/export_xml.cc)
target_link_libraries(yafaray_plugin libyafaray4)

if(APPLE) # set rpath - Jens
	add_custom_command(TARGET yafaray_plugin POST_BUILD COMMAND install_name_tool -add_rpath @loader_path/ libyafaray_plugin.dylib)
endif(APPLE)

install (TARGETS yafaray_plugin ${YAF_TARGET_TYPE} DESTINATION ${YAF_LIB_DIR})
