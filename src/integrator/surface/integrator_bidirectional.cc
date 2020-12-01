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

// The Bidirectional integrator is UNSTABLE at the moment and needs to be improved. It might give unexpected and perhaps even incorrect render results. Use at your own risk.

#include "integrator/surface/integrator_bidirectional.h"
#include "geometry/surface.h"
#include "color/color_layers.h"
#include "common/logger.h"
#include "camera/camera.h"
#include "render/imagefilm.h"
#include "common/param.h"
#include "scene/scene.h"
#include "light/light.h"
#include "material/material.h"
#include "background/background.h"
#include "sampler/halton.h"
#include "sampler/sample_pdf1d.h"
#include "render/render_data.h"

BEGIN_YAFARAY

/*  conventions:
    y_0 := point on light source
    z_0 := point on camera lens
    x_0...x_k := path vertices, while x_0...x_s-1 are y_0...y_s-1 and x_k...x_s are z_0...z_t-1
    so x_i <=> z_k-i, for i>=s
    */


static constexpr int max_path_length__ =  32;
static constexpr int min_path_length__ = 3;

#define BIDIR_DEBUG 0
#define BIDIR_DO_LIGHTIMAGE 1

/*! class that holds some vertex y_i/z_i (depending on wether it is a light or camera path)
*/
class PathVertex
{
	public:
		SurfacePoint sp_;  //!< surface point at which the path vertex lies
		BsdfFlags flags_;       //!< flags of the sampled BSDF component (not all components of the sp!)
		Rgb alpha_;      //!< cumulative subpath weight; note that y_i/z_i stores alpha_i+1 !
		Rgb f_s_;        //!< f(x_i-1, x_i, x_i+1), i.e. throughput from last to next path vertex
		Vec3 wi_, wo_;  //!< sampled direction for next vertex (if available)
		float ds_;           //!< squared distance between x_i-1 and x_i
		float g_;            //!< geometric factor G(x_i-1, x_i), required for MIS
		float qi_wo_;        //!< russian roulette probability for terminating path
		float qi_wi_;        //!< russian roulette probability for terminating path when generating path in opposite direction
		float cos_wi_, cos_wo_; //!< (absolute) cosine of the incoming (wi) and sampled (wo) path direction
		float pdf_wi_, pdf_wo_; //!< the pdf for sampling wi from wo and wo from wi respectively
		void *userdata_;     //!< user data of the material at sp (required for sampling and evaluating)
};

/*! vertices of a connected path going forward from light to eye;
    conventions: path vertices are named x_0...x_k, with k=s+t-1 again.
    x_0 lies on the light source, x_k on the camera */
struct PathEvalVertex
{
	bool specular_; //!< indicate that the ingoing direction determines the outgoing one (and vice versa)
	union
	{
		float pdf_f_;    //!< pdf of sampling forward direction (x_i->x_i+1) given the backward direction
		float pdf_a_k_;  //!< in case of lense vertex we have area pdf here, there is no forward path segment
	};
	union
	{
		float pdf_b_;    //!< pdf of sampling backward direction (x_i-1->x_i) given forward direction
		float pdf_a_0_;  //!< in case of light vertex we have area pdf here, there is no backward path segment
	};
	float g_; //!< geometric term G(x_i-1, x_i)
};


void clearPath__(std::vector<PathEvalVertex> &p, int s, int t)
{
#if BIDIR_DEBUG
	for(int i = 0; i < s + t; ++i)
	{
		pathEvalVert_t &v = p[i];
		v.pdf_f = -1.f;
		v.pdf_b = -1.f;
		v.G = -1.f;
		v.specular = false;
	}
#endif
}

void checkPath__(std::vector<PathEvalVertex> &p, int s, int t)
{
#if BIDIR_DEBUG
	for(int i = 0; i < s + t; ++i)
	{
		pathEvalVert_t &v = p[i];
		if(v.pdf_f == -1.f) Y_DEBUG << integratorName << ": " << "path[" << i << "].pdf_f uninitialized! (s=" << s << " t=" << t << ")\n" << YENDL;
		if(v.pdf_b == -1.f)  Y_DEBUG << integratorName << ": " << "path[" << i << "].pdf_b uninitialized! (s=" << s << " t=" << t << ")\n" << YENDL;
		if(v.G == -1.f)  Y_DEBUG << integratorName << ": " << "path[" << i << "].G uninitialized! (s=" << s << " t=" << t << ")\n" << YENDL;
	}
#endif
}

/*! holds eye and light path, aswell as data for connection (s,t),
    i.e. connection of light vertex y_s with eye vertex z_t */
class PathData
{
	public:
		std::vector<PathVertex> light_path_, eye_path_;
		std::vector<PathEvalVertex> path_;
		//pathCon_t pc;
		// additional information for current path connection:
		Vec3 w_l_e_;       //!< direction of edge from light to eye vertex, i.e. y_s to z_t
		Rgb f_y_, f_z_;       //!< f for light and eye vertex that are connected
		float u_, v_;            //!< current position on image plane
		float d_yz_;             //!< distance between y_s to z_t
		const Light *light_;   //!< the light source to which the current path is connected
		//float pdf_Ad_0;       //!< pdf for direct lighting strategy
		float pdf_emit_, pdf_illum_;  //!< light pdfs required to calculate p1 for direct lighting strategy
		bool singular_l_;         //!< true if light has zero area (point lights for example)
		int n_paths_;             //!< number of paths that have been sampled (for current thread and image)
};

BidirectionalIntegrator::BidirectionalIntegrator(bool transp_shad, int shadow_depth): tr_shad_(transp_shad), s_depth_(shadow_depth),
																					  light_power_d_(nullptr), light_image_(nullptr)
{
	//Empty
}

BidirectionalIntegrator::~BidirectionalIntegrator()
{
	//done in cleanup() now...
	//for(unsigned int i=0; i<pathData.lightPath.size(); ++i) free(pathData.lightPath[i].userdata);
	//for(unsigned int i=0; i<pathData.eyePath.size(); ++i) free(pathData.eyePath[i].userdata);
}

