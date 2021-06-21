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

#include "common/yafaray_common.h"
#include "public_api/yafaray_c_api.h"
#include <string>
#include <mutex>

BEGIN_YAFARAY

//! Progress bar abstract class with pure virtual members
class ProgressBar
{
	public:
		ProgressBar(yafaray_ProgressBarCallback_t monitor_callback = nullptr, void *callback_user_data = nullptr) : progress_bar_callback_(monitor_callback), callback_user_data_(callback_user_data) { }
		virtual ~ProgressBar() = default;
		//! initialize (or reset) the monitor, give the total number of steps that can occur
		virtual void init(int steps_total, bool colors_enabled) { std::lock_guard<std::mutex> lock_guard(mutx_); steps_total_ = steps_total; steps_done_ = 0; colors_enabled_ = colors_enabled; }
		//! update the montior, increment by given number of steps_increment; init has to be called before the first update.
		virtual void update(int steps_increment = 1);
		//! finish progress bar. It could output some summary, simply disappear from GUI or whatever...
		virtual void done();
		//! method to pass some informative text to the progress bar in case needed
		virtual void setTag(const std::string &text);
		virtual std::string getTag() const { return tag_; }
		virtual float getPercent() const;
		virtual float getTotalSteps() const { return steps_total_; }
		void setAutoDelete(bool value) { std::lock_guard<std::mutex> lock_guard(mutx_); auto_delete_ = value; }
		bool isAutoDeleted() const { return auto_delete_; }
		std::string getName() const { return "ProgressBar"; }

	protected:
		bool auto_delete_ = true; //!< If true, the progress bar is owned by libYafaRay and it is automatically deleted when render finishes. Set it to false when the libYafaRay client owns the progress bar.
		bool colors_enabled_ = true;
		int steps_total_ = 0;
		int steps_done_ = 0;
		std::string tag_;
		std::mutex mutx_;

	private:
		void updateCallback();
		yafaray_ProgressBarCallback_t progress_bar_callback_ = nullptr;
		void *callback_user_data_ = nullptr;
};

/*! the default console progress bar (implemented in console.cc)
*/
class ConsoleProgressBar : public ProgressBar
{
	public:
		ConsoleProgressBar(int cwidth = 80, yafaray_ProgressBarCallback_t monitor_callback = nullptr, void *callback_user_data = nullptr);
		virtual void init(int total_steps, bool colors_enabled) override;
		virtual void update(int steps_increment = 1) override;
		virtual void done() override;

	private:
		static void printBar(bool colors_enabled, int progress_empty, int progress_full, int percent);
		int width_, total_bar_len_;
		int last_bar_len_;
};

END_YAFARAY

#endif // YAFARAY_PROGRESS_BAR_H
