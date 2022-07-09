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

#ifndef YAFARAY_CAMERA_ARCHITECT_H
#define YAFARAY_CAMERA_ARCHITECT_H

#include "camera/camera_perspective.h"

namespace yafaray {

class ParamMap;
class Scene;

class ArchitectCamera final : public PerspectiveCamera
{
	public:
		static const Camera * factory(Logger &logger, const Scene &scene, const std::string &name, const ParamMap &params);

	private:
		ArchitectCamera(Logger &logger, const Point3f &pos, const Point3f &look, const Point3f &up,
						int resx, int resy, float aspect = 1,
						float df = 1, float ap = 0, float dofd = 0, BokehType bt = BokehType::BkDisk1, BkhBiasType bbt = BkhBiasType::BbNone, float bro = 0,
						float near_clip_distance = 0.0f, float far_clip_distance = 1e6f);
		void setAxis(const Vec3f &vx, const Vec3f &vy, const Vec3f &vz) override;
		Point3f screenproject(const Point3f &p) const override;
};

} //namespace yafaray

#endif // YAFARAY_CAMERA_ARCHITECT_H
