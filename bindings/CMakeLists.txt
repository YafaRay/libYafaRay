include_directories(${YAF_INCLUDE_DIRS} ${PYTHON_INCLUDE_PATH})
include(${SWIG_USE_FILE})

set(CMAKE_SWIG_FLAGS "-c++")

set_source_files_properties(yafrayinterface.i PROPERTIES CPLUSPLUS ON)
set_source_files_properties(yafqt.i PROPERTIES CPLUSPLUS ON)
SWIG_ADD_MODULE(yafrayinterface PYTHON yafrayinterface.i)
SWIG_ADD_MODULE(yafqt PYTHON yafqt.i)
SWIG_LINK_LIBRARIES(yafrayinterface yafaraycore yafarayplugin ${PYTHON_LIBRARIES})
SWIG_LINK_LIBRARIES(yafqt yafarayqt ${PYTHON_LIBRARIES})


install(TARGETS ${SWIG_MODULE_yafrayinterface_REAL_NAME} DESTINATION ${YAF_BINDINGS_PY_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/yafrayinterface.py DESTINATION ${YAF_BINDINGS_PY_DIR})
install(TARGETS ${SWIG_MODULE_yafqt_REAL_NAME} DESTINATION ${YAF_BINDINGS_PY_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/yafqt.py DESTINATION ${YAF_BINDINGS_PY_DIR})

