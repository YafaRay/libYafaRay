#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef YAFARAY_CONF_H
#define YAFARAY_CONF_H

#define BEGIN_YAFARAY namespace yafaray4 {
#define END_YAFARAY }

/* define symbol export and import attributes */
#ifdef _WIN32
#define YF_EXPORT __declspec(dllexport)
#define YF_IMPORT __declspec(dllimport)
#else /* _WIN32 */
#define YF_EXPORT __attribute__ ((visibility("default")))
#define YF_IMPORT
#endif /* _WIN32 */

/* automatic macros that switch between import and export, depending on compiler environment */
#ifdef BUILDING_LIBYAFARAY
#define LIBYAFARAY_EXPORT YF_EXPORT
#else /* BUILDING_LIBYAFARAY */
#define LIBYAFARAY_EXPORT YF_IMPORT
#endif /* BUILDING_LIBYAFARAY */

/* Callback definitions for the C API */
/* FIXME: Should we care about the function call convention being the same for libYafaRay and its client(s)? */
typedef void (*yafaray4_OutputPutpixelCallback_t)(const char *view_name, const char *layer_name, int x, int y, float r, float g, float b, float a, void *callback_user_data);
typedef void (*yafaray4_OutputFlushAreaCallback_t)(const char *view_name, int x_0, int y_0, int x_1, int y_1, void *callback_user_data);
typedef void (*yafaray4_OutputFlushCallback_t)(const char *view_name, void *callback_user_data);
typedef void (*yafaray4_MonitorCallback_t)(int steps_total, int steps_done, const char *tag, void *callback_user_data);
enum yafaray4_LogLevel { YAFARAY_LOG_LEVEL_MUTE = 0, YAFARAY_LOG_LEVEL_ERROR, YAFARAY_LOG_LEVEL_WARNING, YAFARAY_LOG_LEVEL_PARAMS, YAFARAY_LOG_LEVEL_INFO, YAFARAY_LOG_LEVEL_VERBOSE, YAFARAY_LOG_LEVEL_DEBUG };
typedef enum yafaray4_LogLevel yafaray4_LogLevel_t;
typedef void (*yafaray4_LoggerCallback_t)(yafaray4_LogLevel_t log_level, long datetime, const char *time_of_day, const char *description, void *callback_user_data);

#endif /* YAFARAY_CONF_H */
