
#include <yafraycore/monitor.h>
#include <iostream>
#include <string>
#include <iomanip>

__BEGIN_YAFRAY


ConsoleProgressBar_t::ConsoleProgressBar_t(int cwidth): width(cwidth), nSteps(0), doneSteps(0)
{
	totalBarLen = width - 16;
}

void ConsoleProgressBar_t::init(int totalSteps)
{
	nSteps=totalSteps;
	doneSteps = 0;
	lastBarLen = 0;
	std::cout << "\r[" << std::string(totalBarLen, ' ') << "] (0%)" << std::flush;
}

void ConsoleProgressBar_t::update(int steps)
{
	doneSteps += steps;
	float progress = float(doneSteps)/(float)nSteps;
	int barLen = std::min(totalBarLen, (int)(totalBarLen*progress));
	if(!(barLen >= 0)) barLen = 0;
	if(barLen > lastBarLen)
	{
		std::cout << "\r[" << std::string(barLen, '#') << std::string(totalBarLen-barLen, ' ') << "] (" 
				  << int(100*progress) << "%)" << std::flush;
	}
	lastBarLen = barLen;
}

void ConsoleProgressBar_t::done()
{
	std::cout << "\r[" << std::string(totalBarLen, '#') << "] (done)\n" << std::flush;
}

__END_YAFRAY
