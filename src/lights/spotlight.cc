/****************************************************************************
 * 			spotlight.cc: a spot light with soft edge
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
 
#include <core_api/light.h>
#include <core_api/surface.h>
#include <core_api/environment.h>
#include <utilities/sample_utils.h>

__BEGIN_YAFRAY

class spotLight_t : public light_t
{
	public:
		spotLight_t(const point3d_t &from, const point3d_t &to, const color_t &col, CFLOAT power, PFLOAT angle, PFLOAT falloff, bool ponly, bool sSha, int smpl, float ssfuzzy, bool bLightEnabled=true, bool bCastShadows=true);
		virtual ~spotLight_t();
		virtual color_t totalEnergy() const;
		virtual color_t emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const;
		virtual color_t emitSample(vector3d_t &wo, lSample_t &s) const;
		virtual bool diracLight() const { return !softShadows; }
		virtual bool illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const;
		virtual bool illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi) const;
		virtual void emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const;
		virtual bool canIntersect() const{ return softShadows; }
		virtual bool intersect(const ray_t &ray, float &t, color_t &col, float &ipdf) const;
		static light_t *factory(paraMap_t &params, renderEnvironment_t &render);
		virtual int nSamples() const { return samples; };
	protected:
		point3d_t position;
		vector3d_t dir; //!< orientation of the spot cone
		vector3d_t ndir; //!< negative orientation (-dir)
		vector3d_t du, dv; //!< form a coordinate system with dir, to sample directions
		PFLOAT cosStart, cosEnd; //<! cosStart is actually larger than cosEnd, because cos goes from +1 to -1
		PFLOAT icosDiff; //<! 1.0/(cosStart-cosEnd);
		color_t color; //<! color, premulitplied by light intensity
		float intensity;
		pdf1D_t *pdf;
		float interv1, interv2;
		
		bool photonOnly;
		bool softShadows;
		float shadowFuzzy;
		int samples;
};

spotLight_t::spotLight_t(const point3d_t &from, const point3d_t &to, const color_t &col, CFLOAT power, PFLOAT angle, PFLOAT falloff, bool ponly, bool sSha, int smpl, float ssfuzzy, bool bLightEnabled, bool bCastShadows):
	light_t(LIGHT_SINGULAR), position(from), intensity(power), photonOnly(ponly), softShadows(sSha), shadowFuzzy(ssfuzzy), samples(smpl)
{
    lLightEnabled = bLightEnabled;
    lCastShadows = bCastShadows;
	ndir = (from - to).normalize();
	dir = -ndir;
	color = col*power;
	createCS(dir, du, dv);
	double rad_angle = degToRad(angle);
	double rad_inner_angle = rad_angle * (1.f - falloff);
	cosStart = fCos(rad_inner_angle);
	cosEnd = fCos(rad_angle);
	icosDiff = 1.0/(cosStart-cosEnd);
	
	float *f = new float[65];
	
	for(int i = 0; i < 65; i++)
	{
		float v = (float)i / 64.f;
		f[i] = v*v*(3.f - 2.f*v);
	}
	
	pdf = new pdf1D_t(f, 65);
	
	delete [] f;

	/* the integral of the smoothstep is 0.5, and since it gets applied to the cos, and each delta cos
		corresponds to a constant surface are of the (partial) emitting sphere, we can actually simply
		compute the energie emitted from both areas, the constant and blending one...
		1  cosStart  cosEnd              -1
		|------|--------|-----------------|
	*/
	
	interv1 = (1.0 - cosStart);
	interv2 = 0.5*(cosStart - cosEnd); // as said, energy linear to delta cos, integral is 0.5;
	float sum = std::fabs(interv1) + std::fabs(interv2);
	if(sum > 0.f) sum = 1.0/sum;
	interv1 *= sum;
	interv2 *= sum;
}

spotLight_t::~spotLight_t()
{
	if(pdf) delete pdf;
}

