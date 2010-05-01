include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(UniformVolume SHARED UniformVolume.cc)
target_link_libraries(UniformVolume yafaraycore)

add_library(ExpDensityVolume SHARED ExpDensityVolume.cc)
target_link_libraries(ExpDensityVolume yafaraycore)

add_library(NoiseVolume SHARED NoiseVolume.cc)
target_link_libraries(NoiseVolume yafaraycore)

add_library(GridVolume SHARED GridVolume.cc)
target_link_libraries(GridVolume yafaraycore)

add_library(SkyVolume SHARED SkyVolume.cc)
target_link_libraries(SkyVolume yafaraycore)

install (TARGETS UniformVolume ExpDensityVolume NoiseVolume SkyVolume DESTINATION ${YAF_PLUGIN_DIR})
