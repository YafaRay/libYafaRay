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

#ifndef LIBYAFARAY_RENDER_CONTROL_H
#define LIBYAFARAY_RENDER_CONTROL_H

#include "common/timer.h"
#include <string>
#include <mutex>
#include <memory>

namespace yafaray {

class ProgressBar;

class RenderControl final
{
	public:
		void setStarted();
		void setResumed();
		void setFinished();
		void setCanceled();
		bool inProgress() const;
		bool resumed() const;
		bool finished() const;
		bool canceled() const;
		bool addTimerEvent(const std::string &event) { return timer_.addEvent(event); }
		bool startTimer(const std::string &event) { return timer_.start(event); }
		bool stopTimer(const std::string &event) { return timer_.stop(event); }
		[[nodiscard]] double getTimerTime(const std::string &event) const { return timer_.getTime(event); }
		[[nodiscard]] double getTimerTimeNotStopping(const std::string &event) const { return timer_.getTimeNotStopping(event); }

	private:
		bool render_in_progress_ = false;
		bool render_finished_ = false;
		bool render_resumed_ = false;
		bool render_canceled_ = false;
		Timer timer_;
		std::mutex mutx_;
};

} //namespace yafaray

#endif //LIBYAFARAY_RENDER_CONTROL_H
