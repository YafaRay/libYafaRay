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

#include "yafaray_conf.h"
#include "common/thread.h"
#include <string>

BEGIN_YAFARAY

//! Progress bar abstract class with pure virtual members
class ProgressBar
{
	public:
		virtual ~ProgressBar() = default;
		//! initialize (or reset) the monitor, give the total number of steps that can occur
		virtual void init(int steps_total, bool colors_enabled) { std::lock_guard<std::mutex> lock_guard(mutx_); steps_total_ = steps_total; steps_done_ = 0; colors_enabled_ = colors_enabled; }
		//! update the montior, increment by given number of steps_increment; init has to be called before the first update.
		virtual void update(int steps_increment = 1) { std::lock_guard<std::mutex> lock_guard(mutx_); steps_done_ += steps_increment; }
		//! finish progress bar. It could output some summary, simply disappear from GUI or whatever...
		virtual void done() { std::lock_guard<std::mutex> lock_guard(mutx_); steps_done_ = steps_total_; }
		//! method to pass some informative text to the progress bar in case needed
		virtual void setTag(const std::string &text) { std::lock_guard<std::mutex> lock_guard(mutx_); tag_ = text; }
		virtual std::string getTag() const { return tag_; }
		virtual float getPercent() const;
		virtual float getTotalSteps() const { return steps_total_; }
		void setAutoDelete(bool value) { std::lock_guard<std::mutex> lock_guard(mutx_); auto_delete_ = value; }
		bool isAutoDeleted() const { return auto_delete_; }
		std::string getName() const { return "ProgressBar"; }
		static void printBar(bool colors_enabled, int progress_empty, int progress_full, int percent);

	protected:
		bool auto_delete_ = true; //!< If true, the progress bar is owned by libYafaRay and it is automatically deleted when render finishes. Set it to false when the libYafaRay client owns the progress bar.
		bool colors_enabled_ = true;
		int steps_total_ = 0;
		int steps_done_ = 0;
		std::string tag_;
		std::mutex mutx_;
};

/*! the default console progress bar (implemented in console.cc)
*/
class ConsoleProgressBar final : public ProgressBar
{
	public:
		ConsoleProgressBar(int cwidth = 80);
		virtual void init(int total_steps, bool colors_enabled) override;
		virtual void update(int steps_increment = 1) override;
		virtual void done() override;

	private:
		int width_, total_bar_len_;
		int last_bar_len_;
};

class CallbackProgressBar final : public ProgressBar
{
	public:
		CallbackProgressBar(void *callback_user_data, yafaray4_MonitorCallback_t monitor_callback);
		virtual void init(int total_steps, bool colors_enabled) override;
		virtual void update(int steps_increment = 1) override;
		virtual void done() override;
		virtual void setTag(const std::string &text) override;

	private:
		void updateCallback() { monitor_callback_(steps_total_, steps_done_, tag_.c_str(), callback_user_data_); }
		void *callback_user_data_ = nullptr;
		yafaray4_MonitorCallback_t monitor_callback_ = nullptr;
};

END_YAFARAY

#endif // YAFARAY_PROGRESS_BAR_H
