
#include <yafraycore/monitor.h>
#include <iostream>
#include <string>
#include <iomanip>

__BEGIN_YAFRAY

#define printBar(progEmpty, progFull, per) \
std::cout << "\r" << setColor(Green) << "Progress: " << \
setColor(Red, true) << "[" << setColor(Green, true) << std::string(progFull, '#') << std::string(progEmpty, ' ') << setColor(Red, true) << "] " << \
setColor() << "(" << setColor(Yellow, true) << per << "%" << setColor() << ")" << std::flush

ConsoleProgressBar_t::ConsoleProgressBar_t(int cwidth): width(cwidth), totalArea(0), doneArea(0)
{
	totalBarLen = width - 22;
}

void ConsoleProgressBar_t::init(int total_area)
{
	totalArea=total_area;
	doneArea = 0;
	lastBarLen = 0;
	printBar(totalBarLen, 0, 0);
}

void ConsoleProgressBar_t::update(int added_area)
{
	doneArea += added_area;
	float progress = (float) std::min(doneArea, totalArea) / (float) totalArea;
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
