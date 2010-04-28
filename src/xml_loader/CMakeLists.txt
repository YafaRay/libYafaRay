include_directories(${YAF_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR})

# yafaray-xml
add_executable(yafaray-xml xml-loader.cc)
target_link_libraries(yafaray-xml yafaraycore)

install (TARGETS yafaray-xml RUNTIME DESTINATION ${YAF_BIN_DIR} ${LIBXML2_LIBRARY})
