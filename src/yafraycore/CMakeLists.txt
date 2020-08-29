include_directories(${YAF_INCLUDE_DIRS})
include_directories(SYSTEM ${LIBXML2_INCLUDE_DIR} ${OPENEXR_INCLUDE_DIRS}
                    ${FREETYPE_INCLUDE_DIRS})
set(YF_CORE_SOURCES bound.cc dynamic_library.cc file.cc environment.cc console.cc color_console.cc color_ramp.cc
					sysinfo.cc logging.cc session.cc faure_tables.cc std_primitives.cc color.cc renderpasses.cc
					matrix4.cc object3d.cc timer.cc kdtree.cc ray_kdtree.cc hashgrid.cc tribox3_d.cc
					triclip.cc scene.cc imagefilm.cc imagesplitter.cc material.cc nodematerial.cc
					triangle.cc vector3d.cc photon.cc xmlparser.cc spectrum.cc volume.cc
					surface.cc integrator.cc mcintegrator.cc params.cc
					imageOutput.cc memoryIO.cc imagehandler.cc ${headers})

add_definitions(-DBUILDING_YAFRAYCORE)

add_library(yafaray4_core SHARED ${YF_CORE_SOURCES})

set (DLLOAD_LIB ${CMAKE_DL_LIBS})

if(APPLE) # set rpath - Jens
	add_custom_command(TARGET yafaray4_core POST_BUILD COMMAND install_name_tool -add_rpath @loader_path/ libyafaray4_core.dylib)
endif(APPLE)

target_link_libraries(yafaray4_core ${DLLOAD_LIB} ${OPENEXR_LIBRARIES} ${LIBXML2_LIBRARIES} ${FREETYPE_LIBRARIES} ${OpenCV_LIBRARIES})

install (TARGETS yafaray4_core ${YAF_TARGET_TYPE} DESTINATION ${YAF_LIB_DIR})
