include_directories(${YAF_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR})

if (EMBED_FONT_QT)
	add_definitions(-DYAFQT_EMBEDED_FONT)
endif (EMBED_FONT_QT)

add_definitions(-DBUILDING_QTPLUGIN)

set(YAF_QT_CC animworking.cc events.cc mywindow.cc qtoutput.cc qtprogress.cc renderwidget.cc worker.cc)
set(YAF_QT_H_4_MOC animworking.h mywindow.h renderwidget.h worker.h)
set(YAF_QT_UI windowbase.ui)

QT4_WRAP_CPP(YAF_QT_MOC_CC ${YAF_QT_H_4_MOC})
QT4_WRAP_UI(YAF_QT_UI_H ${YAF_QT_UI})

add_library(yafarayqt SHARED ${YAF_QT_CC} ${YAF_QT_MOC_CC} ${YAF_QT_UI_H})
target_link_libraries(yafarayqt ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} yafaray_v3_core)
install (TARGETS yafarayqt ${YAF_TARGET_TYPE} DESTINATION ${YAF_LIB_DIR})
