include_directories(${YAF_INCLUDE_DIRS} ${PNG_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${TIFF_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${OPENEXR_INCLUDE_DIRS})
add_definitions(-DBUILDING_YAFRAYPLUGIN)

add_library(exrHandler SHARED exrHandler.cc)
target_link_libraries(exrHandler yafaraycore ${OPENEXR_LIBRARIES})

add_library(jpgHandler SHARED jpgHandler.cc)
target_link_libraries(jpgHandler yafaraycore ${JPEG_LIBRARY})

add_library(pngHandler SHARED pngHandler.cc)
target_link_libraries(pngHandler yafaraycore ${PNG_LIBRARIES})

add_library(tgaHandler SHARED tgaHandler.cc)
target_link_libraries(tgaHandler yafaraycore)

add_library(tifHandler SHARED tifHandler.cc)
target_link_libraries(tifHandler yafaraycore ${TIFF_LIBRARY})

add_library(hdrHandler SHARED hdrHandler.cc)
target_link_libraries(hdrHandler yafaraycore)

install (TARGETS exrHandler jpgHandler pngHandler tgaHandler tifHandler hdrHandler DESTINATION ${YAF_PLUGIN_DIR})

