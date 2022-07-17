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

void RenderControl::setProgressBar(std::unique_ptr<ProgressBar> progress_bar)
{
	progress_bar_ = std::move(progress_bar);
}

void RenderControl::updateProgressBar(int steps_increment)
{
	progress_bar_->update(steps_increment);
}

void RenderControl::setProgressBarTag(const std::string &text)
{
	progress_bar_->setTag(text);
}

void RenderControl::setProgressBarTag(std::string &&text)
{
	progress_bar_->setTag(std::move(text));
}

void RenderControl::setProgressBarAsDone()
{
	progress_bar_->done();
}

std::string RenderControl::getProgressBarTag() const
{
	return progress_bar_->getTag();
}

int RenderControl::getProgressBarTotalSteps() const
{
	return progress_bar_->getTotalSteps();
}

void RenderControl::initProgressBar(int steps_total, bool colors_enabled)
{
	progress_bar_->init(steps_total, colors_enabled);
}

void RenderControl::setStarted()
{
	std::lock_guard<std::mutex>lock_guard(mutx_);
	render_in_progress_ = true;
	render_finished_ = false;
	render_resumed_ = false;
	render_canceled_ = false;
	total_passes_ = 0;
	current_pass_ = 0;
	current_pass_percent_ = 0.f;
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

void RenderControl::setTotalPasses(int total_passes)
{
	std::lock_guard<std::mutex>lock_guard(mutx_);
	total_passes_ = total_passes;
}

void RenderControl::setCurrentPass(int current_pass)
{
	std::lock_guard<std::mutex>lock_guard(mutx_);
	current_pass_ = current_pass;
}

void RenderControl::setAaNoiseInfo(const std::string &aa_noise_settings)
{
	aa_noise_info_ = aa_noise_settings;
}

void RenderControl::setRenderInfo(const std::string &render_settings)
{
	render_info_ = render_settings;
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

int RenderControl::totalPasses() const
{
	return total_passes_;
}

int RenderControl::currentPass() const
{
	return current_pass_;
}

float RenderControl::currentPassPercent() const
{
	return current_pass_percent_;
}

} //namespace yafaray

