include_directories(${YAF_INCLUDE_DIRS} ${RUBY_INCLUDE_PATH})
include(${SWIG_USE_FILE})

set(CMAKE_SWIG_FLAGS "-c++")

set_source_files_properties(yafrayinterface.i PROPERTIES CPLUSPLUS ON)

if(WITH_QT)
	set_source_files_properties(yafqt.i PROPERTIES CPLUSPLUS ON)
endif(WITH_QT)

SWIG_ADD_MODULE(yafrayinterface RUBY yafrayinterface.i)

if(WITH_QT)
	SWIG_ADD_MODULE(yafqt RUBY yafqt.i)
endif(WITH_QT)

SWIG_LINK_LIBRARIES(yafrayinterface yafaraycore yafarayplugin ${RUBY_LIBRARIES})

if(WITH_QT)
	SWIG_LINK_LIBRARIES(yafqt yafarayqt ${RUBY_LIBRARIES})
endif(WITH_QT)


install(TARGETS ${SWIG_MODULE_yafrayinterface_REAL_NAME} DESTINATION ${YAF_BINDINGS_RUBY_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/yafrayinterface.rb DESTINATION ${YAF_BINDINGS_RUBY_DIR})

if(WITH_QT)
	install(TARGETS ${SWIG_MODULE_yafqt_REAL_NAME} DESTINATION ${YAF_BINDINGS_RUBY_DIR})
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/yafqt.rb DESTINATION ${YAF_BINDINGS_RUBY_DIR})
endif(WITH_QT)

