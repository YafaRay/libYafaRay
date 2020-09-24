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

#ifndef YAFARAY_MONITOR_H
#define YAFARAY_MONITOR_H

#include "constants.h"
#include "common/thread.h"
#include <string>

BEGIN_YAFARAY
//! Progress bar abstract class with pure virtual members
class LIBYAFARAY_EXPORT ProgressBar
{
	public:
		virtual ~ProgressBar() {}
		//! initialize (or reset) the monitor, give the total number of steps that can occur
		virtual void init(int total_steps = 100) = 0;
		//! update the montior, increment by given number of steps; init has to be called before the first update.
		virtual void update(int steps = 1) = 0;
		//! finish progress bar. It could output some summary, simply disappear from GUI or whatever...
		virtual void done() = 0;
		//! method to pass some informative text to the progress bar in case needed
		virtual void setTag(const char *text) = 0;
		virtual void setTag(std::string text) = 0;
		virtual std::string getTag() const = 0;
		virtual float getPercent() const = 0;
		virtual float getTotalSteps() const = 0;
		std::mutex mutx_;
};

/*! the default console progress bar (implemented in console.cc)
*/
class LIBYAFARAY_EXPORT ConsoleProgressBar : public ProgressBar
{
	public:
		ConsoleProgressBar(int cwidth = 80);
		virtual void init(int total_steps);
		virtual void update(int steps = 1);
		virtual void done();
		virtual void setTag(const char *text) { tag_ = std::string(text); };
		virtual void setTag(std::string text) { tag_ = text; };
		virtual std::string getTag() const { return tag_; }
		virtual float getPercent() const;
		virtual float getTotalSteps() const { return n_steps_; }

	protected:
		int width_, total_bar_len_;
		int last_bar_len_;
		int n_steps_;
		int done_steps_;
		std::string tag_;
};


END_YAFARAY

#endif // YAFARAY_MONITOR_H
