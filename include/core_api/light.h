
#ifndef Y_LIGHT_H
#define Y_LIGHT_H

#include "ray.h"
#include "scene.h"

__BEGIN_YAFRAY

class surfacePoint_t;
class background_t;

enum { LIGHT_NONE = 0, LIGHT_DIRACDIR = 1, LIGHT_SINGULAR = 1<<1 }; // "LIGHT_DIRACDIR" *must* be same as "BSDF_SPECULAR" (material.h)!
typedef unsigned int LIGHTF_t;

struct lSample_t
{
	lSample_t(surfacePoint_t *s_p=0): sp(s_p) {}
	float s1, s2; //<! 2d sample value for choosing a surface point on the light.
	float s3, s4; //<! 2d sample value for choosing an outgoing direction on the light (emitSample)
	float pdf; //<! "standard" directional pdf from illuminated surface point for MC integration of direct lighting (illumSample)
	float dirPdf; //<! probability density for generating this sample direction (emitSample)
	float areaPdf; //<! probability density for generating this sample point on light surface (emitSample)
	color_t col; //<! color of the generated sample
	LIGHTF_t flags; //<! flags of the sampled light source
	surfacePoint_t *sp; //!< surface point on the light source, may only be complete enough to call other light methods with it!
};

class light_t
{
	public:
		//! allow for preprocessing when scene loading has finished
		virtual void init(scene_t &scene) {}
		//! total energy emmitted during whole frame
		virtual color_t totalEnergy() const = 0;
		//! emit a photon
		virtual color_t emitPhoton(float s1, float s2, float s3, float s4, ray_t &ray, float &ipdf) const = 0;
		//! create a sample of light emission, similar to emitPhoton, just more suited for bidirectional methods
		/*! fill in s.dirPdf, s.areaPdf, s.col and s.flags, and s.sp if not NULL */
		virtual color_t emitSample(vector3d_t &wo, lSample_t &s) const{return color_t(0.f);};
		//! indicate whether the light has a dirac delta distribution or not
		virtual bool diracLight() const = 0;
		//! illuminate a given surface point, generating sample s, fill in s.sp if not NULL; Set ray to test visibility by integrator
		/*! fill in s.pdf, s.col and s.flags */
		virtual bool illumSample(const surfacePoint_t &sp, lSample_t &s, ray_t &wi) const = 0;
		//! illuminate a given surfance point; Set ray to test visibility by integrator. Only for dirac lights.
		/*!	return false only if no light is emitted towards sp, e.g. outside cone angle of spot light	*/
		virtual bool illuminate(const surfacePoint_t &sp, color_t &col, ray_t &wi) const = 0;
		//! indicate whether the light can intersect with a ray (by the intersect function)
		virtual bool canIntersect() const { return false; }
		//! intersect the light source with a ray, giving back distance, energy and 1/PDF
		virtual bool intersect(const ray_t &ray, float &t, color_t &col, float &ipdf) const { return false; }
		//! get the pdf for sampling the incoming direction wi at surface point sp (illumSample!)
		/*! this method requires an intersection point with the light (sp_light). Otherwise, use intersect() */
		virtual float illumPdf(const surfacePoint_t &sp, const surfacePoint_t &sp_light) const { return 0.f; }
		//! get the pdf values for sampling point sp on the light and outgoing direction wo when emitting energy (emitSample, NOT illumSample)
		/*! sp should've been generated from illumSample or emitSample, and may only be complete enough to call light functions! */
		virtual void emitPdf(const surfacePoint_t &sp, const vector3d_t &wo, float &areaPdf, float &dirPdf, float &cos_wo) const { areaPdf=0.f; dirPdf=0.f; }
		//! checks if the light can shoot caustic photons (photonmap integrator)
		virtual bool shootsCausticP() const { return true;}
		//! checks if the light can shoot diffuse photons (photonmap integrator)
		virtual bool shootsDiffuseP() const { return true;}
		//! (preferred) number of samples for direct lighting
		virtual int nSamples() const { return 8; }
		virtual ~light_t() {}
		//! This method must be called right after the factory is called on a background light or the light will fail
		virtual void setBackground(background_t *bg) { background = bg; }

		light_t(): flags(LIGHT_NONE) {}
		light_t(LIGHTF_t _flags): flags(_flags) {}
		LIGHTF_t getFlags() const { return flags; }

	protected:
		LIGHTF_t flags;
		background_t* background;
};

__END_YAFRAY

#endif // Y_LIGHT_H
