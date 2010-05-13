#ifndef Y_QTAPI_H
#define Y_QTAPI_H

#ifdef BUILDING_QTPLUGIN
  #define YAFARAY_QT_EXPORT YF_EXPORT
#else
  #define YAFARAY_QT_EXPORT YF_IMPORT
#endif
#include <interface/yafrayinterface.h>
#include <string>

namespace yafaray
{
	class yafrayInterface_t;
}

struct YAFARAY_QT_EXPORT Settings {
	bool autoSave;
	bool autoSaveAlpha;
	bool closeAfterFinish;
	std::string fileName;
};

extern "C"
{
	YAFARAY_QT_EXPORT void initGui();
	YAFARAY_QT_EXPORT int createRenderWidget(yafaray::yafrayInterface_t *interf, int xsize, int ysize, int bStartX, int bStartY, Settings settings);
}

#endif // Y_QTAPI_H

