include_directories(${YAF_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(yafarayplugin SHARED yafrayinterface.cc xmlinterface.cc)
target_link_libraries(yafarayplugin yafaraycore ${LIBXML2_LIBRARY})

install (TARGETS yafarayplugin LIBRARY DESTINATION ${YAF_LIB_DIR} RUNTIME DESTINATION ${YAF_LIB_DIR})
