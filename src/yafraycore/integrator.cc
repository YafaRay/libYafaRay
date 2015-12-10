/****************************************************************************
 *      integrator.cc: Basic tile based surface integrator
 *      This is part of the yafaray package
 *      Copyright (C) 2006  Mathias Wein (Lynx)
 *		Copyright (C) 2009  Rodrigo Placencia (DarkTide)
 *		Previous code might belong to:
 *		Alejandro Conty (jandro)
 *		Alfredo Greef (eshlo)
 *		Others?
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

#include <yafraycore/timer.h>
#include <yafraycore/scr_halton.h>
#include <yafraycore/spectrum.h>

#include <core_api/tiledintegrator.h>
#include <core_api/imagefilm.h>
#include <core_api/camera.h>
#include <core_api/surface.h>
#include <core_api/material.h>

#include <utilities/mcqmc.h>
#include <utilities/sample_utils.h>

#include <sstream>

__BEGIN_YAFRAY

#ifdef USING_THREADS

class renderWorker_t: public yafthreads::thread_t
{
	public:
		renderWorker_t(int numView, tiledIntegrator_t *it, scene_t *s, imageFilm_t *f, threadControl_t *c, int id, int smpls, int offs=0, bool adptv=false, int AA_pass_number=0):
			integrator(it), scene(s), imageFilm(f), control(c), samples(smpls), mNumView(numView), offset(offs), threadID(id), adaptive(adptv), AA_pass(AA_pass_number)
		{
			//Empty
		}
		virtual void body();
	protected:
		tiledIntegrator_t *integrator;
		scene_t *scene;
		imageFilm_t *imageFilm;
		threadControl_t *control;
		int samples;
        int mNumView;
        int offset;
		int threadID;
		bool adaptive;
		int AA_pass;
};

void renderWorker_t::body()
{
	renderArea_t a;
	while(imageFilm->nextArea(mNumView, a))
	{
		if(scene->getSignals() & Y_SIG_ABORT) break;
		integrator->preTile(a, samples, offset, adaptive, threadID);
		integrator->renderTile(mNumView, a, samples, offset, adaptive, threadID, AA_pass);
		control->countCV.lock();
		control->areas.push_back(a);
		control->countCV.signal();
		control->countCV.unlock();
	}
	control->countCV.lock();
	++(control->finishedThreads);
	control->countCV.signal();
	control->countCV.unlock();
}
#endif

void tiledIntegrator_t::preRender()
{
	// Empty
}

void tiledIntegrator_t::prePass(int samples, int offset, bool adaptive)
{
	// Empty
}

void tiledIntegrator_t::preTile(renderArea_t &a, int n_samples, int offset, bool adaptive, int threadID)
{
	// Empty
}

void tiledIntegrator_t::precalcDepths()
{
	const camera_t* camera = scene->getCamera();

	if(camera->getFarClip() > -1)
    {
        minDepth = camera->getNearClip();
        maxDepth = camera->getFarClip();
    }
    else
    {
        diffRay_t ray;
        // We sample the scene at render resolution to get the precision required for AA
        int w = camera->resX();
        int h = camera->resY();
        float wt = 0.f; // Dummy variable
        surfacePoint_t sp;
        for(int i=0; i<h; ++i)
        {
            for(int j=0; j<w; ++j)
            {
                ray.tmax = -1.f;
                ray = camera->shootRay(i, j, 0.5f, 0.5f, wt);
                scene->intersect(ray, sp);
                if(ray.tmax > maxDepth) maxDepth = ray.tmax;
                if(ray.tmax < minDepth && ray.tmax >= 0.f) minDepth = ray.tmax;
            }
        }
    }
	// we use the inverse multiplicative of the value aquired
	if(maxDepth > 0.f) maxDepth = 1.f / (maxDepth - minDepth);
}

bool tiledIntegrator_t::render(int numView, imageFilm_t *image)
{
	std::stringstream passString;
	imageFilm = image;
	scene->getAAParameters(AA_samples, AA_passes, AA_inc_samples, AA_threshold, AA_resampled_floor, AA_sample_multiplier_factor, AA_light_sample_multiplier_factor, AA_indirect_sample_multiplier_factor, AA_detect_color_noise, AA_dark_threshold_factor, AA_variance_edge_size, AA_variance_pixels, AA_clamp_samples, AA_clamp_indirect);
	iAA_passes = 1.f / (float) AA_passes;

	AA_sample_multiplier = 1.f;
	AA_light_sample_multiplier = 1.f;
	AA_indirect_sample_multiplier = 1.f;

	int AA_resampled_floor_pixels = (int) floorf(AA_resampled_floor * (float) imageFilm->getTotalPixels() / 100.f);

	Y_INFO << integratorName << ": Rendering " << AA_passes << " passes" << yendl;
	if(getLightGroupFilter() > 0) Y_INFO << integratorName << ": Rendering only lights from light group: " << getLightGroupFilter() << yendl;
	Y_INFO << integratorName << ": Min. " << AA_samples << " samples" << yendl;
	Y_INFO << integratorName << ": "<< AA_inc_samples << " per additional pass" << yendl;
	Y_INFO << integratorName << ": Resampled pixels floor: "<< AA_resampled_floor << "% (" << AA_resampled_floor_pixels << " pixels)" << yendl;
	Y_INFO << integratorName << ": AA_sample_multiplier_factor: "<< AA_sample_multiplier_factor << yendl;
	Y_INFO << integratorName << ": AA_light_sample_multiplier_factor: "<< AA_light_sample_multiplier_factor << yendl;
	Y_INFO << integratorName << ": AA_indirect_sample_multiplier_factor: "<< AA_indirect_sample_multiplier_factor << yendl;
	Y_INFO << integratorName << ": AA_detect_color_noise: "<< AA_detect_color_noise << yendl;
	Y_INFO << integratorName << ": AA_dark_threshold_factor: "<< AA_dark_threshold_factor << yendl;
	Y_INFO << integratorName << ": AA_variance_edge_size: "<< AA_variance_edge_size << yendl;
	Y_INFO << integratorName << ": AA_variance_pixels: "<< AA_variance_pixels << yendl;
	Y_INFO << integratorName << ": AA_clamp_samples: "<< AA_clamp_samples << yendl;
	Y_INFO << integratorName << ": AA_clamp_indirect: "<< AA_clamp_indirect << yendl;
	Y_INFO << integratorName << ": Max. " << AA_samples + std::max(0,AA_passes-1) * AA_inc_samples << " total samples" << yendl;
	passString << "Rendering pass 1 of " << std::max(1, AA_passes) << "...";
	Y_INFO << integratorName << ": " << passString.str() << yendl;
	if(intpb) intpb->setTag(passString.str().c_str());

	gTimer.addEvent("rendert");
	gTimer.start("rendert");

	imageFilm->reset_accumulated_image_area_flush_time();
	gTimer.addEvent("image_area_flush");

	imageFilm->init(AA_passes);
	imageFilm->setAANoiseParams(AA_detect_color_noise, AA_dark_threshold_factor, AA_variance_edge_size, AA_variance_pixels, AA_clamp_samples);

	maxDepth = 0.f;
	minDepth = 1e38f;

	diffRaysEnabled = false;	//always false for now, reserved for future motion blur and interference features

	if(imageFilm->passEnabled(PASS_INT_Z_DEPTH_NORM) || imageFilm->passEnabled(PASS_INT_MIST)) precalcDepths();

	preRender();

	renderPass(numView, AA_samples, 0, false, 0);
	
	int acumAASamples = AA_samples;
	
	for(int i=1; i<AA_passes; ++i)
	{
		if(scene->getSignals() & Y_SIG_ABORT) break;

		//scene->getSurfIntegrator()->setSampleMultiplier(scene->getSurfIntegrator()->getSampleMultiplier() * AA_sample_multiplier_factor);
		
		AA_sample_multiplier *= AA_sample_multiplier_factor;
		AA_light_sample_multiplier *= AA_light_sample_multiplier_factor;
		AA_indirect_sample_multiplier *= AA_indirect_sample_multiplier_factor;
		
		Y_INFO << integratorName << ": Sample multiplier = " << AA_sample_multiplier << ", Light Sample multiplier = " << AA_light_sample_multiplier << ", Indirect Sample multiplier = " << AA_indirect_sample_multiplier << yendl;
		
		imageFilm->setAAThreshold(AA_threshold);
		imageFilm->setAANoiseParams(AA_detect_color_noise, AA_dark_threshold_factor, AA_variance_edge_size, AA_variance_pixels, AA_clamp_samples);

		int resampled_pixels = imageFilm->nextPass(numView, true, integratorName);
		int AA_samples_mult = (int) ceilf(AA_inc_samples * AA_sample_multiplier);

		renderPass(numView, AA_samples_mult, acumAASamples, true, i);

		acumAASamples += AA_samples_mult;

		if(resampled_pixels < AA_resampled_floor_pixels)
		{
			float AA_variation_ratio = std::min(8.f, ((float) AA_resampled_floor_pixels / resampled_pixels)); //This allows the variation for the new pass in the AA threshold and AA samples to depend, with a certain maximum per pass, on the ratio between how many pixeles were resampled and the target floor, to get a faster approach for noise removal. 
			AA_threshold *= (1.f - 0.1f * AA_variation_ratio);
			
			Y_INFO << integratorName << ": Resampled pixels (" << resampled_pixels << ") below the floor (" << AA_resampled_floor_pixels << "): new AA Threshold (-" << AA_variation_ratio * 0.1f * 100.f << "%) for next pass = " << AA_threshold << yendl;
		} 
	}
	maxDepth = 0.f;
	gTimer.stop("rendert");
	Y_INFO << integratorName << ": Overall rendertime: " << gTimer.getTime("rendert") << "s" << yendl;

	return true;
}


bool tiledIntegrator_t::renderPass(int numView, int samples, int offset, bool adaptive, int AA_pass_number)
{
	prePass(samples, offset, adaptive);

	int nthreads = scene->getNumThreads();

#ifdef USING_THREADS
	if(nthreads>1)
	{
		threadControl_t tc;
		std::vector<renderWorker_t *> workers;
		for(int i=0;i<nthreads;++i) workers.push_back(new renderWorker_t(numView, this, scene, imageFilm, &tc, i, samples, offset, adaptive, AA_pass_number));
		for(int i=0;i<nthreads;++i)
		{
			workers[i]->run();
		}
		//update finished tiles
		tc.countCV.lock();
		while(tc.finishedThreads < nthreads)
		{
			tc.countCV.wait();
			for(size_t i=0; i<tc.areas.size(); ++i) imageFilm->finishArea(numView, tc.areas[i]);
			tc.areas.clear();
		}
		tc.countCV.unlock();
		//join all threads (although they probably have exited already, but not necessarily):
		for(int i=0;i<nthreads;++i) {workers[i]->wait(); delete workers[i];} //Fix for Linux hangs/crashes, it's better to wait for threads to end before deleting the thread objects. Using code to wait for the threads to end in the destructors is not recommended.
	}
	else
	{
#endif
		renderArea_t a;
		while(imageFilm->nextArea(numView, a))
		{
			if(scene->getSignals() & Y_SIG_ABORT) break;
			preTile(a, samples, offset, adaptive, 0);
			renderTile(numView, a, samples, offset, adaptive, 0);
			imageFilm->finishArea(numView, a);
		}
#ifdef USING_THREADS
	}
#endif
	return true; //hm...quite useless the return value :)
}

bool tiledIntegrator_t::renderTile(int numView, renderArea_t &a, int n_samples, int offset, bool adaptive, int threadID, int AA_pass_number)
{
	int x;
	const camera_t* camera = scene->getCamera();
	x=camera->resX();
	diffRay_t c_ray;
	ray_t d_ray;
	PFLOAT dx=0.5, dy=0.5, d1=1.0/(PFLOAT)n_samples;
	float lens_u=0.5f, lens_v=0.5f;
	PFLOAT wt, wt_dummy;
	random_t prng(offset*(x*a.Y+a.X)+123);
	renderState_t rstate(&prng);
	rstate.threadID = threadID;
	rstate.cam = camera;
	bool sampleLns = camera->sampleLense();
	int pass_offs=offset, end_x=a.X+a.W, end_y=a.Y+a.H;
	
	int AA_max_possible_samples = AA_samples;
	
	for(int i=1; i<AA_passes; ++i)
	{
		AA_max_possible_samples += ceilf(AA_inc_samples * pow(AA_sample_multiplier_factor, i));
	}
	
	float inv_AA_max_possible_samples = 1.f / ((float) AA_max_possible_samples);

	Halton halU(3);
	Halton halV(5);

	colorPasses_t colorPasses(scene->get_RenderPasses());
 
	colorPasses_t tmpPassesZero(scene->get_RenderPasses());
	
	for(int i=a.Y; i<end_y; ++i)
	{
		for(int j=a.X; j<end_x; ++j)
		{
			if(scene->getSignals() & Y_SIG_ABORT) break;

			if(adaptive)
			{
				if(!imageFilm->doMoreSamples(j, i)) continue;
			}

			rstate.pixelNumber = x*i+j;
			rstate.samplingOffs = fnv_32a_buf(i*fnv_32a_buf(j));//fnv_32a_buf(rstate.pixelNumber);
			float toff = scrHalton(5, pass_offs+rstate.samplingOffs); // **shall be just the pass number...**

			halU.setStart(pass_offs+rstate.samplingOffs);
			halV.setStart(pass_offs+rstate.samplingOffs);

			for(int sample=0; sample<n_samples; ++sample)
			{
				colorPasses.reset_colors();
				rstate.setDefaults();
				rstate.pixelSample = pass_offs+sample;
				rstate.time = addMod1((PFLOAT)sample*d1, toff);//(0.5+(PFLOAT)sample)*d1;

				// the (1/n, Larcher&Pillichshammer-Seq.) only gives good coverage when total sample count is known
				// hence we use scrambled (Sobol, van-der-Corput) for multipass AA
				if(AA_passes>1)
				{
					dx = RI_vdC(rstate.pixelSample, rstate.samplingOffs);
					dy = RI_S(rstate.pixelSample, rstate.samplingOffs);
				}
				else if(n_samples > 1)
				{
					dx = (0.5+(PFLOAT)sample)*d1;
					dy = RI_LP(sample+rstate.samplingOffs);
				}
				if(sampleLns)
				{
					lens_u = halU.getNext();
					lens_v = halV.getNext();
				}
				c_ray = camera->shootRay(j+dx, i+dy, lens_u, lens_v, wt);
				if(wt==0.0)
				{
					imageFilm->addSample(tmpPassesZero, j, i, dx, dy, &a, sample, AA_pass_number, inv_AA_max_possible_samples);
					continue;
				}
				if(diffRaysEnabled)
				{
					//setup ray differentials
					d_ray = camera->shootRay(j+1+dx, i+dy, lens_u, lens_v, wt_dummy);
					c_ray.xfrom = d_ray.from;
					c_ray.xdir = d_ray.dir;
					d_ray = camera->shootRay(j+dx, i+1+dy, lens_u, lens_v, wt_dummy);
					c_ray.yfrom = d_ray.from;
					c_ray.ydir = d_ray.dir;
					c_ray.hasDifferentials = true;
				}
				
				c_ray.time = rstate.time;

				colorPasses(PASS_INT_COMBINED) = integrate(rstate, c_ray, colorPasses);
				
				if(colorPasses.enabled(PASS_INT_Z_DEPTH_NORM) || colorPasses.enabled(PASS_INT_Z_DEPTH_ABS) || colorPasses.enabled(PASS_INT_MIST))
				{
					float depth_abs = 0.f, depth_norm = 0.f;

					if(colorPasses.enabled(PASS_INT_Z_DEPTH_NORM) || colorPasses.enabled(PASS_INT_MIST))
					{
						if(c_ray.tmax > 0.f)
						{
							depth_norm = 1.f - (c_ray.tmax - minDepth) * maxDepth; // Distance normalization
						}
						colorPasses.probe_set(PASS_INT_Z_DEPTH_NORM, colorA_t(depth_norm));
						colorPasses.probe_set(PASS_INT_MIST, colorA_t(1.f-depth_norm));
					}
					if(colorPasses.enabled(PASS_INT_Z_DEPTH_ABS))
					{
						depth_abs = c_ray.tmax;
						if(depth_abs <= 0.f)
						{
							depth_abs = 99999997952.f;
						}
						colorPasses.probe_set(PASS_INT_Z_DEPTH_ABS, colorA_t(depth_abs));
					}
				}
				
				for(int idx = 0; idx < colorPasses.size(); ++idx)
				{
					if(colorPasses(idx).A > 1.f) colorPasses(idx).A = 1.f;
					
					intPassTypes_t intPassType = colorPasses.intPassTypeFromIndex(idx);
										
					switch(intPassType)
					{
						case PASS_INT_Z_DEPTH_NORM: break;
						case PASS_INT_Z_DEPTH_ABS: break;
						case PASS_INT_MIST: break;
						case PASS_INT_NORMAL_SMOOTH: break;
						case PASS_INT_NORMAL_GEOM: break;
						case PASS_INT_AO: break;
						case PASS_INT_AO_CLAY: break;
						case PASS_INT_UV: break;
						case PASS_INT_DEBUG_NU: break;
						case PASS_INT_DEBUG_NV: break;
						case PASS_INT_DEBUG_DPDU: break;
						case PASS_INT_DEBUG_DPDV: break;
						case PASS_INT_DEBUG_DSDU: break;
						case PASS_INT_DEBUG_DSDV: break;
						case PASS_INT_OBJ_INDEX_ABS: break;
						case PASS_INT_OBJ_INDEX_NORM: break;
						case PASS_INT_OBJ_INDEX_AUTO: break;
						case PASS_INT_MAT_INDEX_ABS: break;
						case PASS_INT_MAT_INDEX_NORM: break;
						case PASS_INT_MAT_INDEX_AUTO: break;
						case PASS_INT_AA_SAMPLES: break;

						//Processing of mask render passes:
						case PASS_INT_OBJ_INDEX_MASK: 
						case PASS_INT_OBJ_INDEX_MASK_SHADOW: 
						case PASS_INT_OBJ_INDEX_MASK_ALL: 
						case PASS_INT_MAT_INDEX_MASK: 
						case PASS_INT_MAT_INDEX_MASK_SHADOW:
						case PASS_INT_MAT_INDEX_MASK_ALL: 

						if(colorPasses.get_pass_mask_invert())
						{
							colorPasses(idx) = colorA_t(1.f) - colorPasses(idx);
						}

						if(!colorPasses.get_pass_mask_only())
						{
							colorA_t colCombined = colorPasses(PASS_INT_COMBINED);
							colCombined.A = 1.f;	
							colorPasses(idx) *= colCombined;
						}
						break;

						default: colorPasses(idx) *= wt; break;
					}
				}

				imageFilm->addSample(colorPasses, j, i, dx, dy, &a, sample, AA_pass_number, inv_AA_max_possible_samples);
			}
		}
	}
	return true;
}

inline void tiledIntegrator_t::generateCommonRenderPasses(colorPasses_t &colorPasses, renderState_t &state, const surfacePoint_t &sp) const
{
	colorPasses.probe_set(PASS_INT_UV, colorA_t(sp.U, sp.V, 0.f, 1.f));
	colorPasses.probe_set(PASS_INT_NORMAL_SMOOTH, colorA_t((sp.N.x + 1.f) * .5f, (sp.N.y + 1.f) * .5f, (sp.N.z + 1.f) * .5f, 1.f));
	colorPasses.probe_set(PASS_INT_NORMAL_GEOM, colorA_t((sp.Ng.x + 1.f) * .5f, (sp.Ng.y + 1.f) * .5f, (sp.Ng.z + 1.f) * .5f, 1.f));
	colorPasses.probe_set(PASS_INT_DEBUG_DPDU, colorA_t((sp.dPdU.x + 1.f) * .5f, (sp.dPdU.y + 1.f) * .5f, (sp.dPdU.z + 1.f) * .5f, 1.f));
	colorPasses.probe_set(PASS_INT_DEBUG_DPDV, colorA_t((sp.dPdV.x + 1.f) * .5f, (sp.dPdV.y + 1.f) * .5f, (sp.dPdV.z + 1.f) * .5f, 1.f));
	colorPasses.probe_set(PASS_INT_DEBUG_DSDU, colorA_t((sp.dSdU.x + 1.f) * .5f, (sp.dSdU.y + 1.f) * .5f, (sp.dSdU.z + 1.f) * .5f, 1.f));
	colorPasses.probe_set(PASS_INT_DEBUG_DSDV, colorA_t((sp.dSdV.x + 1.f) * .5f, (sp.dSdV.y + 1.f) * .5f, (sp.dSdV.z + 1.f) * .5f, 1.f));
	colorPasses.probe_set(PASS_INT_DEBUG_NU, colorA_t((sp.NU.x + 1.f) * .5f, (sp.NU.y + 1.f) * .5f, (sp.NU.z + 1.f) * .5f, 1.f));
	colorPasses.probe_set(PASS_INT_DEBUG_NV, colorA_t((sp.NV.x + 1.f) * .5f, (sp.NV.y + 1.f) * .5f, (sp.NV.z + 1.f) * .5f, 1.f));

	if(colorPasses.enabled(PASS_INT_REFLECT_ALL))
    {
        colorPasses(PASS_INT_REFLECT_ALL) = colorPasses(PASS_INT_REFLECT_PERFECT) + colorPasses(PASS_INT_GLOSSY) + colorPasses(PASS_INT_GLOSSY_INDIRECT);
    }
    
	if(colorPasses.enabled(PASS_INT_REFRACT_ALL))
    {
        colorPasses(PASS_INT_REFRACT_ALL) = colorPasses(PASS_INT_REFRACT_PERFECT) + colorPasses(PASS_INT_TRANS) + colorPasses(PASS_INT_TRANS_INDIRECT);
    }
        
    if(colorPasses.enabled(PASS_INT_INDIRECT_ALL))
    {
        colorPasses(PASS_INT_INDIRECT_ALL) = colorPasses(PASS_INT_INDIRECT) + colorPasses(PASS_INT_DIFFUSE_INDIRECT);
    }

	colorPasses.probe_set(PASS_INT_DIFFUSE_COLOR, sp.material->getDiffuseColor(state));
	colorPasses.probe_set(PASS_INT_GLOSSY_COLOR, sp.material->getGlossyColor(state));
	colorPasses.probe_set(PASS_INT_TRANS_COLOR, sp.material->getTransColor(state));
	colorPasses.probe_set(PASS_INT_SUBSURFACE_COLOR, sp.material->getSubSurfaceColor(state));

	colorPasses.probe_set(PASS_INT_OBJ_INDEX_ABS, sp.object->getAbsObjectIndexColor());
	colorPasses.probe_set(PASS_INT_OBJ_INDEX_NORM, sp.object->getNormObjectIndexColor());
	colorPasses.probe_set(PASS_INT_OBJ_INDEX_AUTO, sp.object->getAutoObjectIndexColor());
	
	colorPasses.probe_set(PASS_INT_MAT_INDEX_ABS, sp.material->getAbsMaterialIndexColor());
	colorPasses.probe_set(PASS_INT_MAT_INDEX_NORM, sp.material->getNormMaterialIndexColor());
	colorPasses.probe_set(PASS_INT_MAT_INDEX_AUTO, sp.material->getAutoMaterialIndexColor());
	
	if(colorPasses.enabled(PASS_INT_OBJ_INDEX_MASK))
	{
        if(sp.object->getAbsObjectIndex() == colorPasses.get_pass_mask_obj_index()) colorPasses(PASS_INT_OBJ_INDEX_MASK) = colorA_t(1.f);
    }

	if(colorPasses.enabled(PASS_INT_OBJ_INDEX_MASK_ALL))
	{
        colorPasses(PASS_INT_OBJ_INDEX_MASK_ALL) = colorPasses(PASS_INT_OBJ_INDEX_MASK) + colorPasses(PASS_INT_OBJ_INDEX_MASK_SHADOW);
	}

	if(colorPasses.enabled(PASS_INT_MAT_INDEX_MASK))
	{
        if(sp.material->getAbsMaterialIndex() == colorPasses.get_pass_mask_mat_index()) colorPasses(PASS_INT_MAT_INDEX_MASK) = colorA_t(1.f);
	}

	if(colorPasses.enabled(PASS_INT_MAT_INDEX_MASK_ALL))
	{
        colorPasses(PASS_INT_MAT_INDEX_MASK_ALL) = colorPasses(PASS_INT_MAT_INDEX_MASK) + colorPasses(PASS_INT_MAT_INDEX_MASK_SHADOW);
	}
}


__END_YAFRAY
