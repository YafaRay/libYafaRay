
#ifndef Y_AREALIGHT_H
#define Y_AREALIGHT_H

#include <core_api/light.h>
#include <core_api/environment.h>

__BEGIN_YAFRAY

class areaLight_t : public light_t
{
	public:
		areaLight_t(const point3d_t &c, const vector3d_t &v1, const vector3d_t &v2,
					const color_t &col, CFLOAT inte, int nsam);
		~areaLight_t();
		virtual void init(scene_t &scene);
		virtual color_t totalEnergy() const;
		virtual color_t emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const;
		virtual color_t emitSample(vector3d_t &wo, lSample_t &s) const;
		virtual bool diracLight() const { return false; }
		virtual bool illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const;
		virtual bool illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi)const { return false; }
		virtual bool canIntersect() const{ return true; }
		virtual bool intersect(const ray_t &ray, PFLOAT &t, color_t &col, float &ipdf) const;
		virtual float illumPdf(const surfacePoint_t &sp, const surfacePoint_t &sp_light) const;
		virtual void emitPdf(const surfacePoint_t &sp, const vector3d_t &wi, float &areaPdf, float &dirPdf, float &cos_wo) const;
		virtual int nSamples() const { return samples; }
		static light_t *factory(paraMap_t &params, renderEnvironment_t &render);
	protected:
		point3d_t corner, c2, c3, c4;
		vector3d_t toX,toY, normal, fnormal;
		vector3d_t du, dv; //!< directions for hemisampler (emitting photons)
		color_t color; //!< includes intensity amplification! so...
		int samples;
		unsigned int objID;
		float intensity; //!< ...this is actually redundant.
		float area, invArea;	
};

__END_YAFRAY

#endif //Y_AREALIGHT_H
