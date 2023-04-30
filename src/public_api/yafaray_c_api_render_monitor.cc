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
#include "render/render_monitor.h"
#include "render/progress_bar.h"

yafaray_RenderMonitor *yafaray_createRenderMonitor(yafaray_ProgressBarCallback monitor_callback, void *callback_data, yafaray_DisplayConsole progress_bar_display_console)
{
	auto render_monitor{new yafaray::RenderMonitor()};
	std::unique_ptr<yafaray::ProgressBar> progress_bar;
	if(progress_bar_display_console == YAFARAY_DISPLAY_CONSOLE_NORMAL) progress_bar = std::make_unique<yafaray::ConsoleProgressBar>(80, monitor_callback, callback_data);
	else progress_bar = std::make_unique<yafaray::ProgressBar>(monitor_callback, callback_data);
	render_monitor->setProgressBar(std::move(progress_bar));
	return reinterpret_cast<yafaray_RenderMonitor *>(render_monitor);
}

void yafaray_destroyRenderMonitor(yafaray_RenderMonitor *render_monitor)
{
	delete reinterpret_cast<yafaray::RenderMonitor *>(render_monitor);
}
