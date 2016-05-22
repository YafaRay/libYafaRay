include_directories(${YAF_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(yafaray_v3_plugin SHARED yafrayinterface.cc xmlinterface.cc)
target_link_libraries(yafaray_v3_plugin yafaray_v3_core ${LIBXML2_LIBRARY})

if(APPLE) # set rpath - Jens
	add_custom_command(TARGET yafaray_v3_plugin POST_BUILD COMMAND install_name_tool -add_rpath @loader_path/ libyafaray_v3_plugin.dylib)
endif(APPLE)

install (TARGETS yafaray_v3_plugin ${YAF_TARGET_TYPE} DESTINATION ${YAF_LIB_DIR})
