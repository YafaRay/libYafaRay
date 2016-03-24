include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(glass SHARED glass.cc)
target_link_libraries(glass yafaray_e3_core)

add_library(roughglass SHARED roughglass.cc)
target_link_libraries(roughglass yafaray_e3_core)

add_library(blend SHARED blend.cc)
target_link_libraries(blend yafaray_e3_core)

add_library(glossy SHARED glossy.cc)
target_link_libraries(glossy yafaray_e3_core)

add_library(shinydiffuse SHARED shinydiffuse.cc)
target_link_libraries(shinydiffuse yafaray_e3_core)

add_library(coatedglossy SHARED coatedglossy.cc)
target_link_libraries(coatedglossy yafaray_e3_core)

add_library(simple SHARED simple.cc mask.cc)
target_link_libraries(simple yafaray_e3_core)

add_library(volumetric SHARED beer.cc)
target_link_libraries(volumetric yafaray_e3_core)

install (TARGETS glass roughglass blend glossy shinydiffuse coatedglossy simple volumetric
		${YAF_TARGET_TYPE} DESTINATION ${YAF_PLUGIN_DIR})
