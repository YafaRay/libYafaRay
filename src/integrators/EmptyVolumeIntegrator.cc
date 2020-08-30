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

#include <yafray_constants.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/integrator.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <integrators/integr_utils.h>
#include <yafraycore/photon.h>
#include <utilities/mcqmc.h>
#include <yafraycore/scr_halton.h>
#include <vector>

BEGIN_YAFRAY

// for removing all participating media effects

class YAFRAYPLUGIN_EXPORT EmptyVolumeIntegrator : public VolumeIntegrator
{
	public:
		EmptyVolumeIntegrator() {}

		virtual Rgba transmittance(RenderState &state, Ray &ray) const
		{
			return Rgb(1.f);
		}

		virtual Rgba integrate(RenderState &state, Ray &ray, ColorPasses &color_passes, int additional_depth /*=0*/) const
		{
			return Rgba(0.f);
		}

		static Integrator *factory(ParamMap &params, RenderEnvironment &render)
		{
			return new EmptyVolumeIntegrator();
		}

};

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("none", EmptyVolumeIntegrator::factory);
	}

}

END_YAFRAY