bool BidirectionalIntegrator::preprocess(const RenderControl &render_control, const RenderView *render_view)
{
	thread_data_.resize(scene_->getNumThreads());
	for(int t = 0; t < scene_->getNumThreads(); ++t)
	{
		PathData &path_data = thread_data_[t];
		path_data.eye_path_.resize(max_path_length__);
		path_data.light_path_.resize(max_path_length__);
		path_data.path_.resize(max_path_length__ * 2 + 1);
		for(int i = 0; i < max_path_length__; ++i) path_data.light_path_[i].userdata_ = malloc(user_data_size_);
		for(int i = 0; i < max_path_length__; ++i) path_data.eye_path_[i].userdata_ = malloc(user_data_size_);
		path_data.n_paths_ = 0;
	}
	// initialize userdata (todo!)
	lights_ = render_view->getLightsVisible();
	int num_lights = lights_.size();
	f_num_lights_ = 1.f / (float) num_lights;
	float *energies = new float[num_lights];
	for(int i = 0; i < num_lights; ++i) energies[i] = lights_[i]->totalEnergy().energy();
	light_power_d_ = new Pdf1D(energies, num_lights);

	for(int i = 0; i < num_lights; ++i) inv_light_power_d_[lights_[i]] = light_power_d_->func_[i] * light_power_d_->inv_integral_;

	for(int i = 0; i < num_lights; ++i) Y_DEBUG << getName() << ": " << energies[i] << " (" << light_power_d_->func_[i] << ") " << YENDL;
	Y_DEBUG << getName() << ": preprocess(): lights: " << num_lights << " invIntegral:" << light_power_d_->inv_integral_ << YENDL;

	delete[] energies;

	//nPaths = 0;
	light_image_ = image_film_;// new imageFilm_t(cam->resX(), cam->resY(), 0, 0, *lightOut, 1.5f);
	light_image_->setDensityEstimation(true);
	//lightImage->setInteractive(false);
	//lightImage->init();

	// test...
	/*
	float wt, u, v;
	float pdf;
	ray_t wo = cam->shootRay(10.25, 10.25, 0, 0, wt);
	bool proj = cam->project(wo, 0, 0, u, v, pdf);
	Y_DEBUG << integratorName << ": " << "camera u=" << u << " v=" << v << " pdf=" << pdf << " (returned " << proj << ")" << YENDL;
	float integral = 0.f;
	for(int i=0; i<10000; ++i)
	{
	    wo.dir = SampleSphere((float)i/10000.f, RI_vdC(i));
	    if( cam->project(wo, 0, 0, u, v, pdf) ) integral += pdf;
	}
	Y_DEBUG << integratorName << ": " << "Camera pdf integral: " << integral/10000.f << YENDL;


	//test...
	lSample_t ls;
	float Apdf, dirPdf, cos_wo;
	surfacePoint_t sp;
	ls.s1=.5f, ls.s2=.5f, ls.s3=.5f, ls.s4=.5f;
	ls.sp = &sp;
	Rgb pcol = lights[0]->emitSample(wo.dir, ls);
	integral = 0.f;
	for(int i=0; i<10000; ++i)
	{
	    wo.dir = SampleSphere((float)i/10000.f, RI_vdC(i));
	    lights[0]->emitPdf(sp, wo.dir, Apdf, dirPdf, cos_wo);
	    integral += dirPdf;
	}
	Y_DEBUG << integratorName << ": " << "Light pdf integral: " << integral/10000.f << YENDL;
	*/

	std::stringstream set;
	set << "Bidirectional  ";

	if(tr_shad_)
	{
		set << "ShadowDepth=" << s_depth_ << "  ";
	}

	if(use_ambient_occlusion_)
	{
		set << "AO samples=" << ao_samples_ << " dist=" << ao_dist_ << "  ";
	}

	render_info_ += set.str();

	return true;
}

void BidirectionalIntegrator::cleanup()
{
	//	Y_DEBUG << integratorName << ": " << "cleanup: flushing light image" << YENDL;
	int n_paths = 0;
	for(int i = 0; i < (int)thread_data_.size(); ++i)
	{
		PathData &path_data = thread_data_[i];
		n_paths += path_data.n_paths_;
		for(int i = 0; i < max_path_length__; ++i) free(path_data.light_path_[i].userdata_);
		for(int i = 0; i < max_path_length__; ++i) free(path_data.eye_path_[i].userdata_);
	}
	light_image_->setNumDensitySamples(n_paths); //dirty hack...
}

/* ============================================================
    integrate
 ============================================================ */
