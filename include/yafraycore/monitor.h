
#ifndef Y_MONITOR_H
#define Y_MONITOR_H

#include <yafray_config.h>
#include <yafraycore/ccthreads.h>

__BEGIN_YAFRAY
//! Progress bar abstract class with pure virtual members
class YAFRAYCORE_EXPORT progressBar_t
{
	public:
		virtual ~progressBar_t(){}
		//! initialize (or reset) the monitor, give the total number of steps that can occur
		virtual void init(int total_area = 100) = 0;
		//! update the montior, increment by given number of steps; init has to be called before the first update.
		virtual void update(int added_area = 1) = 0;
		//! finish progress bar. It could output some summary, simply disappear from GUI or whatever...
		virtual void done() = 0;
		//! method to pass some informative text to the progress bar in case needed
		virtual void setTag(const char* text) = 0;
		yafthreads::mutex_t mutex;
};

/*! the default console progress bar (implemented in console.cc)
*/
class YAFRAYCORE_EXPORT ConsoleProgressBar_t : public progressBar_t
{
	public:
		ConsoleProgressBar_t(int cwidth = 80);
		virtual void init(int total_area);
		virtual void update(int added_area);
		virtual void done();
		virtual void setTag(const char* text) {};
		
	protected:
		int width, totalBarLen;
		int lastBarLen;
		int totalArea;
		int doneArea;
};


__END_YAFRAY

#endif // Y_MONITOR_H
