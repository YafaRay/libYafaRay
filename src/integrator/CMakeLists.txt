target_sources(libyafaray4
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/integrator.cc
)

add_subdirectory(surface)
add_subdirectory(volume)