Rgba BidirectionalIntegrator::integrate(RenderData &render_data, const DiffRay &ray, int additional_depth, ColorLayers *color_layers, const RenderView *render_view) const
{
	const bool layers_used = render_data.raylevel_ == 0 && color_layers && color_layers->size() > 1;

	Rgb col(0.f);
	SurfacePoint sp;
	Ray testray = ray;
	float alpha = 1.f;

	if(scene_->intersect(testray, sp))
	{
		Vec3 wo = -ray.dir_;
		static int dbg = 0;
		render_data.lights_geometry_material_emit_ = true;
		PathData &path_data = thread_data_[render_data.thread_id_];
		++path_data.n_paths_;
		Random &prng = *(render_data.prng_);
		PathVertex &ve = path_data.eye_path_.front();
		PathVertex &vl = path_data.light_path_.front();
		int n_eye = 1, n_light = 1;
		// setup ve
		ve.f_s_ = Rgb(1.f); // some random guess...need to read up on importance paths
		ve.alpha_ = Rgb(1.f);
		ve.sp_.p_ = ray.from_;
		ve.qi_wo_ = ve.qi_wi_ = 1.f; // definitely no russian roulette here...
		// temporary!
		float cu, cv;
		float cam_pdf = 0.0;
		render_view->getCamera()->project(ray, 0, 0, cu, cv, cam_pdf);
		if(cam_pdf == 0.f) cam_pdf = 1.f; //FIXME: this is a horrible hack to fix the -nan problems when using bidirectional integrator with Architecture, Angular or Orto cameras. The fundamental problem is that the code for those 3 cameras LACK the member function project() and therefore leave the camPdf=0.f causing -nan results. So, for now I'm forcing camPdf = 1.f if such 0.f result comes from the non-existing member function. This is BAD, but at least will allow people to work with the different cameras in bidirectional, and bidirectional integrator still needs a LOT of work to make it a decent integrator anyway.
		ve.pdf_wo_ = cam_pdf;
		ve.f_s_ = Rgb(cam_pdf);
		// /temporary
		ve.cos_wo_ = 1.f;
		//ve.pdf_wo = 1.f;
		ve.pdf_wi_ = 1.f;
		ve.flags_ = BsdfFlags::Diffuse; //place holder! not applicable for e.g. orthogonal camera!

		// create eyePath
		n_eye = createPath(render_data, ray, path_data.eye_path_, max_path_length__);

		// sample light (todo!)
		Ray lray;
		lray.tmin_ = scene_->ray_min_dist_;
		lray.tmax_ = -1.f;
		float light_num_pdf;
		int light_num = light_power_d_->dSample(prng(), &light_num_pdf);
		light_num_pdf *= f_num_lights_;
		LSample ls;
		ls.s_1_ = prng(), ls.s_2_ = prng(), ls.s_3_ = prng(), ls.s_4_ = prng();
		ls.sp_ = &vl.sp_;
		Rgb pcol = lights_.size() > 0 ? lights_[light_num]->emitSample(lray.dir_, ls) : Rgb(0.f);
		lray.from_ = vl.sp_.p_;
		// test!
		ls.area_pdf_ *= light_num_pdf;

		if(dbg < 10) Y_DEBUG << getName() << ": " << "lightNumPdf=" << light_num_pdf << YENDL;
		++dbg;

		// setup vl
		vl.f_s_ = Rgb(1.f); // veach set this to L_e^(1)(y0->y1), a BSDF like value; not available yet, cancels out anyway when using direct lighting
		vl.alpha_ = pcol / ls.area_pdf_; // as above, this should not contain the "light BSDF"...missing lightNumPdf!
		vl.g_ = 0.f; //unused actually...
		vl.qi_wo_ = vl.qi_wi_ = 1.f; // definitely no russian roulette here...
		vl.cos_wo_ = ls.flags_.hasAny(Light::Flags::Singular) ? 1.0 : std::abs(vl.sp_.n_ * lray.dir_); //singularities have no surface, hence no normal
		vl.cos_wi_ = 1.f;
		vl.pdf_wo_ = ls.dir_pdf_;
		vl.pdf_wi_ = ls.area_pdf_; //store area PDF here, so we don't need extra members just for camera/eye vertices
		//FIXME: this does not make any sense: vl.flags_ = ls.flags_; //store light flags in BSDF flags...same purpose though, check if delta function are involved
		path_data.singular_l_ = ls.flags_.hasAny(Light::Flags::Singular);

		// create lightPath
		n_light = createPath(render_data, lray, path_data.light_path_, max_path_length__);
		if(n_light > 1)
		{
			path_data.pdf_illum_ = lights_[light_num]->illumPdf(path_data.light_path_[1].sp_, vl.sp_) * light_num_pdf;
			path_data.pdf_emit_ = ls.area_pdf_ * path_data.light_path_[1].ds_ / vl.cos_wo_;
		}

		// do bidir evalulation

#if BIDIR_DO_LIGHTIMAGE
		// TEST! create a light image (t == 1)
		for(int s = 2; s <= n_light; ++s)
		{
			clearPath__(path_data.path_, s, 1);
			if(!connectPathE(render_view, render_data, s, path_data)) continue;
			checkPath__(path_data.path_, s, 1);
			float wt = pathWeight(render_data, s, 1, path_data);
			if(wt > 0.f)
			{
				Rgb li_col = evalPathE(render_data, s, path_data);
				if(li_col.isBlack()) continue;
				float ix, idx, iy, idy;
				idx = std::modf(path_data.u_, &ix);
				idy = std::modf(path_data.v_, &iy);
				light_image_->addDensitySample(li_col, ix, iy, idx, idy);
			}
		}
#endif

		float wt;
		for(int t = 2; t <= n_eye; ++t)
		{
			//directly hit a light?
			if(path_data.eye_path_[t - 1].sp_.light_)
			{
				// pathWeight_0t computes required probabilities, since no connection is required
				clearPath__(path_data.path_, 0, t);
				// unless someone proves the necessity, directly visible lights (s+t==2) will never be connected via light vertices
				wt = (t == 2) ? 1.f : pathWeight0T(render_data, t, path_data);
				if(wt > 0.f)
				{
					//eval is done in place here...
					PathVertex v = path_data.eye_path_[t - 1];
					render_data.arena_ = v.userdata_;
					Rgb emit = v.sp_.material_->emit(render_data, v.sp_, v.wi_);
					col += wt * v.alpha_ * emit;
				}
			}
			// direct lighting strategy (desperately needs adaption...):
			Ray d_ray;
			Rgb dcol;
			clearPath__(path_data.path_, 1, t);
			bool o_singular_l = path_data.singular_l_;  // will be overwritten from connectLPath...
			float o_pdf_illum = path_data.pdf_illum_; // will be overwritten from connectLPath...
			float o_pdf_emit = path_data.pdf_emit_;   // will be overwritten from connectLPath...
			if(connectLPath(render_data, t, path_data, d_ray, dcol))
			{
				checkPath__(path_data.path_, 1, t);
				wt = pathWeight(render_data, 1, t, path_data);
				if(wt > 0.f)
				{
					col += wt * evalLPath(render_data, t, path_data, d_ray, dcol);
				}
			}
			path_data.singular_l_ = o_singular_l;
			path_data.pdf_illum_ = o_pdf_illum;
			path_data.pdf_emit_ = o_pdf_emit;
			// light paths with one vertices are handled by classic direct light sampling (like regular path tracing)
			// hence we start with s=2 here. currently the sampling probability is the same though, so weights are unaffected
			path_data.light_ = lights_.size() > 0 ? lights_[light_num] : 0;
			for(int s = 2; s <= n_light; ++s)
			{
				clearPath__(path_data.path_, s, t);
				if(!connectPaths(render_data, s, t, path_data)) continue;
				checkPath__(path_data.path_, s, t);
				wt = pathWeight(render_data, s, t, path_data);
				if(wt > 0.f)
				{
					col += wt * evalPath(render_data, s, t, path_data);
				}
			}
		}

		if(layers_used)
		{
			generateCommonLayers(render_data, sp, ray, color_layers);

			if(ColorLayer *color_layer = color_layers->find(Layer::Ao))
			{
				BsdfFlags bsdfs;
				sp.material_->initBsdf(render_data, sp, bsdfs);
				color_layer->color_ += sampleAmbientOcclusionLayer(render_data, sp, wo);
			}

			if(ColorLayer *color_layer = color_layers->find(Layer::AoClay))
			{
				color_layer->color_ += sampleAmbientOcclusionClayLayer(render_data, sp, wo);
			}
		}
	}
	else
	{
		if(transp_background_) alpha = 0.f;

		if(scene_->getBackground() && !transp_refracted_background_)
		{
			const Rgb col_tmp = (*scene_->getBackground())(ray, render_data);
			col += col_tmp;
			if(layers_used)
			{
				if(ColorLayer *color_layer = color_layers->find(Layer::Env)) color_layer->color_ += col_tmp;
			}
		}
	}

	Rgb col_vol_transmittance = scene_->vol_integrator_->transmittance(render_data, ray);
	Rgb col_vol_integration = scene_->vol_integrator_->integrate(render_data, ray);

	if(transp_background_) alpha = std::max(alpha, 1.f - col_vol_transmittance.r_);

	if(layers_used)
	{
		if(ColorLayer *color_layer = color_layers->find(Layer::VolumeTransmittance)) color_layer->color_ += col_vol_transmittance;
		if(ColorLayer *color_layer = color_layers->find(Layer::VolumeIntegration)) color_layer->color_ += col_vol_integration;
	}

	col = (col * col_vol_transmittance) + col_vol_integration;
	return Rgba(col, alpha);
}

/* ============================================================
    createPath: create (sub-)path from given starting point
    important: resize path to maxLen *before* calling this function!
 ============================================================ */

