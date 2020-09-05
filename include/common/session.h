#pragma once
/****************************************************************************
 *      session.h: YafaRay Session control
 *      This is part of the libYafaRay package
 *      Copyright (C) 2016 David Bluecame
 *      Session control and persistent objects between renders
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

#ifndef YAFARAY_SESSION_H
#define YAFARAY_SESSION_H

#include "constants.h"
#include "utility/util_thread.h"

BEGIN_YAFARAY

class PhotonMap;

class LIBYAFARAY_EXPORT Session
{
	public:
		Session();
		Session(const Session &);	//customizing copy constructor so we can use a std::mutex as a class member (not copiable)

		~Session();

		void setStatusRenderStarted();
		void setStatusRenderResumed();
		void setStatusRenderFinished();
		void setStatusRenderAborted();
		void setStatusTotalPasses(int total_passes);
		void setStatusCurrentPass(int current_pass);
		void setStatusCurrentPassPercent(float current_pass_percent);
		void setInteractive(bool interactive);
		void setPathYafaRayXml(std::string path);
		void setPathImageOutput(std::string path);
		void setDifferentialRaysEnabled(bool value) { ray_differentials_enabled_ = value; }

		bool renderInProgress();
		bool renderResumed();
		bool renderFinished();
		bool renderAborted();
		bool getDifferentialRaysEnabled() const { return ray_differentials_enabled_; }

		int totalPasses();
		int currentPass();
		float currentPassPercent();
		bool isInteractive();
		std::string getPathYafaRayXml();
		std::string getPathImageOutput();

		PhotonMap *caustic_map_ = nullptr;
		PhotonMap *diffuse_map_ = nullptr;
		PhotonMap *radiance_map_ = nullptr;

		std::mutex mutx_;

	protected:
		bool render_in_progress_ = false;
		bool render_finished_ = false;
		bool render_resumed_ = false;
		bool render_aborted_ = false;
		bool ray_differentials_enabled_ = false;  //!< By default, disable ray differential calculations. Only if at least one texture uses them, then enable differentials. This should avoid the (many) extra calculations when they are not necessary.
		int total_passes_ = 0;
		int current_pass_ = 0;
		float current_pass_percent_ = 0.f;
		bool interactive_ = false;
		std::string path_yafa_ray_xml_;
		std::string path_image_output_;
};

extern LIBYAFARAY_EXPORT Session session__;

END_YAFARAY

#endif
