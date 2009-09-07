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
#include <utilities/interpolation.h>

__BEGIN_YAFRAY

class iesLight_t : public light_t
{
	public:

		iesLight_t(const point3d_t &from, const point3d_t &to, const color_t &col, CFLOAT power, int res, float blurS, IesData& iesD, int smpls, bool sSha, float ang);

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
		
		static light_t *factory(paraMap_t &params, renderEnvironment_t &render);
		

	protected:

		bool getRadiance(float u, float v, float &result) const;
		float getRadianceBlurred(float u, float v) const;

		point3d_t position;
		vector3d_t dir; //!< orientation of the spot cone
		vector3d_t ndir; //!< negative orientation (-dir)
		vector3d_t du, dv; //!< form a coordinate system with dir, to sample directions
		PFLOAT cosEnd; //<! cosStart is actually larger than cosEnd, because cos goes from +1 to -1
		color_t color; //<! color, premulitplied by light intensity
		float intensity;
		int resolution;
		float blurStrength;
		
		float maxRad;
		
		int minRes;
		int maxRes;
		
		float resStep;

		int samples;
		bool softShadow;
		
		float totEnergy;

		IesData iesData;
};

iesLight_t::iesLight_t(const point3d_t &from, const point3d_t &to, const color_t &col, CFLOAT power, int res, float blurS, IesData& iesD, int smpls, bool sSha, float ang):
	light_t(LIGHT_SINGULAR), position(from), intensity(power), resolution(res), blurStrength(blurS), samples(smpls), softShadow(sSha)
{
	ndir = (from - to);
	ndir.normalize();
	dir = -ndir;
	color = col*power;
	createCS(dir, du, dv);

	iesData = iesD;
	// debug output
	for (int i = 0; i < iesData.vertAngles; ++i) {
		std::cout << iesData.vertAngleMap[i] << " ";
	}
	std::cout << std::endl;

	for (int i = 0; i < iesData.horAngles; ++i) {
		std::cout << iesData.horAngleMap[i] << " ";
	}
	std::cout << std::endl;
	
	maxRad = 0.f;
	
	for (int i = 0; i < iesData.horAngles; ++i) {
		for (int j = 0; j < iesData.vertAngles; ++j) {
			if(iesData.radMap[i][j] > maxRad) maxRad = iesData.radMap[i][j];
			
			std::cout << iesData.radMap[i][j] << " ";
		}
		std::cout << std::endl;
	}
	// end debug output
	
	cosEnd = fCos(degToRad(iesData.vertAngleMap[iesData.vertAngles - 1]));
	
	totEnergy = M_2PI * (1.f - 0.5f * cosEnd);
	
	maxRad = 1.f / maxRad;
	
	int reso_2 = resolution >> 1; //reso / 2
	
	minRes = -reso_2; 
	maxRes = reso_2;
	
	resStep = blurStrength / (float)resolution;
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
	if(cosa < cosEnd) return false; //outside cone

	float u, v;

	u = 1.f; // arbitrary
	v = radToDeg(std::acos(cosa));

	col = color * getRadianceBlurred(u, v) * iDistSqrt;
	
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
	if(cosa < cosEnd) return false; //outside cone

	float u, v;
	
	ShirleyDisk(s.s1, s.s2, u, v);

	wi.tmax = dist;
	wi.dir = sampleCone(ldir, du, dv, cosa, u, v);

	u = 1.f; // arbitrary
	v = radToDeg(std::acos(cosa));
	
	float rad = getRadianceBlurred(u, v);
	
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
	
	u = 1.f;
	v = radToDeg(std::acos(ray.dir * dir));
	
	float rad = getRadianceBlurred(u, v);
	
	ipdf = rad;

	return color * totEnergy;
}

color_t iesLight_t::emitSample(vector3d_t &wo, lSample_t &s) const
{
	s.sp->P = position;
	s.areaPdf = 1.f;
	s.flags = flags;
	wo = sampleCone(dir, du, dv, cosEnd, s.s1, s.s2);
	s.dirPdf = 1.0f / 2.0f;
	return color;
}

void iesLight_t::emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const
{
	areaPdf = 1.f;
	cos_wo = 1.f;
	
	PFLOAT cosa = dir*wo;
	if(cosa < cosEnd) dirPdf = 0.f;
	dirPdf = 1.0f / 2.0f;
}

// amount: size of "kernel" in degrees, resolution: number of samples
float iesLight_t::getRadianceBlurred(float u, float v) const {

	float ret = 0.f;

	if (blurStrength < 0.5 || resolution < 2)
	{
		getRadiance(u, v, ret);
	}
	else
	{
		int hits = 0;
	
		for (int i = minRes; i < maxRes; ++i)
		{
			float tmp;
			
			if (getRadiance(u, v + (resStep * (float)i), tmp))
			{
				ret += tmp;
				++hits;
			}
		}
		ret /= (float)hits;
	}

	return ret * maxRad;
}

bool iesLight_t::getRadiance(float u, float v, float& rad) const {
	
	rad = 0;
	
	if (iesData.horAngles == 1)
	{
		float vertAngleStart = iesData.vertAngleMap[0];
		float vertAngleEnd = iesData.vertAngleMap[iesData.vertAngles - 1];

		if (v >= vertAngleStart && v < vertAngleEnd)
		{
			int i_v = 0;

			while (i_v < iesData.vertAngles - 2 && v > iesData.vertAngleMap[i_v+1]) ++i_v;
			
			float vert1 = iesData.vertAngleMap[i_v];
			float vert2 = iesData.vertAngleMap[i_v+1];
			float vertDiff = vert2 - vert1;

			float offset_v = (v - vert1) / vertDiff;

			rad = CosineInterpolate(iesData.radMap[0][i_v], iesData.radMap[0][i_v + 1], offset_v);
			
			return true;
		}
	}
	return false;
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

	IesData iesData;
	if (!parseIesFile(file, iesData)) {
		return NULL;
	}

	return new iesLight_t(from, to, color, power, res, blurS, iesData, sam, sSha, ang);
}


extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("ieslight",iesLight_t::factory);
	}
}

__END_YAFRAY
