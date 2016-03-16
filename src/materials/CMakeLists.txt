include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(glass SHARED glass.cc)
target_link_libraries(glass yafaraycore2)

add_library(roughglass SHARED roughglass.cc)
target_link_libraries(roughglass yafaraycore2)

add_library(blend SHARED blend.cc)
target_link_libraries(blend yafaraycore2)

add_library(glossy SHARED glossy.cc)
target_link_libraries(glossy yafaraycore2)

add_library(shinydiffuse SHARED shinydiffuse.cc)
target_link_libraries(shinydiffuse yafaraycore2)

add_library(coatedglossy SHARED coatedglossy.cc)
target_link_libraries(coatedglossy yafaraycore2)

add_library(simple SHARED simple.cc mask.cc)
target_link_libraries(simple yafaraycore2)

add_library(volumetric SHARED beer.cc)
target_link_libraries(volumetric yafaraycore2)

install (TARGETS glass roughglass blend glossy shinydiffuse coatedglossy simple volumetric
		${YAF_TARGET_TYPE} DESTINATION ${YAF_PLUGIN_DIR})
