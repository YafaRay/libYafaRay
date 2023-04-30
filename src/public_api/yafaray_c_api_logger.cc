/****************************************************************************
 *   This is part of the libYafaRay package
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "public_api/yafaray_c_api.h"
#include "common/logger.h"

yafaray_Logger *yafaray_createLogger(yafaray_LoggerCallback logger_callback, void *callback_data, yafaray_DisplayConsole display_console)
{
	auto logger{new yafaray::Logger(logger_callback, callback_data, display_console)};
	return reinterpret_cast<yafaray_Logger *>(logger);
}

void yafaray_setLoggerCallbacks(yafaray_Logger *logger, yafaray_LoggerCallback logger_callback, void *callback_data)
{
	if(!logger) return;
	reinterpret_cast<yafaray::Logger *>(logger)->setCallback(logger_callback, callback_data);
}

void yafaray_destroyLogger(yafaray_Logger *logger)
{
	delete reinterpret_cast<yafaray::Logger *>(logger);
}

void yafaray_enablePrintDateTime(yafaray_Logger *logger, yafaray_Bool value)
{
	if(!logger) return;
	reinterpret_cast<yafaray::Logger *>(logger)->enablePrintDateTime(value);
}

void yafaray_setConsoleVerbosityLevel(yafaray_Logger *logger, yafaray_LogLevel log_level)
{
	if(!logger) return;
	reinterpret_cast<yafaray::Logger *>(logger)->setConsoleMasterVerbosity(log_level);
}

void yafaray_setLogVerbosityLevel(yafaray_Logger *logger, yafaray_LogLevel log_level)
{
	if(!logger) return;
	reinterpret_cast<yafaray::Logger *>(logger)->setLogMasterVerbosity(log_level);
}

/*! Console Printing wrappers to report in color with yafaray's own console coloring */
void yafaray_printDebug(yafaray_Logger *logger, const char *msg)
{
	if(!logger || !msg) return;
	reinterpret_cast<yafaray::Logger *>(logger)->logDebug(msg);
}

void yafaray_printVerbose(yafaray_Logger *logger, const char *msg)
{
	if(!logger || !msg) return;
	reinterpret_cast<yafaray::Logger *>(logger)->logVerbose(msg);
}

void yafaray_printInfo(yafaray_Logger *logger, const char *msg)
{
	if(!logger || !msg) return;
	reinterpret_cast<yafaray::Logger *>(logger)->logInfo(msg);
}

void yafaray_printParams(yafaray_Logger *logger, const char *msg)
{
	if(!logger || !msg) return;
	reinterpret_cast<yafaray::Logger *>(logger)->logParams(msg);
}

void yafaray_printWarning(yafaray_Logger *logger, const char *msg)
{
	if(!logger || !msg) return;
	reinterpret_cast<yafaray::Logger *>(logger)->logWarning(msg);
}

void yafaray_printError(yafaray_Logger *logger, const char *msg)
{
	if(!logger || !msg) return;
	reinterpret_cast<yafaray::Logger *>(logger)->logError(msg);
}

yafaray_LogLevel yafaray_logLevelFromString(const char *log_level_string)
{
	if(!log_level_string) return YAFARAY_LOG_LEVEL_INFO;
	return static_cast<yafaray_LogLevel>(yafaray::Logger::vlevelFromString(log_level_string));
}

void yafaray_setConsoleLogColorsEnabled(yafaray_Logger *logger, yafaray_Bool colors_enabled)
{
	if(!logger) return;
	reinterpret_cast<yafaray::Logger *>(logger)->setConsoleLogColorsEnabled(colors_enabled);
}