
#ifndef Y_MONITOR_H
#define Y_MONITOR_H

#include <yafray_config.h>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT progressBar_t
{
	public:
		virtual ~progressBar_t(){}
		//! initialize (or reset) the monitor, give the total number of steps that can occur
		virtual void init(int totalSteps)=0;
		//! update the montior, increment by given number of steps; init has to be called before the first update.
		virtual void update(int steps=1)=0;
		//! finish progress bar. It could output some summary, simply disappear from GUI or whatever...
		virtual void done()=0;
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
	protected:
		int width, totalBarLen;
		int lastBarLen;
		int nSteps;
		int doneSteps;
};


__END_YAFRAY

#endif // Y_MONITOR_H
