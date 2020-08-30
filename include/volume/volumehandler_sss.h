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

#ifndef YAFARAY_VOLUMEHANDLER_SSS_H
#define YAFARAY_VOLUMEHANDLER_SSS_H

#include "volume/volumehandler_beer.h"

BEGIN_YAFARAY

class ParamMap;
class RenderEnvironment;

class SssVolumeHandler final : public BeerVolumeHandler
{
	public:
		static VolumeHandler *factory(const ParamMap &params, RenderEnvironment &env);

	private:
		SssVolumeHandler(const Rgb &a_col, const Rgb &s_col, double dist);
		//virtual bool transmittance(const renderState_t &state, const ray_t &ray, Rgb &col) const { return false; };
		virtual bool scatter(const RenderState &state, const Ray &ray, Ray &s_ray, PSample &s) const override;

		float dist_s_;
		Rgb scatter_col_;
};

END_YAFARAY

#endif // YAFARAY_VOLUMEHANDLER_SSS_H