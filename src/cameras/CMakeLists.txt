include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(perspectiveCamera SHARED perspectiveCamera.cc architectCamera.cc)
target_link_libraries(perspectiveCamera yafaray_v3_core)

add_library(orthographicCamera SHARED orthographicCamera.cc)
target_link_libraries(orthographicCamera yafaray_v3_core)

add_library(angularCamera SHARED angularCamera.cc)
target_link_libraries(angularCamera yafaray_v3_core)

add_library(equirectangularCamera SHARED equirectangularCamera.cc)
target_link_libraries(equirectangularCamera yafaray_v3_core)

install (TARGETS perspectiveCamera orthographicCamera angularCamera equirectangularCamera ${YAF_TARGET_TYPE} DESTINATION ${YAF_PLUGIN_DIR})
