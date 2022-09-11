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

#ifndef YAFARAY_PROGRESS_BAR_H
#define YAFARAY_PROGRESS_BAR_H

#include "public_api/yafaray_c_api.h"
#include <string>
#include <mutex>

namespace yafaray {

//! Progress bar abstract class with pure virtual members
class ProgressBar
{
	public:
		explicit ProgressBar(yafaray_ProgressBarCallback_t monitor_callback = nullptr, void *callback_data = nullptr) : progress_bar_callback_(monitor_callback), callback_data_(callback_data) { }
		virtual ~ProgressBar() = default;
		//! initialize (or reset) the monitor, give the total number of steps that can occur
		virtual void init(int steps_total, bool colors_enabled) { std::lock_guard<std::mutex> lock_guard(mutx_); steps_total_ = steps_total; steps_done_ = 0; colors_enabled_ = colors_enabled; }
		//! update the montior, increment by given number of steps_increment; init has to be called before the first update.
		virtual void update(int steps_increment);
		void update() { update(1); }
		//! finish progress bar. It could output some summary, simply disappear from GUI or whatever...
		virtual void done();
		//! method to pass some informative text to the progress bar in case needed
		virtual void setTag(const std::string &text);
		virtual void setTag(std::string &&text);
		virtual std::string getTag() const { return tag_; }
		virtual float getPercent() const;
		int getTotalSteps() const { return steps_total_; }
		[[nodiscard]] static std::string getName() { return "ProgressBar"; }

	protected:
		bool colors_enabled_ = true;
		int steps_total_ = 0;
		int steps_done_ = 0;
		std::string tag_;
		std::mutex mutx_;

	private:
		void updateCallback();
		yafaray_ProgressBarCallback_t progress_bar_callback_ = nullptr;
		void *callback_data_ = nullptr;
};

/*! the default console progress bar (implemented in console.cc)
*/
class ConsoleProgressBar : public ProgressBar
{
	public:
		explicit ConsoleProgressBar(int cwidth = 80, yafaray_ProgressBarCallback_t monitor_callback = nullptr, void *callback_data = nullptr);
		void init(int total_steps, bool colors_enabled) override;
		void update(int steps_increment) override;
		void done() override;

	private:
		static void printBar(bool colors_enabled, int progress_empty, int progress_full, int percent);
		int width_, total_bar_len_;
		int last_bar_len_;
};

} //namespace yafaray

#endif // YAFARAY_PROGRESS_BAR_H