int BidirectionalIntegrator::createPath(RenderData &render_data, const Ray &start, std::vector<PathVertex> &path, int max_len) const
{
	static int dbg = 0;
	Random &prng = *render_data.prng_;
	Ray ray(start);
	BsdfFlags m_bsdf;
	// the 0th vertex has already been generated, which is ray.pos obviously
	int n_vert = 1;
	while(n_vert < max_len)
	{
		PathVertex &v = path[n_vert];
		const PathVertex &v_prev = path[n_vert - 1];
		if(!scene_->intersect(ray, v.sp_)) break;
		const Material *mat = v.sp_.material_;
		// compute alpha_i+1 = alpha_i * fs(wi, wo) / P_proj(wo), where P_proj = bsdf_pdf(wo) / cos(wo*N)
		v.alpha_ = v_prev.alpha_ * v_prev.f_s_ * v_prev.cos_wo_ / (v_prev.pdf_wo_ * v_prev.qi_wo_);
		v.wi_ = -ray.dir_;
		v.cos_wi_ = std::abs(ray.dir_ * v.sp_.n_);
		v.ds_ = (v.sp_.p_ - v_prev.sp_.p_).lengthSqr();
		v.g_ = v_prev.cos_wo_ * v.cos_wi_ / v.ds_;
		++n_vert;
		render_data.arena_ = v.userdata_;
		//if(dbg<10) Y_DEBUG << integratorName << ": " << nVert << "  mat: " << (void*) mat << " alpha:" << v.alpha << " p_f_s:" << v_prev.f_s << " qi:"<< v_prev.qi << YENDL;
		mat->initBsdf(render_data, v.sp_, m_bsdf);
		// create tentative sample for next path segment
		Sample s(prng(), prng(), BsdfFlags::All, true);
		float w = 0.f;
		v.f_s_ = mat->sample(render_data, v.sp_, v.wi_, ray.dir_, s, w);
		if(v.f_s_.isBlack()) break;
		v.pdf_wo_ = s.pdf_;
		v.cos_wo_ = w * s.pdf_;
		// use russian roulette on tentative sample to decide on path termination, unless path is too short
		if(n_vert > min_path_length__)
		{
			v.qi_wo_ = std::min(0.98f, v.f_s_.col2Bri() * v.cos_wo_ / v.pdf_wo_);
			if(prng() > v.qi_wo_) break; // terminate path with russian roulette
		}
		else v.qi_wo_ = 1.f;

		if(s.sampled_flags_.hasAny(BsdfFlags::Specular)) // specular surfaces need special treatment...
		{
			// other materials don't return pdf_back and col_back yet
			//v.pdf_wi = s.pdf;
			v.pdf_wi_ = s.pdf_back_;
			v.qi_wi_ = std::min(0.98f, s.col_back_.col2Bri() * v.cos_wi_ / v.pdf_wi_);
		}
		else
		{
			v.pdf_wi_ = mat->pdf(render_data, v.sp_, ray.dir_, v.wi_, BsdfFlags::All); // all BSDFs? think so...
			v.qi_wi_ = std::min(0.98f, v.f_s_.col2Bri() * v.cos_wi_ / v.pdf_wi_);
		}
		if(v.qi_wi_ < 0) Y_DEBUG << getName() << ": " << "v[" << n_vert << "].qi_wi=" << v.qi_wi_ << " (" << v.f_s_.col2Bri() << " " << v.cos_wi_ << " " << v.pdf_wi_ << ")\n"
								 << "\t" << v.pdf_wo_ << "  flags:" << static_cast<unsigned int>(s.sampled_flags_) << YENDL;

		v.flags_ = s.sampled_flags_;
		v.wo_ = ray.dir_;
		ray.from_ = v.sp_.p_;
		ray.tmin_ = scene_->ray_min_dist_;
		ray.tmax_ = -1.f;
	}
	++dbg;
	return n_vert;
}

// small utilities to make code easier (well...less impossible) to read...
inline void copyLightSubpath__(PathData &pd, int s, int t)
{
	for(int i = 0; i < s - 1; ++i)
	{
		const PathVertex &v = pd.light_path_[i];
		pd.path_[i].pdf_f_ = v.pdf_wo_ / v.cos_wo_;
		pd.path_[i].pdf_b_ = v.pdf_wi_ / v.cos_wi_;
		pd.path_[i].specular_ = v.flags_.hasAny(BsdfFlags::Specular);
		pd.path_[i].g_ = v.g_;
	}
	pd.path_[s - 1].g_ = pd.light_path_[s - 1].g_;
}

inline void copyEyeSubpath__(PathData &pd, int s, int t)
{
	for(int i = s + 1, j = t - 2; j >= 0; ++i, --j)
	{
		const PathVertex &v = pd.eye_path_[j];
		pd.path_[i].pdf_f_ = v.pdf_wi_ / v.cos_wi_;
		pd.path_[i].pdf_b_ = v.pdf_wo_ / v.cos_wo_;
		pd.path_[i].specular_ = v.flags_.hasAny(BsdfFlags::Specular);
		pd.path_[i].g_ = pd.eye_path_[j + 1].g_;
	}
}

/* ============================================================
    connect the two paths in various ways
    if paths cannot be sampled from either side, return false
 ============================================================ */


