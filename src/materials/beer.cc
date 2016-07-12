
#include <core_api/volume.h>
#include <core_api/scene.h>
#include <core_api/material.h>
#include <core_api/environment.h>

__BEGIN_YAFRAY


class beer_t: public volumeHandler_t
{
	public:
		beer_t(const color_t &sigma): sigma_a(sigma) {};
		beer_t(const color_t &acol, double dist);
		virtual bool transmittance(const renderState_t &state, const ray_t &ray, color_t &col) const;
		virtual bool scatter(const renderState_t &state, const ray_t &ray, ray_t &sRay, pSample_t &s) const;
        virtual color_t getSubSurfaceColor(const renderState_t &state) const
        {
                return sigma_a;
        }
		
		static volumeHandler_t* factory(const paraMap_t &params, renderEnvironment_t &env);
	protected:
		color_t sigma_a;
};


beer_t::beer_t(const color_t &acol, double dist)
{
	const float maxlog = log(1e38);
	sigma_a.R = (acol.R > 1e-38) ? -log(acol.R) : maxlog;
	sigma_a.G = (acol.G > 1e-38) ? -log(acol.G) : maxlog;
	sigma_a.B = (acol.B > 1e-38) ? -log(acol.B) : maxlog;
	if (dist!=0.f) sigma_a *= 1.f/dist;
}

bool beer_t::transmittance(const renderState_t &state, const ray_t &ray, color_t &col) const
{
	if(ray.tmax < 0.f || ray.tmax > 1e30f) //infinity check...
	{
		col = color_t( 0.f, 0.f, 0.f);
		return true;
	}
	float dist = ray.tmax; // maybe substract ray.tmin...
	color_t be(-dist * sigma_a);
	col = color_t( fExp(be.getR()), fExp(be.getG()), fExp(be.getB()) );
	return true;
}

bool beer_t::scatter(const renderState_t &state, const ray_t &ray, ray_t &sRay, pSample_t &s) const
{
	return false;
}

volumeHandler_t* beer_t::factory(const paraMap_t &params, renderEnvironment_t &env)
{
	color_t a_col(0.5f);
	double dist=1.f;
	params.getParam("absorption_col", a_col);
	params.getParam("absorption_dist", dist);
	return new beer_t(a_col, dist);
}

//============================

class sss_t: public beer_t
{
	public:
		sss_t(const color_t &a_col, const color_t &s_col, double dist);
		//virtual bool transmittance(const renderState_t &state, const ray_t &ray, color_t &col) const { return false; };
		virtual bool scatter(const renderState_t &state, const ray_t &ray, ray_t &sRay, pSample_t &s) const;
	
		static volumeHandler_t* factory(const paraMap_t &params, renderEnvironment_t &env);
	protected:
		float dist_s;
		color_t scatter_col;
};

sss_t::sss_t(const color_t &a_col, const color_t &s_col, double dist):
	beer_t(a_col, dist), dist_s(dist), scatter_col(s_col)
{}

bool sss_t::scatter(const renderState_t &state, const ray_t &ray, ray_t &sRay, pSample_t &s) const
{
	float dist = -dist_s * log(s.s1);
	if(dist >= ray.tmax) return false;
	sRay.from = ray.from + dist*ray.dir;
	sRay.dir = SampleSphere(s.s2, s.s3);
	s.color = scatter_col;
	return true;
}

volumeHandler_t* sss_t::factory(const paraMap_t &params, renderEnvironment_t &env)
{
	color_t a_col(0.5f), s_col(0.8f);
	double dist=1.f;
	params.getParam("absorption_col", a_col);
	params.getParam("absorption_dist", dist);
	params.getParam("scatter_col", s_col);
	return new sss_t(a_col, s_col, dist);
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("beer", beer_t::factory);
		render.registerFactory("sss", sss_t::factory);
	}
}

__END_YAFRAY
