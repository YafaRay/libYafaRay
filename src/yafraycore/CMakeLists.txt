include_directories(${YAF_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR} ${OPENEXR_INCLUDE_DIRS}
                    ${FREETYPE_INCLUDE_DIRS} ${PNG_INCLUDE_DIR})
set(YF_CORE_SOURCES bound.cc yafsystem.cc environment.cc console.cc faure_tables.cc 
					std_primitives.cc color.cc matrix4.cc object3d.cc
					timer.cc kdtree.cc ray_kdtree.cc tribox3_d.cc triclip.cc scene.cc
					imagefilm.cc imagesplitter.cc material.cc nodematerial.cc
					ccthreads.cc triangle.cc vector3d.cc photon.cc
					xmlparser.cc spectrum.cc volume.cc
					memoryIO.cc surface.cc irradiancecache.cc integrator.cc
					imageOutput.cc EXR_io.cc tga_io.cc)

add_definitions(-DBUILDING_YAFRAYCORE)

add_library(yafaraycore SHARED ${YF_CORE_SOURCES})

if (UNIX)
	set (DLLOAD_LIB "dl" )
else (UNIX)
	set (DLLOAD_LIB "")
endif (UNIX)

target_link_libraries(yafaraycore ${CMAKE_THREAD_LIBS_INIT} ${DLLOAD_LIB} ${PNG_LIBRARIES} ${OPENEXR_LIBRARIES} ${LIBXML2_LIBRARIES} ${FREETYPE_LIBRARIES})

install (TARGETS yafaraycore DESTINATION ${YAF_LIB_DIR})
