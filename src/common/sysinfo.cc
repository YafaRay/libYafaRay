/****************************************************************************
 *      sysinfo.cc: runtime system information
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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
 */

//Threads detection code moved here from scene.cc

#include "common/sysinfo.h"
//#include "yafaray_config.h"

#ifdef __APPLE__
#include <sys/sysctl.h>
#elif _WIN32
#include <windows.h>
#endif

#if !defined(_WIN32) || defined(__MINGW32__)
#include <unistd.h>
#endif

BEGIN_YAFARAY

int SysInfo::getNumSystemThreads() const
{
	int nthreads = 1;

#ifdef WIN32
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	nthreads = static_cast<int>(info.dwNumberOfProcessors);
#else
#	ifdef __APPLE__
	int mib[2];
	size_t len;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	len = sizeof(int);
	sysctl(mib, 2, &nthreads, &len, nullptr, 0);
#	elif defined(__sgi)
	nthreads = sysconf(_SC_NPROC_ONLN);
#	else
	nthreads = (int)sysconf(_SC_NPROCESSORS_ONLN);
#	endif
#endif

	return nthreads;
}

END_YAFARAY
