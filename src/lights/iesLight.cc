/****************************************************************************
 * 		iesLight.cc: IES Light
 *      This is part of the yafaray package
 *      Copyright (C) 2009  Bert Buchholz and Rodrigo Placencia
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
#include <utilities/iesUtils.h>



__BEGIN_YAFRAY

class iesLight_t : public light_t
{
	public:

		iesLight_t(const point3d_t &from, const point3d_t &to, const color_t &col, CFLOAT power, int res, float blurS, const std::string iesFile, int smpls, bool sSha, float ang);

		virtual color_t totalEnergy() const { return color * totEnergy;};
		virtual int nSamples() const { return samples; };
		virtual bool diracLight() const { return !softShadow; }

		virtual bool illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi) const;

		virtual bool illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const;
		virtual bool canIntersect() const;
		virtual bool intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const;

		virtual color_t emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const;

		virtual color_t emitSample(vector3d_t &wo, lSample_t &s) const;
		virtual void emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const;
		
		bool isIESOk(){ return IESOk; };
		
		static light_t *factory(paraMap_t &params, renderEnvironment_t &render);
		

	protected:

		point3d_t position;
		vector3d_t dir; //!< orientation of the spot cone
		vector3d_t ndir; //!< negative orientation (-dir)
		vector3d_t du, dv; //!< form a coordinate system with dir, to sample directions
		PFLOAT cosEnd; //<! cosStart is actually larger than cosEnd, because cos goes from +1 to -1
		color_t color; //<! color, premulitplied by light intensity

		int samples;
		bool softShadow;
		
		float totEnergy;

		IESData_t *iesData;
		
		bool IESOk;
};

iesLight_t::iesLight_t(const point3d_t &from, const point3d_t &to, const color_t &col, CFLOAT power, int res, float blurS, const std::string iesFile, int smpls, bool sSha, float ang):
	light_t(LIGHT_SINGULAR), position(from), samples(smpls), softShadow(sSha)
{
	iesData = new IESData_t(blurS, res);
	
	IESOk = iesData->parseIESFile(iesFile);

	if(IESOk)
	{
		ndir = (from - to);
		ndir.normalize();
		dir = -ndir;

		createCS(dir, du, dv);

		cosEnd = fCos(iesData->getMaxVAngle());

		color = col*power;
		totEnergy = M_2PI * (1.f - 0.5f * cosEnd);
	}
}

bool iesLight_t::illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi) const
{
	vector3d_t ldir(position - sp.P);
	float distSqrt = ldir.lengthSqr();
	float dist = fSqrt(distSqrt);
	float iDistSqrt = 1.f / distSqrt;
	
	if(dist == 0.0) return false;
	
	ldir *= 1.f/dist; //normalize
	
	PFLOAT cosa = ndir*ldir;

	float u, v;
	u = radToDeg(std::acos(ldir.z));
	if(ldir.y < 0) u += 180.f;
	v = radToDeg(std::acos(cosa));

	col = color * iesData->getRadianceBlurred(u, v) * iDistSqrt;
	
	wi.tmax = dist;
	wi.dir = ldir;
	
	return true;
}

bool iesLight_t::illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const
{
	vector3d_t ldir(position - sp.P);
	float distSqrt = ldir.lengthSqr();
	float dist = fSqrt(distSqrt);
	float iDistSqrt = 1.f / distSqrt;
	
	if(dist == 0.0) return false;
	
	ldir *= 1.f/dist; //normalize
	
	PFLOAT cosa = ndir*ldir;

	float u, v;
	
	ShirleyDisk(s.s1, s.s2, u, v);

	wi.tmax = dist;
	wi.dir = sampleCone(ldir, du, dv, cosa, u, v);

	u = radToDeg(std::acos(ldir.z));
	if(ldir.y < 0) u += 180.f;
	v = radToDeg(std::acos(cosa));
	
	float rad = iesData->getRadianceBlurred(u, v);
	
	if(rad == 0.f) return false;
	
	s.col = color * iDistSqrt;
	s.pdf = 1.f / rad;

	return true;
}

bool iesLight_t::canIntersect() const
{
	return false;
}

bool iesLight_t::intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const
{
	return false;
}

color_t iesLight_t::emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const
{
	float u, v;

	ShirleyDisk(s1, s2, u, v);
	
	ray.from = position;
	ray.dir = sampleCone(dir, du, dv, cosEnd, u, v);
	
	u = radToDeg(std::acos(ray.dir.z));
	if(ray.dir.y < 0) u += 180.f;
	v = radToDeg(std::acos(ray.dir * dir));
	
	float rad = iesData->getRadianceBlurred(u, v);

	ipdf = rad;

	return color * totEnergy;
}

color_t iesLight_t::emitSample(vector3d_t &wo, lSample_t &s) const
{
	s.sp->P = position;
	s.flags = flags;
	
	float u, v, cosa;

	ShirleyDisk(s.s3, s.s4, u, v);
	
	wo = sampleCone(dir, du, dv, cosEnd, u, v);
	
	cosa = wo * dir;
	
	u = radToDeg(std::acos(wo.z));
	v = radToDeg(std::acos(cosa));
	
	float rad = iesData->getRadianceBlurred(u, v);

	s.dirPdf = s.areaPdf = (rad>0.f) ? (1.f / rad) : 1.f;

	return color;
}

void iesLight_t::emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const
{
	cos_wo = 1.f;
	
	float cosa = dir * wo;
	
	if(cosa < cosEnd)
	{
		dirPdf = areaPdf = 0.f;
	}
	else
	{
		float u, v;
		u = radToDeg(std::acos(wo.z));
		v = radToDeg(std::acos(cosa));
	
		float rad = iesData->getRadianceBlurred(u, v);

		dirPdf = areaPdf = (rad>0.f) ? (1.f / rad) : 1.f;
	}
}

light_t *iesLight_t::factory(paraMap_t &params,renderEnvironment_t &render)
{
	point3d_t from(0.0);
	point3d_t to(0.f, 0.f, -1.f);
	color_t color(1.0);
	CFLOAT power = 1.0;
	std::string file;
	float blurS = 0;
	int res = 0;
	int sam = 16; //wild goose... sorry guess :D
	bool sSha = false;
	float ang = 180.f; //full hemi

	params.getParam("from",from);
	params.getParam("to",to);
	params.getParam("color",color);
	params.getParam("power",power);
	params.getParam("resolution", res);
	params.getParam("blurStrength", blurS);
	params.getParam("file", file);
	params.getParam("samples", sam);
	params.getParam("soft_shadows", sSha);
	params.getParam("cone_angle", ang);

	iesLight_t* light = new iesLight_t(from, to, color, power, res, blurS, file, sam, sSha, ang);

	if (!light->isIESOk())
	{
		delete light;
		return NULL;
	}

	return light;
}


extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("ieslight",iesLight_t::factory);
	}
}

__END_YAFRAY
