#pragma once

#ifndef YAFARAY_YAFRAY_CONSTANTS_H
#define YAFARAY_YAFRAY_CONSTANTS_H

#define BEGIN_YAFRAY namespace yafaray4 {
#define END_YAFRAY }

#define PACKAGE "libYafaRay"

#if (__GNUC__ > 3)
#define GCC_HASCLASSVISIBILITY
#endif

// define symbol export and import attributes
#ifdef _WIN32
#define YF_EXPORT __declspec(dllexport)
#define YF_IMPORT __declspec(dllimport)
#else
#ifdef GCC_HASCLASSVISIBILITY
#define YF_EXPORT __attribute__ ((visibility("default")))
#define YF_IMPORT __attribute__ ((visibility("default")))
#else
#define YF_EXPORT
#define YF_IMPORT
#endif
#endif

// automatic macros that switch between import and export, depending on compiler environment
#ifdef BUILDING_YAFRAYCORE
#define YAFRAYCORE_EXPORT YF_EXPORT
#else
#define YAFRAYCORE_EXPORT YF_IMPORT
#endif

#ifdef BUILDING_YAFRAYPLUGIN
#define YAFRAYPLUGIN_EXPORT YF_EXPORT
#else
#define YAFRAYPLUGIN_EXPORT YF_IMPORT
#endif

#endif // YAFARAY_YAFRAY_CONSTANTS_H
