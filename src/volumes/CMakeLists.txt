include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(UniformVolume SHARED UniformVolume.cc)
target_link_libraries(UniformVolume yafaray_v3_core)

add_library(ExpDensityVolume SHARED ExpDensityVolume.cc)
target_link_libraries(ExpDensityVolume yafaray_v3_core)

add_library(NoiseVolume SHARED NoiseVolume.cc)
target_link_libraries(NoiseVolume yafaray_v3_core)

add_library(GridVolume SHARED GridVolume.cc)
target_link_libraries(GridVolume yafaray_v3_core)

add_library(SkyVolume SHARED SkyVolume.cc)
target_link_libraries(SkyVolume yafaray_v3_core)

install (TARGETS UniformVolume ExpDensityVolume NoiseVolume SkyVolume
		${YAF_TARGET_TYPE} DESTINATION ${YAF_PLUGIN_DIR})
