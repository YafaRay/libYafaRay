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

/* Callback definitions for the C API */
/* FIXME: Should we care about the function call convention being the same for libYafaRay and its client(s)? */
typedef void (*yafaray_OutputPutpixelCallback_t)(const char *view_name, const char *layer_name, int x, int y, float r, float g, float b, float a, void *callback_user_data);
typedef void (*yafaray_OutputFlushAreaCallback_t)(const char *view_name, int x_0, int y_0, int x_1, int y_1, void *callback_user_data);
typedef void (*yafaray_OutputFlushCallback_t)(const char *view_name, void *callback_user_data);
typedef void (*yafaray_ProgressBarCallback_t)(int steps_total, int steps_done, const char *tag, void *callback_user_data);
enum yafaray_LogLevel { YAFARAY_LOG_LEVEL_MUTE = 0, YAFARAY_LOG_LEVEL_ERROR, YAFARAY_LOG_LEVEL_WARNING, YAFARAY_LOG_LEVEL_PARAMS, YAFARAY_LOG_LEVEL_INFO, YAFARAY_LOG_LEVEL_VERBOSE, YAFARAY_LOG_LEVEL_DEBUG };
typedef enum yafaray_LogLevel yafaray_LogLevel_t;
typedef void (*yafaray_LoggerCallback_t)(yafaray_LogLevel_t log_level, long datetime, const char *time_of_day, const char *description, void *callback_user_data);
typedef enum { YAFARAY_DISPLAY_CONSOLE_HIDDEN, YAFARAY_DISPLAY_CONSOLE_NORMAL } yafaray_DisplayConsole_t;

#endif /* YAFARAY_CONF_H */
