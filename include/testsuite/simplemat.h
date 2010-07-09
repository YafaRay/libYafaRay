
#ifndef Y_SIMPLEMAT_H
#define Y_SIMPLEMAT_H

#include <yafray_config.h>
#include <core_api/material.h>
#include <core_api/texture.h>


__BEGIN_YAFRAY
/*
class sbsdf_t: public BSDF_t
{
	public:
	sbsdf_t(){};
	sbsdf_t(const surfacePoint_t &sp, void* userdata);
	virtual color_t sample(const vector3d_t &wo, vector3d_t &wi, float s1, float s2, void* userdata){ return col; }
	virtual color_t eval(const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, void* userdata)
	{
		*/ /*if(sp.geometry.N*wl > 0.0)*/ /* return col;
		//return color_t(0.0);
	}
	color_t col;
};
*/
class simplemat_t: public material_t
{
	public:
	simplemat_t(const color_t &col, float transp=0.0, float emit=0.0, texture_t *tex=0);

//	virtual BSDF_t* getBSDF(const surfacePoint_t &sp, void* userdata)const { return &bsdf; }
	virtual void initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const { bsdfTypes=bsdfFlags; }
	virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs)const;
	virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const;
	virtual bool isTransparent() const { return transparent; }
	virtual color_t getTransparency(const vector3d_t &wo, const surfacePoint_t &sp)const { return trCol; }
	virtual color_t emit(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const { return emitCol; }
	virtual void getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, 
							bool &reflect, bool &refract, vector3d_t *const dir, color_t *const col)const;
	virtual bool scatterPhoton(const surfacePoint_t &sp, const vector3d_t &wi, vector3d_t &wo, float s1, float s2,
								BSDF_t bsdfs, BSDF_t &sampledBSDF, color_t &col) const;
	protected:
//	mutable sbsdf_t bsdf;
	bool transparent;
	color_t color, trCol, emitCol, photonCol;
	float pDiffuse, pScatter;
	texture_t *texture;
};

__END_YAFRAY

#endif // Y_SIMPLEMAT_H
