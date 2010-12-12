#check for swig if needed

if (WITH_YAF_PY_BINDINGS OR WITH_YAF_RUBY_BINDINGS)
	
	find_package(SWIG REQUIRED)
	set(YAF_SWIG_MIN_VERSION 1.3.36)
	
	if(NOT SWIG_FOUND)
		message("Swig not found, please install it to build script bindings")
		set(WITH_YAF_PY_BINDINGS OFF)
		set(WITH_YAF_RUBY_BINDINGS OFF)
	else(NOT SWIG_FOUND)
		if(SWIG_VERSION VERSION_LESS ${YAF_SWIG_MIN_VERSION})
			message("Swig version < ${YAF_SWIG_MIN_VERSION}, please update or bindings can't be built")
			set(WITH_YAF_PY_BINDINGS OFF)
			set(WITH_YAF_RUBY_BINDINGS OFF)
		endif(SWIG_VERSION VERSION_LESS ${YAF_SWIG_MIN_VERSION})
	endif(NOT SWIG_FOUND)
	
	include(${SWIG_USE_FILE})
endif (WITH_YAF_PY_BINDINGS OR WITH_YAF_RUBY_BINDINGS)

# Python bindings 

if (WITH_YAF_PY_BINDINGS)
	
	if(YAF_PY_VERSION)
		set(REQUIRED_PYTHON_VERSION ${YAF_PY_VERSION})
	else(YAF_PY_VERSION)
		set(REQUIRED_PYTHON_VERSION 2.6)
	endif(YAF_PY_VERSION)
	
	MESSAGE("Searching for Python ${REQUIRED_PYTHON_VERSION}")
	find_package(YafPythonLibs REQUIRED)
	
	if (NOT PYTHON_LIBRARIES)
		message("Python wasn't found, please install it to build python bindings")
		set(WITH_YAF_PY_BINDINGS OFF)
	else (NOT PYTHON_LIBRARIES)
		message("Python ${REQUIRED_PYTHON_VERSION} found at: ${PYTHON_LIBRARIES}")
	endif (NOT PYTHON_LIBRARIES)
	
endif (WITH_YAF_PY_BINDINGS)

# Ruby bindings

if (WITH_YAF_RUBY_BINDINGS)
	
	find_package(Ruby REQUIRED)

	if (NOT SWIG_FOUND OR NOT RUBY_FOUND)
		set(WITH_YAF_RUBY_BINDINGS OFF)
	endif (NOT SWIG_FOUND OR NOT RUBY_FOUND)
	
endif (WITH_YAF_RUBY_BINDINGS)


if (WITH_YAF_PY_BINDINGS)
	
	if(NOT WIN32)

		if(NOT YAF_BINDINGS_PY_DIR)
			set(YAF_BINDINGS_PY_DIR ${CMAKE_SOURCE_DIR}/install/bindings/python)
		endif(NOT YAF_BINDINGS_PY_DIR)
		
	else(NOT WIN32)

		if(NOT YAF_BINDINGS_PY_DIR)
			set(YAF_BINDINGS_PY_DIR ${YAF_BIN_DIR}/python)
		endif(NOT YAF_BINDINGS_PY_DIR)

	endif(NOT WIN32)
	
	add_subdirectory(blender-python)

endif (WITH_YAF_PY_BINDINGS)

if (WITH_YAF_RUBY_BINDINGS)
	
	if(NOT WIN32)

		if(NOT YAF_BINDINGS_RUBY_DIR)
			set(YAF_BINDINGS_RUBY_DIR ${CMAKE_SOURCE_DIR}/install/bindings/ruby)
		endif(NOT YAF_BINDINGS_RUBY_DIR)
		
	else(NOT WIN32)

		if(NOT YAF_BINDINGS_RUBY_DIR)
			set(YAF_BINDINGS_RUBY_DIR ${YAF_BIN_DIR}/ruby)
		endif(NOT YAF_BINDINGS_RUBY_DIR)

	endif(NOT WIN32)

	add_subdirectory(sketchup-ruby)
	
endif (WITH_YAF_RUBY_BINDINGS)

