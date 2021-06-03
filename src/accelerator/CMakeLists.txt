target_sources(libyafaray4
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/accelerator.cc
		${CMAKE_CURRENT_SOURCE_DIR}/accelerator_kdtree.cc
		${CMAKE_CURRENT_SOURCE_DIR}/accelerator_kdtree_multi_thread.cc
		${CMAKE_CURRENT_SOURCE_DIR}/accelerator_simple_test.cc
)