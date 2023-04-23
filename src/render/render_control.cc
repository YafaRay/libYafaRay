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

#include "render/render_control.h"
#include "render/progress_bar.h"

namespace yafaray {

void RenderControl::setStarted()
{
	std::lock_guard<std::mutex>lock_guard(mutx_);
	render_in_progress_ = true;
	render_finished_ = false;
	render_resumed_ = false;
	render_canceled_ = false;
}

void RenderControl::setResumed()
{
	std::lock_guard<std::mutex>lock_guard(mutx_);
	render_in_progress_ = true;
	render_finished_ = false;
	render_resumed_ = true;
	render_canceled_ = false;
}

void RenderControl::setFinished()
{
	std::lock_guard<std::mutex>lock_guard(mutx_);
	render_in_progress_ = false;
	render_finished_ = true;
	render_resumed_ = false;
	render_canceled_ = false;
}

void RenderControl::setCanceled()
{
	std::lock_guard<std::mutex>lock_guard(mutx_);
	render_in_progress_ = false;
	render_finished_ = false;
	render_resumed_ = false;
	render_canceled_ = true;
}

bool RenderControl::inProgress() const
{
	return render_in_progress_;
}

bool RenderControl::resumed() const
{
	return render_resumed_;
}

bool RenderControl::finished() const
{
	return render_finished_;
}

bool RenderControl::canceled() const
{
	return render_canceled_;
}

} //namespace yafaray

