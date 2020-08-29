/****************************************************************************
 * 			directlight.cc: an integrator for direct lighting only
 *      This is part of the yafray package
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

#include <yafray_constants.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/tiledintegrator.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <core_api/params.h>
#include <core_api/scene.h>
#include <core_api/imagesplitter.h>
#include <integrators/integr_utils.h>
#include <yafraycore/photon.h>
#include <utilities/mcqmc.h>
#include <yafraycore/scr_halton.h>

BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT DebugIntegrator final : public TiledIntegrator
{
	public:
		static Integrator *factory(ParamMap &params, RenderEnvironment &render);

	private:
		enum SurfaceProperties {N = 1, DPdU = 2, DPdV = 3, Nu = 4, Nv = 5, DSdU = 6, DSdV = 7};
		DebugIntegrator(SurfaceProperties dt);
		virtual bool preprocess() override;
		virtual Rgba integrate(RenderState &state, DiffRay &ray, ColorPasses &color_passes, int additional_depth /*=0*/) const override;

		std::vector<Light *> lights_;
		SurfaceProperties debug_type_;
		bool show_pn_;
};

DebugIntegrator::DebugIntegrator(SurfaceProperties dt)
{
	type_ = Surface;
	debug_type_ = dt;
	integrator_name_ = "DebugIntegrator";
	integrator_short_name_ = "DBG";
	logger__.appendRenderSettings("Debug integrator: '");
	switch(dt)
	{
		case N:
			logger__.appendRenderSettings("N");
			break;
		case DPdU:
			logger__.appendRenderSettings("dPdU");
			break;
		case DPdV:
			logger__.appendRenderSettings("dPdV");
			break;
		case Nu:
			logger__.appendRenderSettings("NU");
			break;
		case Nv:
			logger__.appendRenderSettings("NV");
			break;
		case DSdU:
			logger__.appendRenderSettings("dSdU");
			break;
		case DSdV:
			logger__.appendRenderSettings("dSdV");
			break;
	}

	logger__.appendRenderSettings("' | ");
}

bool DebugIntegrator::preprocess()
{
	return true;
}

Rgba DebugIntegrator::integrate(RenderState &state, DiffRay &ray, ColorPasses &color_passes, int additional_depth /*=0*/) const
{
	Rgb col(0.0);
	SurfacePoint sp;
	void *o_udat = state.userdata_;
	bool old_include_lights = state.include_lights_;
	//shoot ray into scene
	if(scene_->intersect(ray, sp))
	{
		if(show_pn_)
		{
			// Normals perturbed by materials
			unsigned char userdata[USER_DATA_SIZE + 7];
			userdata[0] = 0;
			state.userdata_ = (void *)(&userdata[7] - (((size_t)&userdata[7]) & 7));   // pad userdata to 8 bytes

			Bsdf_t bsdfs;
			const Material *material = sp.material_;
			material->initBsdf(state, sp, bsdfs);
		}
		if(debug_type_ == N)
			col = Rgb((sp.n_.x_ + 1.f) * .5f, (sp.n_.y_ + 1.f) * .5f, (sp.n_.z_ + 1.f) * .5f);
		else if(debug_type_ == DPdU)
			col = Rgb((sp.dp_du_.x_ + 1.f) * .5f, (sp.dp_du_.y_ + 1.f) * .5f, (sp.dp_du_.z_ + 1.f) * .5f);
		else if(debug_type_ == DPdV)
			col = Rgb((sp.dp_dv_.x_ + 1.f) * .5f, (sp.dp_dv_.y_ + 1.f) * .5f, (sp.dp_dv_.z_ + 1.f) * .5f);
		else if(debug_type_ == Nu)
			col = Rgb((sp.nu_.x_ + 1.f) * .5f, (sp.nu_.y_ + 1.f) * .5f, (sp.nu_.z_ + 1.f) * .5f);
		else if(debug_type_ == Nv)
			col = Rgb((sp.nv_.x_ + 1.f) * .5f, (sp.nv_.y_ + 1.f) * .5f, (sp.nv_.z_ + 1.f) * .5f);
		else if(debug_type_ == DSdU)
			col = Rgb((sp.ds_du_.x_ + 1.f) * .5f, (sp.ds_du_.y_ + 1.f) * .5f, (sp.ds_du_.z_ + 1.f) * .5f);
		else if(debug_type_ == DSdV)
			col = Rgb((sp.ds_dv_.x_ + 1.f) * .5f, (sp.ds_dv_.y_ + 1.f) * .5f, (sp.ds_dv_.z_ + 1.f) * .5f);

	}
	state.userdata_ = o_udat;
	state.include_lights_ = old_include_lights;
	return Rgba(col, 1.f);
}

Integrator *DebugIntegrator::factory(ParamMap &params, RenderEnvironment &render)
{
	int dt = 1;
	bool pn = false;
	params.getParam("debugType", dt);
	params.getParam("showPN", pn);
	std::cout << "debugType " << dt << std::endl;
	DebugIntegrator *inte = new DebugIntegrator((SurfaceProperties)dt);
	inte->show_pn_ = pn;

	return inte;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("DebugIntegrator", DebugIntegrator::factory);
	}

}

END_YAFRAY
