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

#ifndef YAFARAY_TIMER_H
#define YAFARAY_TIMER_H

#include "yafaray_common.h"
#include <time.h>
#include <string>
#include <map>

#ifndef _WIN32
extern "C" struct timeval;
#include <sys/time.h>
#endif

BEGIN_YAFARAY

class Timer
{
	public:
		bool addEvent(const std::string &name);
		bool start(const std::string &name);
		bool stop(const std::string &name);
		bool reset(const std::string &name);
		double getTime(const std::string &name);
		double getTimeNotStopping(const std::string &name);

		static void splitTime(double t, double *secs, int *mins = nullptr, int *hours = nullptr, int *days = nullptr);

	protected:
		bool includes(const std::string &label) const;

		struct Tdata
		{
			Tdata(): started_(false), stopped_(false) {};
			clock_t start_, finish_;
#ifndef _WIN32
			timeval tvs_, tvf_;
#endif
			bool started_, stopped_;
		};
		std::map<std::string, Tdata> events_;
};

// global timer object, defined in timer.cc
extern Timer g_timer_global;

END_YAFARAY

#endif
