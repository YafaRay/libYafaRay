#pragma once

#ifndef YAFARAY_YAFQTAPI_H
#define YAFARAY_YAFQTAPI_H

#ifdef BUILDING_QTPLUGIN
#define YAFARAY_QT_EXPORT YF_EXPORT
#else
#define YAFARAY_QT_EXPORT YF_IMPORT
#endif
#include <interface/yafrayinterface.h>
#include <string>

namespace yafaray4
{
class Interface;
}

struct YAFARAY_QT_EXPORT Settings
{
	bool auto_save_;
	bool auto_save_alpha_;
	bool close_after_finish_;
	std::string file_name_;
};

extern "C"
{
	YAFARAY_QT_EXPORT void initGui__();
	YAFARAY_QT_EXPORT int createRenderWidget__(yafaray4::Interface *interf, int xsize, int ysize, int b_start_x, int b_start_y, Settings settings);
}

#endif // YAFARAY_YAFQTAPI_H

