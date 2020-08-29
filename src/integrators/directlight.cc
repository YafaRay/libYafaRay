/****************************************************************************
 *		directlight.cc: an integrator for direct lighting only
 *		This is part of the yafaray package
 *		Copyright (C) 2006  Mathias Wein (Lynx)
 *		Copyright (C) 2009  Rodrigo Placencia (DarkTide)
 *
 *		This library is free software; you can redistribute it and/or
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

#include <integrators/directlight.h>
#include <core_api/params.h>
#include <core_api/scene.h>
#include <core_api/imagesplitter.h>

BEGIN_YAFRAY

DirectLightIntegrator::DirectLightIntegrator(bool transp_shad, int shadow_depth, int ray_depth)
{
	type_ = Surface;
	caus_radius_ = 0.25;
	caus_depth_ = 10;
	n_caus_photons_ = 100000;
	n_caus_search_ = 100;
	tr_shad_ = transp_shad;
	use_photon_caustics_ = false;
	s_depth_ = shadow_depth;
	r_depth_ = ray_depth;
	integrator_name_ = "DirectLight";
	integrator_short_name_ = "DL";
}

bool DirectLightIntegrator::preprocess()
{
	bool success = true;
	std::stringstream set;
	g_timer__.addEvent("prepass");
	g_timer__.start("prepass");

	set << "Direct Light  ";

	if(tr_shad_)
	{
		set << "ShadowDepth=" << s_depth_ << "  ";
	}
	set << "RayDepth=" << r_depth_ << "  ";

	if(use_ambient_occlusion_)
	{
		set << "AO samples=" << ao_samples_ << " dist=" << ao_dist_ << "  ";
	}

	background_ = scene_->getBackground();
	lights_ = scene_->lights_;

	if(use_photon_caustics_)
	{
		success = createCausticMap();
		set << "\nCaustic photons=" << n_caus_photons_ << " search=" << n_caus_search_ << " radius=" << caus_radius_ << " depth=" << caus_depth_ << "  ";

		if(photon_map_processing_ == PhotonsLoad)
		{
			set << " (loading photon maps from file)";
		}
		else if(photon_map_processing_ == PhotonsReuse)
		{
			set << " (reusing photon maps from memory)";
		}
		else if(photon_map_processing_ == PhotonsGenerateAndSave) set << " (saving photon maps to file)";
	}

	g_timer__.stop("prepass");
	Y_INFO << integrator_name_ << ": Photonmap building time: " << std::fixed << std::setprecision(1) << g_timer__.getTime("prepass") << "s" << " (" << scene_->getNumThreadsPhotons() << " thread(s))" << YENDL;

	set << "| photon maps: " << std::fixed << std::setprecision(1) << g_timer__.getTime("prepass") << "s" << " [" << scene_->getNumThreadsPhotons() << " thread(s)]";

	logger__.appendRenderSettings(set.str());

	for(std::string line; std::getline(set, line, '\n');) Y_VERBOSE << line << YENDL;

	return success;
}

Rgba DirectLightIntegrator::integrate(RenderState &state, DiffRay &ray, ColorPasses &color_passes, int additional_depth /*=0*/) const
{
	Rgb col(0.0);
	float alpha;
	SurfacePoint sp;
	void *o_udat = state.userdata_;
	bool old_include_lights = state.include_lights_;

	if(transp_background_) alpha = 0.0;
	else alpha = 1.0;

	// Shoot ray into scene

	if(scene_->intersect(ray, sp)) // If it hits
	{
		unsigned char userdata[USER_DATA_SIZE];
		const Material *material = sp.material_;
		Bsdf_t bsdfs;

		state.userdata_ = (void *) userdata;
		Vec3 wo = -ray.dir_;
		if(state.raylevel_ == 0) state.include_lights_ = true;

		material->initBsdf(state, sp, bsdfs);

		if(additional_depth < material->getAdditionalDepth()) additional_depth = material->getAdditionalDepth();


		if(bsdfs & BsdfEmit)
		{
			col += color_passes.probeSet(PassIntEmit, material->emit(state, sp, wo), state.raylevel_ == 0);
		}

		if(bsdfs & BsdfDiffuse)
		{
			col += estimateAllDirectLight(state, sp, wo, color_passes);

			if(use_photon_caustics_)
			{
				if(aa_clamp_indirect_ > 0)
				{
					Rgb tmp_col = estimateCausticPhotons(state, sp, wo);
					tmp_col.clampProportionalRgb(aa_clamp_indirect_);
					col += color_passes.probeAdd(PassIntIndirect, tmp_col, state.raylevel_ == 0);
				}
				else col += color_passes.probeAdd(PassIntIndirect, estimateCausticPhotons(state, sp, wo), state.raylevel_ == 0);
			}

			if(use_ambient_occlusion_) col += sampleAmbientOcclusion(state, sp, wo);
		}

		recursiveRaytrace(state, ray, bsdfs, sp, wo, col, alpha, color_passes, additional_depth);

		if(color_passes.size() > 1 && state.raylevel_ == 0)
		{
			generateCommonRenderPasses(color_passes, state, sp, ray);

			if(color_passes.enabled(PassIntAo))
			{
				color_passes(PassIntAo) = sampleAmbientOcclusionPass(state, sp, wo);
			}

			if(color_passes.enabled(PassIntAoClay))
			{
				color_passes(PassIntAoClay) = sampleAmbientOcclusionPassClay(state, sp, wo);
			}
		}

		if(transp_refracted_background_)
		{
			float m_alpha = material->getAlpha(state, sp, wo);
			alpha = m_alpha + (1.f - m_alpha) * alpha;
		}
		else alpha = 1.0;
	}
	else // Nothing hit, return background if any
	{
		if(background_ && !transp_refracted_background_)
		{
			col += color_passes.probeSet(PassIntEnv, (*background_)(ray, state), state.raylevel_ == 0);
		}
	}

	state.userdata_ = o_udat;
	state.include_lights_ = old_include_lights;

	Rgb col_vol_transmittance = scene_->vol_integrator_->transmittance(state, ray);
	Rgb col_vol_integration = scene_->vol_integrator_->integrate(state, ray, color_passes);

	if(transp_background_) alpha = std::max(alpha, 1.f - col_vol_transmittance.r_);

	color_passes.probeSet(PassIntVolumeTransmittance, col_vol_transmittance);
	color_passes.probeSet(PassIntVolumeIntegration, col_vol_integration);

	col = (col * col_vol_transmittance) + col_vol_integration;

	return Rgba(col, alpha);
}

