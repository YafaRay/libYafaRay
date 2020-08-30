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
#include <core_api/logging.h>
#include <core_api/session.h>
#include <core_api/volume.h>
#include <core_api/environment.h>
#include <core_api/scene.h>
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

class YAFRAYPLUGIN_EXPORT EmissionIntegrator : public VolumeIntegrator
{
	public:
		EmissionIntegrator() {}

		// optical thickness, absorption, attenuation, extinction
		virtual Rgba transmittance(RenderState &state, Ray &ray) const
		{
			Rgba result(1.f);

			std::vector<VolumeRegion *> list_vr = scene_->getVolumes();


			for(unsigned int i = 0; i < list_vr.size(); i++)
			{
				result *= list_vr.at(i)->tau(ray, 0, 0);
			}

			result = Rgba(fExp__(-result.getR()), fExp__(-result.getG()), fExp__(-result.getB()));

			return result;
		}

		// emission part
		virtual Rgba integrate(RenderState &state, Ray &ray, ColorPasses &color_passes, int additional_depth /*=0*/) const
		{
			float t_0 = 0.f, t_1 = 0.f;
			int n = 10; // samples + 1 on the ray inside the volume

			Rgba result(0.f);

			bool hit = ray.tmax_ > 0.f;

			std::vector<VolumeRegion *> list_vr = scene_->getVolumes();

			for(unsigned int i = 0; i < list_vr.size(); i++)
			{
				VolumeRegion *vr = list_vr.at(i);

				if(!vr->intersect(ray, t_0, t_1)) continue;

				if(hit && ray.tmax_ < t_0) continue;

				if(hit && ray.tmax_ < t_1)
				{
					t_1 = ray.tmax_;
				}

				float step = (t_1 - t_0) / (float)n; // length between two sample points
				--n;
				float pos = t_0 + 0.5 * step;
				Point3 p(ray.from_);
				Rgb tr(1.f);

				for(int i = 0; i < n; ++i)
				{
					Ray step_ray(ray.from_ + (ray.dir_ * pos), ray.dir_, 0, step, 0);
					Rgb step_tau = vr->tau(step_ray, 0, 0);
					tr *= Rgba(fExp__(-step_tau.getR()), fExp__(-step_tau.getG()), fExp__(-step_tau.getB()));
					result += tr * vr->emission(step_ray.from_, step_ray.dir_);
					pos += step;
				}

				result *= step;
			}

			return result;
		}

		static Integrator *factory(ParamMap &params, RenderEnvironment &render)
		{
			EmissionIntegrator *inte = new EmissionIntegrator();
			return inte;
		}

};

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("EmissionIntegrator", EmissionIntegrator::factory);
	}

}

END_YAFRAY
