include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(perspectiveCamera SHARED perspectiveCamera.cc architectCamera.cc)
target_link_libraries(perspectiveCamera yafaraycore)

add_library(orthographicCamera SHARED orthographicCamera.cc)
target_link_libraries(orthographicCamera yafaraycore)

add_library(angularCamera SHARED angularCamera.cc)
target_link_libraries(angularCamera yafaraycore)

install (TARGETS perspectiveCamera orthographicCamera angularCamera DESTINATION ${YAF_PLUGIN_DIR})
