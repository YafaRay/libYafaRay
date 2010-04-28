include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(sunlight SHARED sunlight.cc)
target_link_libraries(sunlight yafaraycore)

add_library(pointlight SHARED pointlight.cc)
target_link_libraries(pointlight yafaraycore)

add_library(directional SHARED directional.cc)
target_link_libraries(directional yafaraycore)

add_library(spotlight SHARED spotlight.cc)
target_link_libraries(spotlight yafaraycore)

add_library(arealight SHARED arealight.cc meshlight.cc bgportallight.cc)
target_link_libraries(arealight yafaraycore)

add_library(spherelight SHARED spherelight.cc)
target_link_libraries(spherelight yafaraycore)

install (TARGETS sunlight pointlight directional spotlight arealight spherelight LIBRARY DESTINATION ${YAF_PLUGIN_DIR})
