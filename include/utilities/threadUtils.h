#pragma once
/****************************************************************************
 *
 * 		threadUtils.h: Thread Utilities api
 *      This is part of the yafray package
 *      Copyright (C) 2016  David Bluecame
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef Y_THREADUTILS_H
#define Y_THREADUTILS_H

#if defined(_WIN32) && defined(__MINGW32__) && defined(HAVE_MINGW_STD_THREADS) //If compiling for Windows with MinGW, the standard C++11 thread management is very slow, causing a performance drop. I'll be using the alternative (much faster) implementation from https://github.com/meganz/mingw-std-threads
	#undef _GLIBCXX_HAS_GTHREADS
	#include <mingw-std-threads/mingw.thread.h>
	#include <mutex>
	#include <mingw-std-threads/mingw.mutex.h>
	#include <mingw-std-threads/mingw.condition_variable.h>
#else
	#include <thread>
	#include <mutex>
	#include <condition_variable>
#endif


#endif
