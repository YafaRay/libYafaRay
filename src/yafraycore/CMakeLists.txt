include_directories(${YAF_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR} ${OPENEXR_INCLUDE_DIRS}
                    ${FREETYPE_INCLUDE_DIRS})
set(YF_CORE_SOURCES bound.cc yafsystem.cc environment.cc console.cc color_console.cc
					logging.cc faure_tables.cc std_primitives.cc color.cc renderpasses.cc
					matrix4.cc object3d.cc timer.cc kdtree.cc ray_kdtree.cc hashgrid.cc tribox3_d.cc
					triclip.cc scene.cc imagefilm.cc imagesplitter.cc material.cc nodematerial.cc
					triangle.cc vector3d.cc photon.cc xmlparser.cc spectrum.cc volume.cc
					surface.cc integrator.cc mcintegrator.cc ccthreads.cc
					imageOutput.cc memoryIO.cc ${headers})

add_definitions(-DBUILDING_YAFRAYCORE)

add_library(yafaray_e2_core SHARED ${YF_CORE_SOURCES})

if (UNIX)
	set (DLLOAD_LIB "dl" )
else (UNIX)
	set (DLLOAD_LIB "")
endif (UNIX)

if(APPLE) # set rpath - Jens
	add_custom_command(TARGET yafaray_e2_core POST_BUILD COMMAND install_name_tool -add_rpath @loader_path/ libyafaray_e2_core.dylib)
endif(APPLE)

target_link_libraries(yafaray_e2_core ${CMAKE_THREAD_LIBS_INIT} ${DLLOAD_LIB} ${OPENEXR_LIBRARIES} ${LIBXML2_LIBRARIES} ${FREETYPE_LIBRARIES})

install (TARGETS yafaray_e2_core ${YAF_TARGET_TYPE} DESTINATION ${YAF_LIB_DIR})
