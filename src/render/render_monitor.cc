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

#include "render/render_monitor.h"
#include "render/progress_bar.h"

namespace yafaray {

void RenderMonitor::setProgressBar(std::unique_ptr<ProgressBar> progress_bar)
{
	progress_bar_ = std::move(progress_bar);
}

void RenderMonitor::updateProgressBar(int steps_increment)
{
	progress_bar_->update(steps_increment);
}

void RenderMonitor::setProgressBarTag(const std::string &text)
{
	progress_bar_->setTag(text);
}

void RenderMonitor::setProgressBarTag(std::string &&text)
{
	progress_bar_->setTag(std::move(text));
}

void RenderMonitor::setProgressBarAsDone()
{
	progress_bar_->done();
}

std::string RenderMonitor::getProgressBarTag() const
{
	return progress_bar_->getTag();
}

int RenderMonitor::getProgressBarTotalSteps() const
{
	return progress_bar_->getTotalSteps();
}

void RenderMonitor::initProgressBar(int steps_total, bool colors_enabled)
{
	progress_bar_->init(steps_total, colors_enabled);
}

void RenderMonitor::setTotalPasses(int total_passes)
{
	std::lock_guard<std::mutex>lock_guard(mutx_);
	total_passes_ = total_passes;
}

void RenderMonitor::setCurrentPass(int current_pass)
{
	std::lock_guard<std::mutex>lock_guard(mutx_);
	current_pass_ = current_pass;
}

void RenderMonitor::setAaNoiseInfo(const std::string &aa_noise_settings)
{
	aa_noise_info_ = aa_noise_settings;
}

void RenderMonitor::setRenderInfo(const std::string &render_settings)
{
	render_info_ = render_settings;
}

int RenderMonitor::totalPasses() const
{
	return total_passes_;
}

int RenderMonitor::currentPass() const
{
	return current_pass_;
}

float RenderMonitor::currentPassPercent() const
{
	if(progress_bar_) return progress_bar_->getPercent();
	else return 0.f;
}

} //namespace yafaray

