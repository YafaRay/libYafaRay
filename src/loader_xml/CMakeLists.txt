include_directories(${YAF_INCLUDE_DIRS})

add_executable(yafaray-xml loader_xml.cc)
target_link_libraries(yafaray-xml libyafaray4)

install (TARGETS yafaray-xml RUNTIME DESTINATION ${YAF_BIN_DIR})
