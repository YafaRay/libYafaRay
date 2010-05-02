include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(glass SHARED glass.cc roughglass.cc)
target_link_libraries(glass yafaraycore)

add_library(blendermat SHARED blendmat.cc)
target_link_libraries(blendermat yafaraycore)

add_library(glossymat SHARED glossy_mat.cc)
target_link_libraries(glossymat yafaraycore)

add_library(shinydiffuse SHARED shinydiff.cc)
target_link_libraries(shinydiffuse yafaraycore)

add_library(coatedglossy SHARED coatedglossy.cc)
target_link_libraries(coatedglossy yafaraycore)

add_library(simplemats SHARED simplemats.cc maskmat.cc)
target_link_libraries(simplemats yafaraycore)

add_library(volumetrics SHARED beer.cc)
target_link_libraries(volumetrics yafaraycore)

install (TARGETS glass blendermat glossymat shinydiffuse coatedglossy simplemats volumetrics
		LIBRARY DESTINATION ${YAF_PLUGIN_DIR} RUNTIME DESTINATION ${YAF_PLUGIN_DIR})
