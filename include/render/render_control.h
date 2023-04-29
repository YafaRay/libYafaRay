#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
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

#ifndef LIBYAFARAY_RENDER_CONTROL_H
#define LIBYAFARAY_RENDER_CONTROL_H

#include <atomic>

namespace yafaray {

class RenderControl final
{
	public:
		void setStarted() { flags_ = InProgress; }
		void setResumed() { flags_ = InProgress | Resumed; }
		void setFinished() { flags_ = Finished; }
		void setProgressive() { flags_ |= Progressive; }
		void setCanceled() { flags_ = Canceled; }
		bool inProgress() const { return flags_ & InProgress; }
		bool resumed() const { return flags_ & Resumed; }
		bool finished() const { return flags_ & Finished; }
		bool progressive() const { return flags_ & Progressive; }
		bool canceled() const { return flags_ & Canceled; }

	private:
		enum {
			InProgress = 1 << 0,
			Finished = 1 << 1,
			Resumed = 1 << 2,
			Progressive = 1 << 3,
			Canceled = 1 << 4,
		};
		std::atomic<unsigned char> flags_{0};
};

} //namespace yafaray

#endif //LIBYAFARAY_RENDER_CONTROL_H