Integrator *DirectLightIntegrator::factory(ParamMap &params, RenderEnvironment &render)
{
	bool transp_shad = false;
	bool caustics = false;
	bool do_ao = false;
	int shadow_depth = 5;
	int raydepth = 5, c_depth = 10;
	int search = 100, photons = 500000;
	int ao_samples = 32;
	double c_rad = 0.25;
	double ao_dist = 1.0;
	Rgb ao_col(1.f);
	bool bg_transp = false;
	bool bg_transp_refract = false;
	std::string photon_maps_processing_str = "generate";

	params.getParam("raydepth", raydepth);
	params.getParam("transpShad", transp_shad);
	params.getParam("shadowDepth", shadow_depth);
	params.getParam("caustics", caustics);
	params.getParam("photons", photons);
	params.getParam("caustic_mix", search);
	params.getParam("caustic_depth", c_depth);
	params.getParam("caustic_radius", c_rad);
	params.getParam("do_AO", do_ao);
	params.getParam("AO_samples", ao_samples);
	params.getParam("AO_distance", ao_dist);
	params.getParam("AO_color", ao_col);
	params.getParam("bg_transp", bg_transp);
	params.getParam("bg_transp_refract", bg_transp_refract);
	params.getParam("photon_maps_processing", photon_maps_processing_str);

	DirectLightIntegrator *inte = new DirectLightIntegrator(transp_shad, shadow_depth, raydepth);
	// caustic settings
	inte->use_photon_caustics_ = caustics;
	inte->n_caus_photons_ = photons;
	inte->n_caus_search_ = search;
	inte->caus_depth_ = c_depth;
	inte->caus_radius_ = c_rad;
	// AO settings
	inte->use_ambient_occlusion_ = do_ao;
	inte->ao_samples_ = ao_samples;
	inte->ao_dist_ = ao_dist;
	inte->ao_col_ = ao_col;
	// Background settings
	inte->transp_background_ = bg_transp;
	inte->transp_refracted_background_ = bg_transp_refract;

	if(photon_maps_processing_str == "generate-save") inte->photon_map_processing_ = PhotonsGenerateAndSave;
	else if(photon_maps_processing_str == "load") inte->photon_map_processing_ = PhotonsLoad;
	else if(photon_maps_processing_str == "reuse-previous") inte->photon_map_processing_ = PhotonsReuse;
	else inte->photon_map_processing_ = PhotonsGenerateOnly;

	return inte;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("directlighting", DirectLightIntegrator::factory);
	}

}

END_YAFRAY