color_t spotLight_t::totalEnergy() const
{
	return color * M_2PI * (1.f - 0.5f*(cosStart+cosEnd));
}

bool spotLight_t::illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi) const
{
	if(photonOnly) return false;
	
	vector3d_t ldir(position - sp.P);
	PFLOAT dist_sqr = ldir*ldir;
	PFLOAT dist = fSqrt(dist_sqr);
	if(dist == 0.0) return false;
	
	PFLOAT idist_sqr = 1.f/(dist_sqr);
	ldir *= 1.f/dist; //normalize
	
	PFLOAT cosa = ndir*ldir;
	
	if(cosa < cosEnd) return false; //outside cone
	if(cosa >= cosStart) // not affected by falloff
	{
		col = color * (CFLOAT)idist_sqr;
	}
	else
	{
		PFLOAT v = (cosa - cosEnd)*icosDiff;
		v = v*v*(3.f - 2.f*v);
		col = color * (CFLOAT)(v * idist_sqr);
	}
	
	wi.tmax = dist;
	wi.dir = ldir;
	return true;
}

bool spotLight_t::illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const
{
	if(photonOnly) return false;

	vector3d_t ldir(position - sp.P);
	float dist_sqr = ldir*ldir;
	if(dist_sqr == 0.0) return false;
	float dist = fSqrt(dist_sqr);
	
	ldir *= 1.f/dist; //normalize
	
	PFLOAT cosa = ndir*ldir;	
	if(cosa < cosEnd) return false; //outside cone
	
	wi.tmax = dist;
	wi.dir = sampleCone(ldir, du, dv, cosEnd, s.s1 * shadowFuzzy, s.s2 * shadowFuzzy);
	
	if(cosa >= cosStart) // not affected by falloff
	{
		s.col = color;
	}
	else
	{
		PFLOAT v = (cosa - cosEnd)*icosDiff;
		v = v*v*(3.f - 2.f*v);
		s.col = color * (CFLOAT)v;
	}
	
	s.flags = flags;
	s.pdf = dist_sqr;
	
	//FIXME: I don't quite understand how pdf is calculated in the spotlight, but something looks wrong when dist<1.f, so I'm applying a manual correction to keep s.pdf >= 1 always, and "move" the effect of the distance to the color itself. This is a horrible patch but at least solves a problem causing darker light when distance between spotlight and surface is less than 1.f
	if(s.pdf < 1.f)
	{
		s.pdf = 1.f;
		s.col = s.col / dist_sqr;
	}
	
	return true;
}

color_t spotLight_t::emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const
{
	ray.from = position;

	if(s3 <= interv1) // sample from cone not affected by falloff:
	{
		ray.dir = sampleCone(dir, du, dv, cosStart, s1, s2);
		ipdf = M_2PI * (1.f - cosStart) / interv1;
	}
	else // sample in the falloff area 
	{
		float spdf;
		float sm2 = pdf->Sample(s2, &spdf) * pdf->invCount;
		ipdf = M_2PI * (cosStart - cosEnd) / (interv2 * spdf);
		double cosAng = cosEnd + (cosStart - cosEnd) * (double)sm2;
		double sinAng = fSqrt(1.0 - cosAng*cosAng);
		PFLOAT t1 = M_2PI*s1;
		ray.dir =  (du*fCos(t1) + dv*fSin(t1))*(PFLOAT)sinAng + dir*(PFLOAT)cosAng;
		return color * spdf*pdf->integral; // scale is just the actual falloff function, since spdf is func * invIntegral...
	}
	return color;
}