//===  connect paths with s and t > 1  ===//
inline bool BidirectionalIntegrator::connectPaths(RenderData &render_data, int s, int t, PathData &pd) const
{
	const PathVertex &y = pd.light_path_[s - 1];
	const PathVertex &z = pd.eye_path_[t - 1];
	PathEvalVertex &x_l = pd.path_[s - 1];
	PathEvalVertex &x_e = pd.path_[s];
	// precompute stuff in pc that is specific to the current connection of sub-paths
	Vec3 vec = z.sp_.p_ - y.sp_.p_;
	float dist_2 = vec.normLenSqr();
	float cos_y = std::abs(y.sp_.n_ * vec);
	float cos_z = std::abs(z.sp_.n_ * vec);

	render_data.arena_ = y.userdata_;
	x_l.pdf_f_ = y.sp_.material_->pdf(render_data, y.sp_, y.wi_, vec, BsdfFlags::All); // light vert to eye vert
	x_l.pdf_b_ = y.sp_.material_->pdf(render_data, y.sp_, vec, y.wi_, BsdfFlags::All); // light vert to prev. light vert
	if(x_l.pdf_f_ < 1e-6f) return false;
	x_l.pdf_f_ /= cos_y;
	x_l.pdf_b_ /= y.cos_wi_;
	pd.f_y_ = y.sp_.material_->eval(render_data, y.sp_, y.wi_, vec, BsdfFlags::All);
	pd.f_y_ += y.sp_.material_->emit(render_data, y.sp_, vec);

	render_data.arena_ = z.userdata_;
	x_e.pdf_b_ = z.sp_.material_->pdf(render_data, z.sp_, z.wi_, -vec, BsdfFlags::All); // eye vert to light vert
	x_e.pdf_f_ = z.sp_.material_->pdf(render_data, z.sp_, -vec, z.wi_, BsdfFlags::All); // eye vert to prev eye vert
	if(x_e.pdf_b_ < 1e-6f) return false;
	x_e.pdf_b_ /= cos_z;
	x_e.pdf_f_ /= z.cos_wi_;
	pd.f_z_ = z.sp_.material_->eval(render_data, z.sp_, z.wi_, -vec, BsdfFlags::All);
	pd.f_z_ += z.sp_.material_->emit(render_data, z.sp_, -vec);

	pd.w_l_e_ = vec;
	pd.d_yz_ = math::sqrt(dist_2);
	x_e.g_ = std::abs(cos_y * cos_z) / dist_2; // or use Ng??
	x_e.specular_ = false;
	x_l.specular_ = false;

	//copy values required
	copyLightSubpath__(pd, s, t);
	copyEyeSubpath__(pd, s, t);

	// calculate qi's...
	if(s > min_path_length__) x_l.pdf_f_ *= std::min(0.98f, pd.f_y_.col2Bri()/* *cos_y */ / x_l.pdf_f_);
	if(s + 1 > min_path_length__) x_e.pdf_f_ *= std::min(0.98f, pd.f_z_.col2Bri()/* *y.cos_wi */ / x_e.pdf_f_);
	// backward:
	if(t + 1 > min_path_length__) x_l.pdf_b_ *= std::min(0.98f, pd.f_y_.col2Bri()/* *y.cos_wi */ / x_l.pdf_b_);
	if(t > min_path_length__) x_e.pdf_b_ *= std::min(0.98f, pd.f_z_.col2Bri()/* *cos_z */ / x_e.pdf_b_);

	// multiply probabilities with qi's
	int k = s + t - 1;
	// forward:
	for(int i = min_path_length__, s_1 = s - 1; i < s_1; ++i)
		pd.path_[i].pdf_f_ *= pd.light_path_[i].qi_wo_;
	for(int i = std::max(min_path_length__, s + 1), st = s + t; i < st; ++i)
		pd.path_[i].pdf_f_ *= pd.eye_path_[k - i].qi_wi_;

	//backward:
	for(int i = min_path_length__, t_1 = t - 1; i < t_1; ++i)
		pd.path_[k - i].pdf_b_ *= pd.eye_path_[i].qi_wo_;
	for(int i = std::max(min_path_length__, t + 1), st = s + t; i < st; ++i)
		pd.path_[k - i].pdf_b_ *= pd.light_path_[k - i].qi_wi_;
	return true;
}

// connect path with s==1 (eye path with single light vertex)
inline bool BidirectionalIntegrator::connectLPath(RenderData &render_data, int t, PathData &pd, Ray &l_ray, Rgb &lcol) const
{
	// create light sample with direct lighting strategy:
	const PathVertex &z = pd.eye_path_[t - 1];
	l_ray.from_ = z.sp_.p_;
	l_ray.tmin_ = 0.0005;
	int n_lights_i = lights_.size();
	if(n_lights_i == 0) return false;
	float light_num_pdf, cos_wo;
	int lnum = light_power_d_->dSample((*render_data.prng_)(), &light_num_pdf);
	light_num_pdf *= f_num_lights_;
	if(lnum > n_lights_i - 1) lnum = n_lights_i - 1;
	const Light *light = lights_[lnum];
	SurfacePoint sp_light;

	//== use illumSample, no matter what...s1/s2 is only set when required ==
	LSample ls;
	if(!light->getFlags()) //only lights with non-specular components need sample values
	{
		ls.s_1_ = (*render_data.prng_)();
		ls.s_2_ = (*render_data.prng_)();
	}
	ls.sp_ = &sp_light;
	// generate light sample, abort when none could be created:
	if(!light->illumSample(z.sp_, ls, l_ray)) return false;

	//FIXME DAVID: another series of horrible hacks to avoid uninitialized values and incorrect renders in bidir. However, this should be properly solved by implementing correctly the functions needed by bidir in the lights and materials, and correcting the bidir integrator itself...
	ls.sp_->p_ = Point3(0.f, 0.f, 0.f);
	Vec3 wo = l_ray.dir_;
	light->emitSample(wo, ls);
	ls.flags_ = static_cast<Light::Flags>(0xFFFFFFFF);

	lcol = ls.col_ / (ls.pdf_ * light_num_pdf); //shouldn't really do that division, better use proper c_st in evalLPath...
	// get probabilities for generating light sample without a given surface point
	Vec3 vec = -l_ray.dir_;
	light->emitPdf(sp_light, vec, pd.path_[0].pdf_a_0_, pd.path_[0].pdf_f_, cos_wo);
	pd.path_[0].pdf_a_0_ *= light_num_pdf;
	pd.path_[0].pdf_f_ /= cos_wo;
	pd.path_[0].specular_ = ls.flags_.hasAny(Light::Flags::DiracDir);
	pd.singular_l_ = ls.flags_.hasAny(Light::Flags::Singular);
	pd.pdf_illum_ = ls.pdf_ * light_num_pdf;
	pd.pdf_emit_ = pd.path_[0].pdf_a_0_ * (sp_light.p_ - z.sp_.p_).lengthSqr() / cos_wo;


	//fill in pc...connecting to light vertex:
	//pathEvalVert_t &x_l = pd.path[0];
	PathEvalVertex &x_e = pd.path_[1];
	float cos_z = std::abs(z.sp_.n_ * vec);
	x_e.g_ = std::abs(cos_wo * cos_z) / (l_ray.tmax_ * l_ray.tmax_); // or use Ng??
	pd.w_l_e_ = vec;
	pd.d_yz_ = l_ray.tmax_;
	render_data.arena_ = z.userdata_;
	x_e.pdf_b_ = z.sp_.material_->pdf(render_data, z.sp_, z.wi_, l_ray.dir_, BsdfFlags::All); //eye to light
	if(x_e.pdf_b_ < 1e-6f) return false;
	x_e.pdf_f_ = z.sp_.material_->pdf(render_data, z.sp_, l_ray.dir_, z.wi_, BsdfFlags::All); // eye to prev eye
	x_e.pdf_b_ /= cos_z;
	x_e.pdf_f_ /= z.cos_wi_;
	x_e.specular_ = false;
	pd.f_z_ = z.sp_.material_->eval(render_data, z.sp_, z.wi_, l_ray.dir_, BsdfFlags::All);
	pd.f_z_ += z.sp_.material_->emit(render_data, z.sp_, l_ray.dir_);
	pd.light_ = light;

	//copy values required
	pd.path_[0].g_ = 0.f;
	copyEyeSubpath__(pd, 1, t);

	// calculate qi's...
	// backward:
	//if(t+1>MIN_PATH_LENGTH) x_l.pdf_b *= std::min( 0.98f, pd.f_y.col2bri()*y.cos_wi / x_l.pdf_b ); //unused/meaningless(?)
	if(t > min_path_length__) x_e.pdf_b_ *= std::min(0.98f, pd.f_z_.col2Bri()/* *cos_z */ / x_e.pdf_b_);

	// multiply probabilities with qi's
	int k = t;
	// forward:
	for(int i = std::max(min_path_length__, 2), st = t + 1; i < st; ++i)
		pd.path_[i].pdf_f_ *= pd.eye_path_[st - i - 1].qi_wi_;

	//backward:
	for(int i = min_path_length__, t_1 = t - 1; i < t_1; ++i)
		pd.path_[k - i].pdf_b_ *= pd.eye_path_[i].qi_wo_;
	return true;
}

