
#include <yafraycore/monitor.h>
#include <core_api/logging.h>
#include <core_api/color_console.h>
#include <utilities/math_utils.h>
//#include <iostream>
//#include <string>
//#include <iomanip>

#ifdef _MSC_VER
#include <algorithm>
#endif

BEGIN_YAFRAY

#define PRINT_BAR(progEmpty, progFull, per) \
std::cout << "\r"; \
if(logger__.getConsoleLogColorsEnabled()) std::cout << SetColor(Green); \
std::cout << "Progress: "; \
if(logger__.getConsoleLogColorsEnabled()) std::cout << SetColor(Red, true); \
std::cout << "["; \
if(logger__.getConsoleLogColorsEnabled()) std::cout << SetColor(Green, true); \
std::cout << std::string(progFull, '#') << std::string(progEmpty, ' '); \
if(logger__.getConsoleLogColorsEnabled()) std::cout << SetColor(Red, true); \
std::cout << "] "; \
if(logger__.getConsoleLogColorsEnabled()) std::cout << SetColor(); \
std::cout << "("; \
if(logger__.getConsoleLogColorsEnabled()) std::cout << SetColor(Yellow, true); \
std::cout << per << "%"; \
if(logger__.getConsoleLogColorsEnabled()) std::cout << SetColor(); \
std::cout << ")" << std::flush

ConsoleProgressBar::ConsoleProgressBar(int cwidth): width_(cwidth), n_steps_(0), done_steps_(0)
{
	total_bar_len_ = width_ - 22;
}

void ConsoleProgressBar::init(int total_steps)
{
	n_steps_ = total_steps;
	done_steps_ = 0;
	last_bar_len_ = 0;
	PRINT_BAR(total_bar_len_, 0, 0);
}

void ConsoleProgressBar::update(int steps)
{
	done_steps_ += steps;
	float progress = (float) std::min(done_steps_, n_steps_) / (float) n_steps_;
	int bar_len = std::min(total_bar_len_, (int)(total_bar_len_ * progress));
	if(!(bar_len >= 0)) bar_len = 0;
	if(bar_len > last_bar_len_)
	{
		PRINT_BAR(total_bar_len_ - bar_len, bar_len, (int)(100 * progress));
	}
	last_bar_len_ = bar_len;
}

void ConsoleProgressBar::done()
{
	PRINT_BAR(0, total_bar_len_, 100) << YENDL;
}

float ConsoleProgressBar::getPercent() const
{
	float progress = 0.f;
	if(n_steps_ != 0) progress = 100.f * roundFloatPrecision__((float) std::min(done_steps_, n_steps_) / (float) n_steps_, 0.01);
	return progress;
}

END_YAFRAY
