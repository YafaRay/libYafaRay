include_directories(${YAF_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(yafarayplugin2 SHARED yafrayinterface.cc xmlinterface.cc)
target_link_libraries(yafarayplugin2 yafaraycore2 ${LIBXML2_LIBRARY})

if(APPLE) # set rpath - Jens
	add_custom_command(TARGET yafarayplugin2 POST_BUILD COMMAND install_name_tool -add_rpath @loader_path/ libyafarayplugin2.dylib)
endif(APPLE)

install (TARGETS yafarayplugin2 ${YAF_TARGET_TYPE} DESTINATION ${YAF_LIB_DIR})