// connect path with t==1 (s>1)
inline bool BidirectionalIntegrator::connectPathE(const RenderView *render_view, RenderData &render_data, int s, PathData &pd) const
{
	const PathVertex &y = pd.light_path_[s - 1];
	const PathVertex &z = pd.eye_path_[0];
	PathEvalVertex &x_l = pd.path_[s - 1];
	PathEvalVertex &x_e = pd.path_[s];

	Vec3 vec = z.sp_.p_ - y.sp_.p_;
	float dist_2 = vec.normLenSqr();
	float cos_y = std::abs(y.sp_.n_ * vec);

	Ray wo(z.sp_.p_, -vec);
	if(! render_view->getCamera()->project(wo, 0, 0, pd.u_, pd.v_, x_e.pdf_b_)) return false;

	x_e.specular_ = false; // cannot query yet...

	render_data.arena_ = y.userdata_;
	x_l.pdf_f_ = y.sp_.material_->pdf(render_data, y.sp_, y.wi_, vec, BsdfFlags::All); // light vert to eye vert
	if(x_l.pdf_f_ < 1e-6f) return false;
	x_l.pdf_b_ = y.sp_.material_->pdf(render_data, y.sp_, vec, y.wi_, BsdfFlags::All); // light vert to prev. light vert
	x_l.pdf_f_ /= cos_y;
	x_l.pdf_b_ /= y.cos_wi_;
	pd.f_y_ = y.sp_.material_->eval(render_data, y.sp_, y.wi_, vec, BsdfFlags::All);
	pd.f_y_ += y.sp_.material_->emit(render_data, y.sp_, vec);
	x_l.specular_ = false;

	pd.w_l_e_ = vec;
	pd.d_yz_ = math::sqrt(dist_2);
	x_e.g_ = cos_y / dist_2; // or use Ng??
	x_e.pdf_f_ = 1.f; // unused...

	copyLightSubpath__(pd, s, 1);

	// calculate qi's...
	if(s > min_path_length__) x_l.pdf_f_ *= std::min(0.98f, pd.f_y_.col2Bri()/* *cos_y */ / x_l.pdf_f_);
	//if(s+1>MIN_PATH_LENGTH) x_e.pdf_f *= std::min( 0.98f, pd.f_z.col2bri()*y.cos_wi / x_e.pdf_f );

	// multiply probabilities with qi's
	int k = s;
	// forward:
	for(int i = min_path_length__, s_1 = s - 1; i < s_1; ++i)
		pd.path_[i].pdf_f_ *= pd.light_path_[i].qi_wo_;

	//backward:
	for(int i = std::max(min_path_length__, 2), st = s + 1; i < st; ++i) //FIXME: why st=s+1 and not just s?
		pd.path_[k - i].pdf_b_ *= pd.light_path_[k - i].qi_wi_;
	return true;
}


/* ============================================================
    calculate the path weight with some combination strategy
 ============================================================ */

// compute path densities and weight path
float BidirectionalIntegrator::pathWeight(RenderData &render_data, int s, int t, PathData &pd) const
{
	const std::vector<PathEvalVertex> &path = pd.path_;
	float pr[2 * max_path_length__ + 1], p[2 * max_path_length__ + 1];
	p[s] = 1.f;
	// "forward" weights (towards eye), ratio pr_i here is p_i+1 / p_i
	int k = s + t - 1;
	for(int i = s; i < k; ++i)
	{
		pr[i] = (path[i - 1].pdf_f_ * path[i].g_) / (path[i + 1].pdf_b_ * path[i + 1].g_);
		p[i + 1] = p[i] * pr[i];
	}
	// "backward" weights (towards light), ratio pr_i here is p_i / p_i+1
	for(int i = s - 1; i > 0; --i)
	{
		pr[i] = (path[i + 1].pdf_b_ * path[i + 1].g_) / (path[i - 1].pdf_f_ * path[i].g_);
		p[i] = p[i + 1] * pr[i];
	}
	// do p_0/p_1...
	pr[0] = (path[1].pdf_b_ * path[1].g_) / path[0].pdf_a_0_;
	p[0] = p[1] * pr[0];
	// p_k+1/p_k is zero currently, hitting the camera lens in general should be very seldom anyway...
	p[k + 1] = 0.f;
#if !(BIDIR_DO_LIGHTIMAGE)
	p[k] = 0.f; // cannot intersect camera yet...
#endif
	// treat specular scatter events.
	// specular x_i makes p_i (join x_i-1 and x_i) and p_i+1 (join x_i and x_i+1) = 0:
	for(int i = 0; i <= k; ++i)
	{
		if(path[i].specular_)
		{
			p[i] = 0.f;
			p[i + 1] = 0.f;
		}
	}
	if(pd.singular_l_) p[0] = 0.f;
	// correct p1 with direct lighting strategy:
	else if(pd.pdf_emit_ < -1.0e-12 || pd.pdf_emit_ > +1.0e-12) p[1] *= pd.pdf_illum_ / pd.pdf_emit_; //test! workaround for incomplete pdf funcs of lights
	else return 1.f;	//FIXME: horrible workaround for the problem of Bidir render black randomly (depending on whether you compiled with debug or -O3, depending on whether you started Blender directly or using gdb, etc), when using Sky Sun. All this part of the Bidir integrator is horrible anyway, so for now I'm just trying to make it work.
	// do MIS...maximum heuristic, particularly simple, if there's a more likely sample method, weight is zero, otherwise 1
	float weight = 1.f;

	for(int i = s - 1; i >= 0; --i) if(p[i] > p[s] && !(p[i] < -1.0e36) && !(p[i] > 1.0e36) && !(p[s] < -1.0e36) && !(p[s] > 1.0e36)) weight = 0.f;	//FIXME: manual check for very big positive/negative values (horrible fix) for for the problem of Bidir render black when compiling with -O3 --fast-math.
	for(int i = s + 1; i <= k + 1; ++i) if(p[i] > p[s] && !(p[i] < -1.0e36) && !(p[i] > 1.0e36) && !(p[s] < -1.0e36) && !(p[s] > 1.0e36)) weight = 0.f; //FIXME: manual check for very big positive/negative values (horrible fix) for for the problem of Bidir render black when compiling with -O3 --fast-math.

	return weight;
}

