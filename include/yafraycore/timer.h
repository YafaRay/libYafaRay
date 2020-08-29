#pragma once

#ifndef YAFARAY_TIMER_H
#define YAFARAY_TIMER_H

#include <yafray_constants.h>
#include <time.h>
#include <string>
#include <map>

#ifndef _WIN32
extern "C" struct timeval;
#include <sys/time.h>
#endif

BEGIN_YAFRAY

class YAFRAYCORE_EXPORT Timer
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
#ifndef WIN32
			timeval tvs_, tvf_;
#endif
			bool started_, stopped_;
		};
		std::map<std::string, Tdata> events_;
};

// global timer object, defined in timer.cc
extern YAFRAYCORE_EXPORT Timer g_timer__;

END_YAFRAY

#endif