color_t spotLight_t::emitSample(vector3d_t &wo, lSample_t &s) const
{
	s.sp->P = position;
	s.areaPdf = 1.f;
	s.flags = flags;
	if(s.s3 <= interv1) // sample from cone not affected by falloff:
	{
		wo = sampleCone(dir, du, dv, cosStart, s.s1, s.s2);
		s.dirPdf = interv1 / ( M_2PI * (1.f - cosStart) );
	}
	else // sample in the falloff area 
	{
		float spdf;
		float sm2 = pdf->Sample(s.s2, &spdf) * pdf->invCount;
		s.dirPdf = (interv2 * spdf) / ( M_2PI * (cosStart - cosEnd) );
		double cosAng = cosEnd + (cosStart - cosEnd) * (double)sm2;
		double sinAng = fSqrt(1.0 - cosAng*cosAng);
		PFLOAT t1 = M_2PI*s.s1;
		wo =  (du*fCos(t1) + dv*fSin(t1))*(PFLOAT)sinAng + dir*(PFLOAT)cosAng;
		float v = sm2*sm2*(3.f - 2.f*sm2);
		return color * v;
	}
	return color;
}

void spotLight_t::emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const
{
	areaPdf = 1.f;
	cos_wo = 1.f;
	
	PFLOAT cosa = dir*wo;
	if(cosa < cosEnd) dirPdf = 0.f;
	else if(cosa >= cosStart) // not affected by falloff
	{
		dirPdf = interv1 / ( M_2PI * (1.f - cosStart) );
	}
	else
	{
		PFLOAT v = (cosa - cosEnd)*icosDiff;
		v = v*v*(3.f - 2.f*v);
		dirPdf = interv2 * v * 2.f / ( M_2PI * (cosStart - cosEnd) ); //divide by integral of v (0.5)?
	}
}
bool spotLight_t::intersect(const ray_t &ray, float &t, color_t &col, float &ipdf) const
{
	float cosA = dir*ray.dir;
	
	if(cosA == 0.f) return false;
	
	t = (dir*vector3d_t(position - ray.from)) / cosA;

	if(t < 0.f) return false;
	
	vector3d_t P(ray.from + point3d_t(t*ray.dir));
	
	if(dir * vector3d_t(P - position) == 0)
	{
		if(P*P <= 1e-2)
		{
			float cosa = dir * ray.dir;
			
			if(cosa < cosEnd) return false; //outside cone
			
			if(cosa >= cosStart) // not affected by falloff
			{
				col = color;
			}
			else
			{
				float v = (cosa - cosEnd)*icosDiff;
				v = v*v*(3.f - 2.f*v);
				col = color * v;
			}
			
			ipdf = 1.f / (t*t);
			Y_INFO << "SpotLight: ipdf, color = " << ipdf << ", " << color << yendl;
			return true;
		}
	}
	return false;
}

light_t *spotLight_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	point3d_t from(0.0);
	point3d_t to(0.f, 0.f, -1.f);
	color_t color(1.0);
	float power = 1.0;
	float angle=45, falloff=0.15;
	bool pOnly = false;
	bool softShadows = false;
	int smpl = 8;
	float ssfuzzy = 1.f;
	bool lightEnabled = true;
	bool castShadows = true;
	bool shootD = true;
	bool shootC = true;

	params.getParam("from",from);
	params.getParam("to",to);
	params.getParam("color",color);
	params.getParam("power",power);
	params.getParam("cone_angle",angle);
	params.getParam("blend",falloff);
	params.getParam("photon_only",pOnly);
	params.getParam("soft_shadows",softShadows);
	params.getParam("shadowFuzzyness",ssfuzzy);
	params.getParam("samples",smpl);
	params.getParam("light_enabled", lightEnabled);
	params.getParam("cast_shadows", castShadows);
	params.getParam("shoot_caustics", shootC);
	params.getParam("shoot_diffuse", shootD);
	
	spotLight_t *light = new spotLight_t(from, to, color, power, angle, falloff, pOnly, softShadows, smpl, ssfuzzy, lightEnabled, castShadows);
	
	light->lShootCaustic = shootC;
	light->lShootDiffuse = shootD;

	return light;
}


extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("spotlight",spotLight_t::factory);
	}
}

__END_YAFRAY