// weight paths that directly hit a light, i.e. s=0; t is at least 2 //
float BidirectionalIntegrator::pathWeight0T(RenderData &render_data, int t, PathData &pd) const
{
	const std::vector<PathEvalVertex> &path = pd.path_;
	const PathVertex &vl = pd.eye_path_[t - 1];
	// since we need no connect, complete some probabilities here:
	float light_num_pdf = inv_light_power_d_.find(vl.sp_.light_)->second;
	light_num_pdf *= f_num_lights_;
	float cos_wo;
	// direct lighting pdf...
	float pdf_illum = vl.sp_.light_->illumPdf(pd.eye_path_[t - 2].sp_, vl.sp_) * light_num_pdf;
	if(pdf_illum < 1e-6f) return 0.f;

	vl.sp_.light_->emitPdf(vl.sp_, vl.wi_, pd.path_[0].pdf_a_0_, pd.path_[0].pdf_f_, cos_wo);
	pd.path_[0].pdf_a_0_ *= light_num_pdf;
	float pdf_emit = pd.path_[0].pdf_a_0_ * vl.ds_ / cos_wo;
	pd.path_[0].g_ = 0.f; // unused...
	pd.path_[0].specular_ = false;
	copyEyeSubpath__(pd, 0, t);
	checkPath__(pd.path_, 0, t);

	// == standard weighting procedure now == //

	float pr, p[2 * max_path_length__ + 1];

	p[0] = 1;
	p[1] = path[0].pdf_a_0_ / (path[1].pdf_b_ * path[1].g_);

	int k = t - 1;
	for(int i = 1; i < k; ++i)
	{
		pr = (path[i - 1].pdf_f_ * path[i].g_) / (path[i + 1].pdf_b_ * path[i + 1].g_);
		p[i + 1] = p[i] * pr;
	}

	// p_k+1/p_k is zero currently, hitting the camera lens in general should be very seldom anyway...
	p[k + 1] = 0.f;
#if !(BIDIR_DO_LIGHTIMAGE)
	p[k] = 0.f; // cannot intersect camera yet...
#endif
	// treat specular scatter events.
	for(int i = 0; i <= k; ++i)
	{
		if(path[i].specular_) p[i] = p[i + 1] = 0.f;
	}
	// correct p1 with direct lighting strategy:
	p[1] *= pdf_illum / pdf_emit;

	// do MIS...maximum heuristic, particularly simple, if there's a more likely sample method, weight is zero, otherwise 1
	float weight = 1.f;
	for(int i = 1; i <= t; ++i) if(p[i] > 1.f) weight = 0.f;

	return weight;
}

/* ============================================================
    eval the (unweighted) path created by connecting
    sub-paths at s,t
 ============================================================ */


