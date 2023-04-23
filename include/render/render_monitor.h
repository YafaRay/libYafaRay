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

#ifndef LIBYAFARAY_RENDER_MONITOR_H
#define LIBYAFARAY_RENDER_MONITOR_H

#include "common/timer.h"
#include <string>
#include <mutex>
#include <memory>

namespace yafaray {

class ProgressBar;

class RenderMonitor final
{
	public:
		void setTotalPasses(int total_passes);
		void setCurrentPass(int current_pass);
		void setRenderInfo(const std::string &render_settings);
		void setAaNoiseInfo(const std::string &aa_noise_settings);
		int totalPasses() const;
		int currentPass() const;
		float currentPassPercent() const;
		std::string getRenderInfo() const { return render_info_; }
		std::string getAaNoiseInfo() const { return aa_noise_info_; }
		void setProgressBar(std::unique_ptr<ProgressBar> progress_bar);
		void updateProgressBar(int steps_increment = 1);
		void setProgressBarTag(const std::string &text);
		void setProgressBarTag(std::string &&text);
		void initProgressBar(int steps_total, bool colors_enabled);
		void setProgressBarAsDone();
		std::string getProgressBarTag() const;
		int getProgressBarTotalSteps() const;
		bool addTimerEvent(const std::string &event) { return timer_.addEvent(event); }
		bool startTimer(const std::string &event) { return timer_.start(event); }
		bool stopTimer(const std::string &event) { return timer_.stop(event); }
		[[nodiscard]] double getTimerTime(const std::string &event) const { return timer_.getTime(event); }
		[[nodiscard]] double getTimerTimeNotStopping(const std::string &event) const { return timer_.getTimeNotStopping(event); }

	private:
		int total_passes_{0};
		int current_pass_{0};
		std::string render_info_;
		std::string aa_noise_info_;
		Timer timer_;
		std::unique_ptr<ProgressBar> progress_bar_;
		std::mutex mutx_;
};

} //namespace yafaray

#endif //LIBYAFARAY_RENDER_MONITOR_H
