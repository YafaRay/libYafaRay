add_executable(yafaray_test01 test01.c)
if(WIN32 AND NOT MINGW) # MSVC
else(WIN32 AND NOT MINGW)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi") # enable C99 standard
	endif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
endif(WIN32 AND NOT MINGW)
target_include_directories(yafaray_test01 PRIVATE ${YAF_INCLUDE_DIRS})
target_link_libraries(yafaray_test01 PRIVATE libyafaray4)