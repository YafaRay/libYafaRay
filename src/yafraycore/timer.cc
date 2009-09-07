#include "yafraycore/timer.h"

#ifdef WIN32
	#include <windows.h>
#else
	#include <sys/time.h>
	#include <unistd.h>
#endif

#include <iostream>

__BEGIN_YAFRAY

timer_t gTimer;

bool timer_t::addEvent(const std::string &name)
{
	if(includes(name)) return false;
	else events[name] = tdata_t();
	return true;
}

bool timer_t::start(const std::string &name)
{
	std::map<std::string, tdata_t>::iterator i=events.find(name);
	if(i==events.end()) return false;
#ifdef WIN32
	i->second.start = clock();
#else
	struct timezone tz;
	gettimeofday(&i->second.tvs, &tz);
#endif
	i->second.started = true;
	return true;
}

bool timer_t::stop(const std::string &name)
{
	std::map<std::string, tdata_t>::iterator i=events.find(name);
	if(i==events.end()) return false;
	if(!(i->second.started))return false;
#ifdef WIN32
	i->second.finish = clock();
#else
	struct timezone tz;
	gettimeofday(&i->second.tvf, &tz);
#endif
	i->second.stopped = true;
	return true;
}

bool timer_t::reset(const std::string &name)
{
	std::map<std::string, tdata_t>::iterator i=events.find(name);
	if (i==events.end()) return false;
	i->second.started = false;
	i->second.stopped = false;
	return true;
}

double timer_t::getTime(const std::string &name)
{
	std::map<std::string, tdata_t>::const_iterator i=events.find(name);
	if (i==events.end()) return -1;
#ifdef WIN32
	else return ((double) (i->second.finish - i->second.start) ) / CLOCKS_PER_SEC;
#else
	else
	{
		const tdata_t &td = i->second;
		return (td.tvf.tv_sec - td.tvs.tv_sec) + double(td.tvf.tv_usec - td.tvs.tv_usec)/1.0e6;
	}
#endif
}


bool timer_t::includes(const std::string &label)const
{
	std::map<std::string, tdata_t>::const_iterator i=events.find(label);
	return (i==events.end()) ? false : true;
}

void timer_t::splitTime(double t, double *secs, int *mins, int *hours, int *days)
{
	int times = (int)t;
	int s = times;
	int d = times / 86400;
	if(days)
	{
		*days = d;
		times -= d*86400;
	}
	int h = times / 3600;
	if(hours)
	{
		*hours = h;
		times -= h*3600;
	}
	int m = times / 60;
	if(mins)
	{
		*mins = m;
		times -= m*60;
	}
	*secs = t - double(s - times);
	//std::cout << "splitTime: " << d << "d " << h << "h " << m << "m " << times << "s (" << *secs << "s)" << std::endl;
}

__END_YAFRAY

//testing:
/*
int main()
{
	timer_t myTimer;
	myTimer.addEvent("waiting");
	myTimer.start("waiting");
	Sleep(1800);
	myTimer.stop("waiting");
	std::cout << "The waiting time was: "<<myTimer.getTime("waiting")<<"s!\n";
}
*/
