add_executable(yafaray_test01 test01.c)
set_target_properties(yafaray_test01 PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_link_libraries(yafaray_test01 PRIVATE libyafaray4)
target_include_directories(yafaray_test01 PRIVATE ../../include ${CMAKE_BINARY_DIR}/src)

install(TARGETS yafaray_test01
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		)
#set_target_properties(yafaray_test01 PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE INSTALL_RPATH "@executable_path/;@executable_path/../../src")
