SET(CMAKE_INSTALL_RPATH "$ORIGIN/:$ORIGIN/bin")

include_directories(${YAF_INCLUDE_DIRS})

add_executable(yafaray-xml xml-loader.cc)
target_link_libraries(yafaray-xml yafaray_v3_core)

install (TARGETS yafaray-xml RUNTIME DESTINATION ${YAF_BIN_DIR})
