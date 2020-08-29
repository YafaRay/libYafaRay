include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(directlight SHARED directlight.cc)
target_link_libraries(directlight yafaray4_core)

add_library(photonmap SHARED photonintegr.cc)
target_link_libraries(photonmap yafaray4_core)

add_library(pathtrace SHARED pathtracer.cc)
target_link_libraries(pathtrace yafaray4_core)

add_library(bidirpath SHARED bidirpath.cc)
target_link_libraries(bidirpath yafaray4_core)

add_library(EmissionIntegrator SHARED EmissionIntegrator.cc)
target_link_libraries(EmissionIntegrator yafaray4_core)

add_library(SingleScatterIntegrator SHARED SingleScatterIntegrator.cc)
target_link_libraries(SingleScatterIntegrator yafaray4_core)

add_library(EmptyVolumeIntegrator SHARED EmptyVolumeIntegrator.cc)
target_link_libraries(EmptyVolumeIntegrator yafaray4_core)

add_library(DebugIntegrator SHARED DebugIntegrator.cc)
target_link_libraries(DebugIntegrator yafaray4_core)

add_library(SkyIntegrator SHARED SkyIntegrator.cc)
target_link_libraries(SkyIntegrator yafaray4_core)

add_library(sppm SHARED sppm.cc)
target_link_libraries(sppm yafaray4_core)

install (TARGETS directlight photonmap pathtrace bidirpath sppm
		EmissionIntegrator SingleScatterIntegrator EmptyVolumeIntegrator DebugIntegrator SkyIntegrator
		${YAF_TARGET_TYPE} DESTINATION ${YAF_PLUGIN_DIR})
