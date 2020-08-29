include_directories(${YAF_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(sunsky SHARED sunsky.cc sunspectrum.cc)
target_link_libraries(sunsky yafaray4_core)

add_library(textureback SHARED textureback.cc)
target_link_libraries(textureback yafaray4_core)

add_library(gradientback SHARED gradientback.cc)
target_link_libraries(gradientback yafaray4_core)

add_library(SunSky2 SHARED darksky.cc)
target_link_libraries(SunSky2 yafaray4_core)

install (TARGETS gradientback textureback sunsky SunSky2 ${YAF_TARGET_TYPE} DESTINATION ${YAF_PLUGIN_DIR})
