include_directories(${YAF_INCLUDE_DIRS} ${PNG_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${OPENEXR_INCLUDE_DIRS})

add_definitions(-DBUILDING_YAFRAYPLUGIN)

set(basictex_SRCS basictex.cc noise.cc jpeg.cc imagetex.cc rgbe_texture.cc png.cc)
add_library(basictex SHARED ${basictex_SRCS})
target_link_libraries(basictex yafaraycore ${PNG_LIBRARIES} ${OPENEXR_LIBRARIES} ${JPEG_LIBRARY})

add_library(basicnodes SHARED basicnodes.cc layernode.cc)
target_link_libraries(basicnodes yafaraycore)

install (TARGETS basictex basicnodes LIBRARY DESTINATION ${YAF_PLUGIN_DIR})
