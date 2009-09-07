#ifndef Y_TIMER_H
#define Y_TIMER_H

#include <yafray_config.h>

#include <time.h>
#include <string>
#include <map>

#ifndef WIN32
extern "C" struct timeval;
#include <sys/time.h>
#endif

__BEGIN_YAFRAY
// #define YAFRAYCORE_EXPORT


class YAFRAYCORE_EXPORT timer_t
{
	public:
		bool addEvent(const std::string &name);
		bool start(const std::string &name);
		bool stop(const std::string &name);
		bool reset(const std::string &name);
		double getTime(const std::string &name);
		
		static void splitTime(double t, double *secs, int *mins=0, int *hours=0, int *days=0);
	
	protected:
		bool includes(const std::string &label)const;
		
		struct tdata_t
		{
			tdata_t():started(false), stopped(false) {};
			clock_t start, finish;
			#ifndef WIN32
			timeval tvs, tvf;
			#endif
			bool started, stopped;
		};
		std::map<std::string, tdata_t> events;
};

// global timer object, defined in timer.cc
extern YAFRAYCORE_EXPORT timer_t gTimer;

__END_YAFRAY

#endif
