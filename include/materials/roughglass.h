
#ifndef Y_ROUGHGLASS_H
#define Y_ROUGHGLASS_H

#include <yafraycore/nodematerial.h>

__BEGIN_YAFRAY

class roughGlassMat_t: public nodeMaterial_t
{
	public:
		roughGlassMat_t(float IOR, color_t filtC, const color_t &srcol, bool fakeS, float alpha, float disp_pow);
		virtual void initBSDF(const renderState_t &state, const surfacePoint_t &sp, unsigned int &bsdfTypes)const;
		virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs)const {return color_t(0.f);}
		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const;
		virtual float pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const {return 0.f;}
		static material_t* factory(paraMap_t &, std::list< paraMap_t > &, renderEnvironment_t &);
		virtual bool isTransparent() const { return fakeShadow; }
		virtual color_t getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const;
		virtual float getAlpha(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const;
		virtual float getMatIOR() const;
		
	protected:
		shaderNode_t* bumpS;
		shaderNode_t *mirColS;
		color_t filterCol, specRefCol;
		color_t beer_sigma_a;
		float ior;
		float a2;
		float a;
		bool absorb, disperse, fakeShadow;
		float CauchyA, CauchyB;
};

__END_YAFRAY

#endif // Y_ROUGHGLASS_H
