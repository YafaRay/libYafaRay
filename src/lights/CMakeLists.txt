include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(sunlight SHARED sunlight.cc)
target_link_libraries(sunlight yafaray_v3_core)

add_library(pointlight SHARED pointlight.cc)
target_link_libraries(pointlight yafaray_v3_core)

add_library(directional SHARED directional.cc)
target_link_libraries(directional yafaray_v3_core)

add_library(spotlight SHARED spotlight.cc)
target_link_libraries(spotlight yafaray_v3_core)

add_library(arealight SHARED arealight.cc meshlight.cc bgportallight.cc)
target_link_libraries(arealight yafaray_v3_core)

add_library(spherelight SHARED spherelight.cc)
target_link_libraries(spherelight yafaray_v3_core)

add_library(ieslight SHARED iesLight.cc)
target_link_libraries(ieslight yafaray_v3_core)

add_library(bglight SHARED bglight.cc)
target_link_libraries(bglight yafaray_v3_core)

install (TARGETS sunlight pointlight directional spotlight arealight spherelight ieslight bglight
		${YAF_TARGET_TYPE} DESTINATION ${YAF_PLUGIN_DIR})
