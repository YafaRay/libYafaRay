#pragma once

#ifndef Y_MONITOR_H
#define Y_MONITOR_H

#include <yafray_constants.h>
#include <utilities/threadUtils.h>
#include <string>

__BEGIN_YAFRAY
//! Progress bar abstract class with pure virtual members
class YAFRAYCORE_EXPORT progressBar_t
{
	public:
		virtual ~progressBar_t(){}
		//! initialize (or reset) the monitor, give the total number of steps that can occur
		virtual void init(int totalSteps = 100) = 0;
		//! update the montior, increment by given number of steps; init has to be called before the first update.
		virtual void update(int steps = 1) = 0;
		//! finish progress bar. It could output some summary, simply disappear from GUI or whatever...
		virtual void done() = 0;
		//! method to pass some informative text to the progress bar in case needed
		virtual void setTag(const char* text) = 0;
		virtual void setTag(std::string text) = 0;
		virtual std::string getTag() const = 0;
		virtual float getPercent() const = 0; 
		virtual float getTotalSteps() const = 0; 
		std::mutex mutx;
};

/*! the default console progress bar (implemented in console.cc)
*/
class YAFRAYCORE_EXPORT ConsoleProgressBar_t : public progressBar_t
{
	public:
		ConsoleProgressBar_t(int cwidth = 80);
		virtual void init(int totalSteps);
		virtual void update(int steps=1);
		virtual void done();
		virtual void setTag(const char* text) { tag = std::string(text); };
		virtual void setTag(std::string text) { tag = text; };
		virtual std::string getTag() const { return tag; }
		virtual float getPercent() const; 
		virtual float getTotalSteps() const { return nSteps; } 
		
	protected:
		int width, totalBarLen;
		int lastBarLen;
		int nSteps;
		int doneSteps;
		std::string tag;
};


__END_YAFRAY

#endif // Y_MONITOR_H
