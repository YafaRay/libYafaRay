include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(directlight SHARED directlight.cc)
target_link_libraries(directlight yafaraycore2)

add_library(photonmap SHARED photonintegr.cc)
target_link_libraries(photonmap yafaraycore2)

add_library(pathtrace SHARED pathtracer.cc)
target_link_libraries(pathtrace yafaraycore2)

add_library(bidirpath SHARED bidirpath.cc)
target_link_libraries(bidirpath yafaraycore2)

add_library(EmissionIntegrator SHARED EmissionIntegrator.cc)
target_link_libraries(EmissionIntegrator yafaraycore2)

add_library(SingleScatterIntegrator SHARED SingleScatterIntegrator.cc)
target_link_libraries(SingleScatterIntegrator yafaraycore2)

add_library(EmptyVolumeIntegrator SHARED EmptyVolumeIntegrator.cc)
target_link_libraries(EmptyVolumeIntegrator yafaraycore2)

add_library(DebugIntegrator SHARED DebugIntegrator.cc)
target_link_libraries(DebugIntegrator yafaraycore2)

add_library(SkyIntegrator SHARED SkyIntegrator.cc)
target_link_libraries(SkyIntegrator yafaraycore2)

add_library(sppm SHARED sppm.cc)
target_link_libraries(sppm yafaraycore2)

install (TARGETS directlight photonmap pathtrace bidirpath sppm
		EmissionIntegrator SingleScatterIntegrator EmptyVolumeIntegrator DebugIntegrator SkyIntegrator
		${YAF_TARGET_TYPE} DESTINATION ${YAF_PLUGIN_DIR})
