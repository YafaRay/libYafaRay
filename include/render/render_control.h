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

#ifndef YAFARAY_RENDER_CONTROL_H
#define YAFARAY_RENDER_CONTROL_H

#include "constants.h"
#include "common/thread.h"

BEGIN_YAFARAY

class RenderControl final
{
	public:
		void setStarted();
		void setResumed();
		void setFinished();
		void setAborted();
		void setTotalPasses(int total_passes);
		void setCurrentPass(int current_pass);
		void setCurrentPassPercent(float current_pass_percent);
		void setRenderInfo(const std::string &render_settings);
		void setAaNoiseInfo(const std::string &aa_noise_settings);

		bool inProgress() const;
		bool resumed() const;
		bool finished() const;
		bool aborted() const;
		int totalPasses() const;
		int currentPass() const;
		float currentPassPercent() const;
		std::string getRenderInfo() const { return render_info_; }
		std::string getAaNoiseInfo() const { return aa_noise_info_; }

	private:
		bool render_in_progress_ = false;
		bool render_finished_ = false;
		bool render_resumed_ = false;
		bool render_aborted_ = false;
		int total_passes_ = 0;
		int current_pass_ = 0;
		float current_pass_percent_ = 0.f;
		std::string render_info_;
		std::string aa_noise_info_;

		std::mutex mutx_;
};

END_YAFARAY

#endif //YAFARAY_RENDER_CONTROL_H
