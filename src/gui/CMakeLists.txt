include_directories(${YAF_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

if (EMBED_FONT_QT)
	add_definitions(-DYAFQT_EMBEDED_FONT)
endif (EMBED_FONT_QT)

add_definitions(-DBUILDING_QTPLUGIN)

cmake_policy(SET CMP0020 NEW)
cmake_policy(SET CMP0028 NEW)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(YAF_QT_CC animworking.cc events.cc mywindow.cc qtoutput.cc qtprogress.cc renderwidget.cc worker.cc)
set(YAF_QT_H_MOC animworking.h mywindow.h renderwidget.h worker.h)
set(YAF_QT_UI windowbase.ui)

add_library(yafaray4_qt SHARED ${YAF_QT_CC} ${YAF_QT_MOC_CC} ${YAF_QT_UI_H} ${YAF_QT_UI_CC})
target_link_libraries(yafaray4_qt Qt5::Core Qt5::Widgets libyafaray4)
install (TARGETS yafaray4_qt ${YAF_TARGET_TYPE} DESTINATION ${YAF_LIB_DIR})
