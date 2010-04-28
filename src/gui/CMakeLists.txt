include_directories(${YAF_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${QT_INCLUDE_DIRS} ${OPENEXR_INCLUDE_DIRS})

if (EMBED_FONT_QT)
	add_definitions(-DYAFQT_EMBEDED_FONT)
endif (EMBED_FONT_QT)

set(yfgui_SRCS animworking.cc events.cc mywindow.cc qtoutput.cc qtprogress.cc renderwidget.cc worker.cc)
set(yfgui_HDRS animworking.h events.h mywindow.h qtoutput.h qtprogress.h renderwidget.h worker.h)
set(yfgui_UIS windowbase.ui)

QT4_WRAP_CPP(yfgui_MOC_SRCS ${yfgui_HDRS})
QT4_WRAP_UI(yfgui_UI_SRCS ${yfgui_UIS})

# Qt enabling header
include(${QT_USE_FILE})

add_library(yafarayqt SHARED ${yfgui_SRCS} ${yfgui_MOC_SRCS} ${yfgui_UI_SRCS})
target_link_libraries(yafarayqt ${QT_LIBRARIES} yafaraycore ${OPENEXR_LIBRARIES})
install (TARGETS yafarayqt LIBRARY DESTINATION ${YAF_LIB_DIR})
