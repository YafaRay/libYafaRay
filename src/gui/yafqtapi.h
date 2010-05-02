#ifndef Y_QTAPI_H
#define Y_QTAPI_H

#include <yafray_constants.h>
#include <interface/yafrayinterface.h>
#include <string>

namespace yafaray
{
	class yafrayInterface_t;
}

struct YAFRAYPLUGIN_EXPORT Settings {
	float* mem;
	bool autoSave;
	bool autoSaveAlpha;
	bool closeAfterFinish;
	std::string fileName;
};

extern "C"
{
	YAFRAYPLUGIN_EXPORT void initGui();
	YAFRAYPLUGIN_EXPORT int createRenderWidget(yafaray::yafrayInterface_t *interf, int xsize, int ysize, int bStartX, int bStartY, Settings settings);
}

#endif // Y_QTAPI_H

