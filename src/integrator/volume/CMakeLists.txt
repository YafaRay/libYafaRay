target_sources(libyafaray4
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/integrator_emission.cc
		${CMAKE_CURRENT_SOURCE_DIR}/integrator_empty_volume.cc
		${CMAKE_CURRENT_SOURCE_DIR}/integrator_single_scatter.cc
		${CMAKE_CURRENT_SOURCE_DIR}/integrator_sky.cc
)