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

#include "common/timer.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace yafaray {

bool Timer::addEvent(const std::string &name)
{
	if(includes(name)) return false;
	else events_[name] = Tdata();
	return true;
}

bool Timer::start(const std::string &name)
{
	auto i = events_.find(name);
	if(i == events_.end()) return false;
#ifdef _WIN32
	i->second.start_ = clock();
#else
	struct ::timezone tz;
	gettimeofday(&i->second.tvs_, &tz);
#endif
	i->second.started_ = true;
	return true;
}

bool Timer::stop(const std::string &name)
{
	auto i = events_.find(name);
	if(i == events_.end()) return false;
	if(!(i->second.started_)) return false;
#ifdef _WIN32
	i->second.finish_ = clock();
#else
	struct ::timezone tz;
	gettimeofday(&i->second.tvf_, &tz);
#endif
	i->second.stopped_ = true;
	return true;
}

bool Timer::reset(const std::string &name)
{
	auto i = events_.find(name);
	if(i == events_.end()) return false;
	i->second.started_ = false;
	i->second.stopped_ = false;
	return true;
}

double Timer::getTime(const std::string &name) const
{
	auto i = events_.find(name);
	if(i == events_.end()) return -1;
#ifdef _WIN32
	else return (static_cast<double>(i->second.finish_ - i->second.start_)) / CLOCKS_PER_SEC;
#else
	else
	{
		const Tdata &td = i->second;
		return (td.tvf_.tv_sec - td.tvs_.tv_sec) + double(td.tvf_.tv_usec - td.tvs_.tv_usec) / 1.0e6;
	}
#endif
}

double Timer::getTimeNotStopping(const std::string &name) const
{
	auto i = events_.find(name);
	if(i == events_.end()) return -1;
#ifdef _WIN32
	else return (static_cast<double>(clock() - i->second.start_)) / CLOCKS_PER_SEC;
#else
	else
	{
		timeval now;
		struct ::timezone tz;
		gettimeofday(&now, &tz);
		const Tdata &td = i->second;
		return (now.tv_sec - td.tvs_.tv_sec) + double(now.tv_usec - td.tvs_.tv_usec) / 1.0e6;
	}
#endif
}


bool Timer::includes(const std::string &label) const
{
	const auto i = events_.find(label);
	return (i == events_.end()) ? false : true;
}

void Timer::splitTime(double t, double *secs, int *mins, int *hours, int *days)
{
	int times = static_cast<int>(t);
	const int s = times;
	const int d = times / 86400;
	if(days)
	{
		*days = d;
		times -= d * 86400;
	}
	const int h = times / 3600;
	if(hours)
	{
		*hours = h;
		times -= h * 3600;
	}
	const int m = times / 60;
	if(mins)
	{
		*mins = m;
		times -= m * 60;
	}
	*secs = t - static_cast<double>(s - times);
}

} //namespace yafaray
