#ifndef Y_CCTHREADS_H
#define Y_CCTHREADS_H

#include<iostream>

#include<yafray_config.h>

#include<errno.h>

#if HAVE_PTHREAD
	#include<pthread.h>
	#define USING_THREADS
#elif defined(WIN32)
	#define USING_THREADS
	#define WIN32_THREADS
#endif

namespace yafthreads {

/*! The try to provide a platform independant mutex, as a matter of fact
	it is simply a pthread wrapper now...
*/
class YAFRAYCORE_EXPORT mutex_t
{
	public:
		mutex_t();
		void lock();
		void unlock();
		~mutex_t();
	protected:
		mutex_t(const mutex_t &m);
		mutex_t & operator = (const mutex_t &m);
#if HAVE_PTHREAD
		pthread_mutex_t m;
#elif defined( WIN32_THREADS )
		HANDLE winMutex;
#endif
};

/*! The try to provide a platform independant read-shared write-exclusive lock,
	 as a matter of fact it is simply a pthread wrapper now...
*/
class YAFRAYCORE_EXPORT rwlock_t
{
	public:
		rwlock_t();
		void readLock();
		void writeLock();
		void unlock();
		~rwlock_t();
	protected:
		rwlock_t(const rwlock_t &l);
		rwlock_t & operator = (const rwlock_t &l);
#if HAVE_PTHREAD
		pthread_rwlock_t l;
#endif
};


/*! The try to provide a platform independant codition object, as a matter of fact
	it is simply a pthread wrapper now...
	It is mutex and condition variable in one!
	Usage: 	waiting thread:		lock(); ...initialize conditions to be met...; wait();
			signalling thread:	lock(); ...check if you want to signal...; [signal();] unlock();
*/
class YAFRAYCORE_EXPORT conditionVar_t
{
	public:
		conditionVar_t();
		~conditionVar_t();
		void lock();
		void unlock();
		void signal();
		void wait();
	protected:
		conditionVar_t(const conditionVar_t &m);
		conditionVar_t & operator = (const conditionVar_t &m);
#if HAVE_PTHREAD
		pthread_mutex_t m;
		pthread_cond_t c;
#elif defined( WIN32_THREADS )
		HANDLE condHandle;
		HANDLE winMutex;
#endif
};
                                                                                                                
class YAFRAYCORE_EXPORT thread_t
{
#if HAVE_PTHREAD
	friend void * wrapper(void *data);
#elif defined( WIN32_THREADS )
	friend DWORD WINAPI wrapper (void *data);
#endif
	public:
		thread_t() {running=false;};
		virtual ~thread_t();
		virtual void body()=0;
		void run();
		void wait();
		bool isRunning()const {return running;};
#if HAVE_PTHREAD
		pthread_t getPid() {return id;};
#elif defined( WIN32_THREADS )
		DWORD getPid() {return id;};
#endif
	protected:
		bool running;
		mutex_t lock;
#if HAVE_PTHREAD
		pthread_t id;
		pthread_attr_t attr;
#elif defined( WIN32_THREADS )
		DWORD id;
		HANDLE winThread;
#endif
};

} // yafthreads

#endif
