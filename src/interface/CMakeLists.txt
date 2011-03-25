include_directories(${YAF_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(yafarayplugin SHARED yafrayinterface.cc xmlinterface.cc)
target_link_libraries(yafarayplugin yafaraycore ${LIBXML2_LIBRARY})

if(APPLE) # set rpath - Jens
	add_custom_command(TARGET yafarayplugin POST_BUILD COMMAND install_name_tool -add_rpath @loader_path/ libyafarayplugin.dylib)
endif(APPLE)

install (TARGETS yafarayplugin ${YAF_TARGET_TYPE} DESTINATION ${YAF_LIB_DIR})
