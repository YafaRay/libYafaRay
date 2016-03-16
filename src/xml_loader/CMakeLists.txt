include_directories(${YAF_INCLUDE_DIRS})

add_executable(yafaray-xml xml-loader.cc)
target_link_libraries(yafaray-xml yafaraycore2)

install (TARGETS yafaray-xml RUNTIME DESTINATION ${YAF_BIN_DIR})
