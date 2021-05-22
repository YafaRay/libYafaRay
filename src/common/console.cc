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

#include "render/monitor.h"
#include "common/logger.h"
#include "color/color_console.h"
#include "math/math.h"
//#include <iostream>
//#include <string>
//#include <iomanip>

#ifdef _MSC_VER
#include <algorithm>
#endif

BEGIN_YAFARAY

#define PRINT_BAR(progEmpty, progFull, per) \
std::cout << "\r"; \
if(logger_global.getConsoleLogColorsEnabled()) std::cout << ConsoleColor(ConsoleColor::Green); \
std::cout << "Progress: "; \
if(logger_global.getConsoleLogColorsEnabled()) std::cout << ConsoleColor(ConsoleColor::Red, true); \
std::cout << "["; \
if(logger_global.getConsoleLogColorsEnabled()) std::cout << ConsoleColor(ConsoleColor::Green, true); \
std::cout << std::string(progFull, '#') << std::string(progEmpty, ' '); \
if(logger_global.getConsoleLogColorsEnabled()) std::cout << ConsoleColor(ConsoleColor::Red, true); \
std::cout << "] "; \
if(logger_global.getConsoleLogColorsEnabled()) std::cout << ConsoleColor(); \
std::cout << "("; \
if(logger_global.getConsoleLogColorsEnabled()) std::cout << ConsoleColor(ConsoleColor::Yellow, true); \
std::cout << per << "%"; \
if(logger_global.getConsoleLogColorsEnabled()) std::cout << ConsoleColor(); \
std::cout << ")" << std::flush

float ProgressBar::getPercent() const
{
	float progress = 0.f;
	if(steps_total_ != 0) progress = 100.f * math::roundFloatPrecision((float) std::min(steps_done_, steps_total_) / (float) steps_total_, 0.01);
	return progress;
}

ConsoleProgressBar::ConsoleProgressBar(int cwidth): width_(cwidth)
{
	total_bar_len_ = width_ - 22;
}

void ConsoleProgressBar::init(int total_steps)
{
	ProgressBar::init(total_steps);
	last_bar_len_ = 0;
	PRINT_BAR(total_bar_len_, 0, 0);
}

void ConsoleProgressBar::update(int steps_increment)
{
	ProgressBar::update(steps_increment);
	const float progress = (float) std::min(steps_done_, steps_total_) / (float) steps_total_;
	int bar_len = std::min(total_bar_len_, (int)(total_bar_len_ * progress));
	if(!(bar_len >= 0)) bar_len = 0;
	if(bar_len > last_bar_len_)
	{
		PRINT_BAR(total_bar_len_ - bar_len, bar_len, (int)(100 * progress));
	}
	last_bar_len_ = bar_len;
}

void ConsoleProgressBar::done()
{
	ProgressBar::done();
	PRINT_BAR(0, total_bar_len_, 100) << YENDL;
}

CallbackProgressBar::CallbackProgressBar(void *callback_user_data, MonitorCallback_t monitor_callback): callback_user_data_(callback_user_data), monitor_callback_(monitor_callback)
{
}

void CallbackProgressBar::init(int total_steps)
{
	ProgressBar::init(total_steps);
	updateCallback();
}

void CallbackProgressBar::update(int steps_increment)
{
	ProgressBar::update(steps_increment);
	updateCallback();
}

void CallbackProgressBar::setTag(const std::string &text)
{
	ProgressBar::setTag(text);
	updateCallback();
}

void CallbackProgressBar::done()
{
	ProgressBar::done();
	updateCallback();
}


END_YAFARAY
