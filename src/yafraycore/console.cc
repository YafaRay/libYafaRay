
#include <yafraycore/monitor.h>
#include <iostream>
#include <string>
#include <iomanip>

__BEGIN_YAFRAY

#define printBar(progEmpty, progFull, per) \
std::cout << "\r" << setColor(Green) << "Progress: " << \
setColor(Red, true) << "[" << setColor(Green, true) << std::string(progFull, '#') << std::string(progEmpty, ' ') << setColor(Red, true) << "] " << \
setColor() << "(" << setColor(Yellow, true) << per << "%" << setColor() << ")" << std::flush

ConsoleProgressBar_t::ConsoleProgressBar_t(int cwidth): width(cwidth), nSteps(0), doneSteps(0)
{
	totalBarLen = width - 22;
}

void ConsoleProgressBar_t::init(int totalSteps)
{
	nSteps=totalSteps;
	doneSteps = 0;
	lastBarLen = 0;
	printBar(totalBarLen, 0, 0);
}

void ConsoleProgressBar_t::update(int steps)
{
	doneSteps += steps;
	float progress = (float) std::min(doneSteps, nSteps) / (float) nSteps;
	int barLen = std::min(totalBarLen, (int)(totalBarLen*progress));
	if(!(barLen >= 0)) barLen = 0;
	if(barLen > lastBarLen)
	{
		printBar(totalBarLen-barLen, barLen, (int) (100 * progress));
	}
	lastBarLen = barLen;
}

void ConsoleProgressBar_t::done()
{
	printBar(0, totalBarLen, 100) << yendl;
}

__END_YAFRAY
