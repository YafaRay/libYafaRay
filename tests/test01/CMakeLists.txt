add_executable(yafaray_test01 test01.c)
target_include_directories(yafaray_test01 PRIVATE ${YAF_INCLUDE_DIRS})
target_link_libraries(yafaray_test01 PRIVATE libyafaray4)