Rgb BidirectionalIntegrator::evalPath(RenderData &render_data, int s, int t, PathData &pd) const
{
	const PathVertex &y = pd.light_path_[s - 1];
	const PathVertex &z = pd.eye_path_[t - 1];
	float mask_obj_index = 0.f, mask_mat_index = 0.f;

	Rgb c_st = pd.f_y_ * pd.path_[s].g_ * pd.f_z_;
	//unweighted contronution C*:
	Rgb c_uw = y.alpha_ * c_st * z.alpha_;
	Ray con_ray(y.sp_.p_, pd.w_l_e_, 0.0005, pd.d_yz_);
	Rgb scol = Rgb(0.f);
	const bool shadowed = (tr_shad_) ? scene_->isShadowed(render_data, con_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(render_data, con_ray, mask_obj_index, mask_mat_index);
	if(shadowed) return Rgb(0.f);
	if(tr_shad_) c_uw *= scol;
	return c_uw;
}

//===  eval paths with s==1 (direct lighting strategy)  ===//
Rgb BidirectionalIntegrator::evalLPath(RenderData &render_data, int t, PathData &pd, Ray &l_ray, const Rgb &lcol) const
{
	static int dbg = 0;
	float mask_obj_index = 0.f, mask_mat_index = 0.f;
	Rgb scol = Rgb(0.f);
	const bool shadowed = (tr_shad_) ? scene_->isShadowed(render_data, l_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(render_data, l_ray, mask_obj_index, mask_mat_index);
	if(shadowed) return Rgb(0.f);

	const PathVertex &z = pd.eye_path_[t - 1];

	Rgb c_uw = lcol * pd.f_z_ * z.alpha_ * std::abs(z.sp_.n_ * l_ray.dir_); // f_y, cos_x0_f and r^2 computed in connectLPath...(light pdf)
	if(tr_shad_) c_uw *= scol;
	// hence c_st is only cos_x1_b * f_z...like path tracing
	//if(dbg < 10) Y_DEBUG << integratorName << ": " << "evalLPath(): f_z:" << pd.f_z << " C_uw:" << C_uw << YENDL;
	++dbg;
	return c_uw;
}

//=== eval path with t==1 (light path directly connected to eve vertex)
//almost same as evalPath, just that there is no material on one end but a camera sensor function (soon...)
Rgb BidirectionalIntegrator::evalPathE(RenderData &render_data, int s, PathData &pd) const
{
	const PathVertex &y = pd.light_path_[s - 1];
	float mask_obj_index = 0.f, mask_mat_index = 0.f;
	Ray con_ray(y.sp_.p_, pd.w_l_e_, 0.0005, pd.d_yz_);

	Rgb scol = Rgb(0.f);
	const bool shadowed = (tr_shad_) ? scene_->isShadowed(render_data, con_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(render_data, con_ray, mask_obj_index, mask_mat_index);
	if(shadowed) return Rgb(0.f);

	//eval material
	render_data.arena_ = y.userdata_;
	//Rgb f_y = y.sp.material->eval(state, y.sp, y.wi, pd.w_l_e, BSDF_ALL);
	//TODO:
	Rgb c_uw = y.alpha_ * M_PI * pd.f_y_ * pd.path_[s].g_;
	if(tr_shad_) c_uw *= scol;
	return c_uw;
}

////

/* inline Rgb biDirIntegrator_t::estimateOneDirect(renderState_t &state, const surfacePoint_t &sp, vector3d_t wo, pathCon_t &pc) const
{
    Rgb lcol(0.0), scol, col(0.0);
    ray_t lightRay;
    bool shadowed;
    const material_t *oneMat = sp.material;
    lightRay.from = sp.P;
    int nLightsI = lights.size();
    if(nLightsI == 0) return Rgb(0.f);
    float s1, s2, lightPdf, lightNumPdf;
    int lnum = lightPowerD->DSample((*state.prng)(), &lightNumPdf);
    if(lnum > nLightsI-1) lnum = nLightsI-1;
    const light_t *light = lights[lnum];
    // handle lights with delta distribution, e.g. point and directional lights
    if( light->diracLight() )
    {
        if( light->illuminate(sp, lcol, lightRay) )
        {
            // ...shadowed...
            lightRay.tmin = 0.0005; // < better add some _smart_ self-bias value...this is bad.
            shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
            if(!shadowed)
            {
                if(trShad) lcol *= scol;
                Rgb surfCol = oneMat->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
                col = surfCol * lcol * std::abs(sp.N*lightRay.dir);
            }
        }
    }
    else // area light and suchlike
    {
        Rgb ccol(0.0);
        s1 = (*state.prng)();
        s2 = (*state.prng)();

        if( light->illumSample (sp, s1, s2, lcol, lightPdf, lightRay) )
        {
            // ...shadowed...
            lightRay.tmin = 0.0005; // < better add some _smart_ self-bias value...this is bad.
            shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
            if(!shadowed)
            {
                if(trShad) lcol *= scol;
                Rgb surfCol = oneMat->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
                //test! limit lightPdf...
                if(lightPdf > 2.f) lightPdf = 2.f;
                col = surfCol * lcol * std::abs(sp.N*lightRay.dir) * lightPdf;
            }
        }
    }
    return col/lightNumPdf;
} */

Rgb BidirectionalIntegrator::sampleAmbientOcclusionLayer(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	Rgb col(0.f), surf_col(0.f), scol(0.f);
	bool shadowed;
	const Material *material = sp.material_;
	Ray light_ray;
	light_ray.from_ = sp.p_;
	float mask_obj_index = 0.f, mask_mat_index = 0.f;

	int n = ao_samples_;//(int) ceilf(aoSamples*getSampleMultiplier());
	if(render_data.ray_division_ > 1) n = std::max(1, n / render_data.ray_division_);

	unsigned int offs = n * render_data.pixel_sample_ + render_data.sampling_offs_;

	Halton hal_2(2, offs - 1);
	Halton hal_3(3, offs - 1);

	for(int i = 0; i < n; ++i)
	{
		float s_1 = hal_2.getNext();
		float s_2 = hal_3.getNext();

		if(render_data.ray_division_ > 1)
		{
			s_1 = math::addMod1(s_1, render_data.dc_1_);
			s_2 = math::addMod1(s_2, render_data.dc_2_);
		}

		light_ray.tmax_ = ao_dist_;

		float w = 0.f;

		Sample s(s_1, s_2, BsdfFlags::Glossy | BsdfFlags::Diffuse | BsdfFlags::Reflect);
		surf_col = material->sample(render_data, sp, wo, light_ray.dir_, s, w);

		if(material->getFlags().hasAny(BsdfFlags::Emit))
		{
			col += material->emit(render_data, sp, wo) * s.pdf_;
		}

		shadowed = tr_shad_ ? scene_->isShadowed(render_data, light_ray, s_depth_, scol, mask_obj_index, mask_mat_index) : scene_->isShadowed(render_data, light_ray, mask_obj_index, mask_mat_index);

		if(!shadowed)
		{
			float cos = std::abs(sp.n_ * light_ray.dir_);
			if(tr_shad_) col += ao_col_ * scol * surf_col * cos * w;
			else col += ao_col_ * surf_col * cos * w;
		}
	}

	return col / (float)n;
}


Rgb BidirectionalIntegrator::sampleAmbientOcclusionClayLayer(RenderData &render_data, const SurfacePoint &sp, const Vec3 &wo) const
{
	Rgb col(0.f), surf_col(0.f);
	bool shadowed;
	const Material *material = sp.material_;
	Ray light_ray;
	light_ray.from_ = sp.p_;
	float mask_obj_index = 0.f, mask_mat_index = 0.f;

	int n = ao_samples_;
	if(render_data.ray_division_ > 1) n = std::max(1, n / render_data.ray_division_);

	const unsigned int offs = n * render_data.pixel_sample_ + render_data.sampling_offs_;
	Halton hal_2(2, offs - 1);
	Halton hal_3(3, offs - 1);

	for(int i = 0; i < n; ++i)
	{
		float s_1 = hal_2.getNext();
		float s_2 = hal_3.getNext();

		if(render_data.ray_division_ > 1)
		{
			s_1 = math::addMod1(s_1, render_data.dc_1_);
			s_2 = math::addMod1(s_2, render_data.dc_2_);
		}

		light_ray.tmax_ = ao_dist_;

		float w = 0.f;

		Sample s(s_1, s_2, BsdfFlags::All);
		surf_col = material->sampleClay(render_data, sp, wo, light_ray.dir_, s, w);
		s.pdf_ = 1.f;

		if(material->getFlags().hasAny(BsdfFlags::Emit))
		{
			col += material->emit(render_data, sp, wo) * s.pdf_;
		}

		shadowed = scene_->isShadowed(render_data, light_ray, mask_obj_index, mask_mat_index);

		if(!shadowed)
		{
			float cos = std::abs(sp.n_ * light_ray.dir_);
			//if(trShad) col += aoCol * scol * surfCol * cos * W;
			col += ao_col_ * surf_col * cos * w;
		}
	}

	return col / (float)n;
}


Integrator *BidirectionalIntegrator::factory(ParamMap &params, const Scene &scene)
{
	bool do_ao = false;
	int ao_samples = 32;
	double ao_dist = 1.0;
	Rgb ao_col(1.f);
	bool bg_transp = false;
	bool bg_transp_refract = false;
	bool transp_shad = false;
	int shadow_depth = 4;

	params.getParam("transpShad", transp_shad);
	params.getParam("shadowDepth", shadow_depth);
	params.getParam("do_AO", do_ao);
	params.getParam("AO_samples", ao_samples);
	params.getParam("AO_distance", ao_dist);
	params.getParam("AO_color", ao_col);
	params.getParam("bg_transp", bg_transp);
	params.getParam("bg_transp_refract", bg_transp_refract);

	BidirectionalIntegrator *inte = new BidirectionalIntegrator(transp_shad, shadow_depth);

	// AO settings
	inte->use_ambient_occlusion_ = do_ao;
	inte->ao_samples_ = ao_samples;
	inte->ao_dist_ = ao_dist;
	inte->ao_col_ = ao_col;

	// Background settings
	inte->transp_background_ = bg_transp;
	inte->transp_refracted_background_ = bg_transp_refract;

	return inte;
}

END_YAFARAY
