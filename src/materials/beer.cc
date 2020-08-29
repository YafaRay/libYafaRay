
#include <core_api/volume.h>
#include <core_api/scene.h>
#include <core_api/material.h>
#include <core_api/environment.h>
#include <core_api/params.h>

BEGIN_YAFRAY


class BeerVolumeHandler : public VolumeHandler
{
	public:
		static VolumeHandler *factory(const ParamMap &params, RenderEnvironment &env);
		BeerVolumeHandler(const Rgb &sigma): sigma_a_(sigma) {};
		BeerVolumeHandler(const Rgb &acol, double dist);

	private:
		virtual bool transmittance(const RenderState &state, const Ray &ray, Rgb &col) const override;
		virtual bool scatter(const RenderState &state, const Ray &ray, Ray &s_ray, PSample &s) const override;
		Rgb getSubSurfaceColor(const RenderState &state) const { return sigma_a_; }
		Rgb sigma_a_;
};


BeerVolumeHandler::BeerVolumeHandler(const Rgb &acol, double dist)
{
	const float maxlog = log(1e38);
	sigma_a_.r_ = (acol.r_ > 1e-38) ? -log(acol.r_) : maxlog;
	sigma_a_.g_ = (acol.g_ > 1e-38) ? -log(acol.g_) : maxlog;
	sigma_a_.b_ = (acol.b_ > 1e-38) ? -log(acol.b_) : maxlog;
	if(dist != 0.f) sigma_a_ *= 1.f / dist;
}

bool BeerVolumeHandler::transmittance(const RenderState &state, const Ray &ray, Rgb &col) const
{
	if(ray.tmax_ < 0.f || ray.tmax_ > 1e30f) //infinity check...
	{
		col = Rgb(0.f, 0.f, 0.f);
		return true;
	}
	float dist = ray.tmax_; // maybe substract ray.tmin...
	Rgb be(-dist * sigma_a_);
	col = Rgb(fExp__(be.getR()), fExp__(be.getG()), fExp__(be.getB()));
	return true;
}

bool BeerVolumeHandler::scatter(const RenderState &state, const Ray &ray, Ray &s_ray, PSample &s) const
{
	return false;
}

VolumeHandler *BeerVolumeHandler::factory(const ParamMap &params, RenderEnvironment &env)
{
	Rgb a_col(0.5f);
	double dist = 1.f;
	params.getParam("absorption_col", a_col);
	params.getParam("absorption_dist", dist);
	return new BeerVolumeHandler(a_col, dist);
}

//============================

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

SssVolumeHandler::SssVolumeHandler(const Rgb &a_col, const Rgb &s_col, double dist):
		BeerVolumeHandler(a_col, dist), dist_s_(dist), scatter_col_(s_col)
{}

bool SssVolumeHandler::scatter(const RenderState &state, const Ray &ray, Ray &s_ray, PSample &s) const
{
	float dist = -dist_s_ * log(s.s_1_);
	if(dist >= ray.tmax_) return false;
	s_ray.from_ = ray.from_ + dist * ray.dir_;
	s_ray.dir_ = sampleSphere__(s.s_2_, s.s_3_);
	s.color_ = scatter_col_;
	return true;
}

VolumeHandler *SssVolumeHandler::factory(const ParamMap &params, RenderEnvironment &env)
{
	Rgb a_col(0.5f), s_col(0.8f);
	double dist = 1.f;
	params.getParam("absorption_col", a_col);
	params.getParam("absorption_dist", dist);
	params.getParam("scatter_col", s_col);
	return new SssVolumeHandler(a_col, s_col, dist);
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("beer", BeerVolumeHandler::factory);
		render.registerFactory("sss", SssVolumeHandler::factory);
	}
}

END_YAFRAY
