/****************************************************************************
 * 			blendmat.cc: a material that blends two material
 *      This is part of the yafray package
 *      Copyright (C) 2008  Mathias Wein
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
 
#include <materials/blendmat.h>
#include <utilities/sample_utils.h>

__BEGIN_YAFRAY

#define PTR_ADD(ptr,sz) ((char*)ptr+(sz))

#define screenColors(c1,c2) (1.0 - ((1.0 - (c1)) * (1.0 - (c2))))
#define sumColors(c1, c2) ((c1) + (c2))
#define multColors(c1,c2) ((c1) * (c2))
#define addColors(c1, c2, v1, v2) sumColors(c1*v1, c2*v2)

#define addPdf(p1, p2) std::max(p1, p2)//((p1+p2) * 0.5f)

blendMat_t::blendMat_t(const material_t *m1, const material_t *m2, float bval):
	mat1(m1), mat2(m2), blendS(0)
{
	bsdfFlags = mat1->getFlags() | mat2->getFlags();
	mmem1 = mat1->getReqMem();
	recalcBlend = false;
	blendVal = bval;
	blendedIOR = (mat1->getMatIOR() + mat2->getMatIOR()) * 0.5f;
}

blendMat_t::~blendMat_t()
{
}

inline void blendMat_t::getBlendVal(const renderState_t &state, const surfacePoint_t &sp, float &val, float &ival) const
{

	if(recalcBlend)
	{
		void *old_dat = state.userdata;
		
		nodeStack_t stack(state.userdata);
		evalNodes(state, sp, allSorted, stack);
		val = blendS->getScalar(stack);
		state.userdata = old_dat;
	}
	else
	{	
		val = blendVal;
	}

	ival = 1.f - val;
}

void blendMat_t::initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const
{
	float val, ival;
	getBlendVal(state, sp, val, ival);
	
	void *old_udat = state.userdata;
	
	BSDF_t matFlags = BSDF_NONE;
	bsdfTypes = BSDF_NONE;
	
	state.userdata = PTR_ADD(state.userdata, reqMem);
	if(ival > 0.f)
	mat1->initBSDF(state, sp, bsdfTypes);
	
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val > 0.f)
	mat2->initBSDF(state, sp, matFlags);
	
	bsdfTypes |= matFlags;
	
	//todo: bump mapping blending
	state.userdata = old_udat;
}

color_t blendMat_t::eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wl, BSDF_t bsdfs)const
{
	float val, ival;
	getBlendVal(state, sp, val, ival);
	
	color_t col1(1.f), col2(1.f);
	void *old_udat = state.userdata;

	state.userdata = PTR_ADD(state.userdata, reqMem);
	if(ival > 0.f)
	col1 = mat1->eval(state, sp, wo, wl, bsdfs);
	
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val > 0.f)
	col2 = mat2->eval(state, sp, wo, wl, bsdfs);
	
	state.userdata = old_udat;

	col1.clampRGB01();
	col2.clampRGB01();

	col1 = addColors(col1, col2, ival, val);
	return col1;
}

color_t blendMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	float val, ival;
	getBlendVal(state, sp, val, ival);
	
	color_t col1(0.f), col2(0.f);
	sample_t s1 = s, s2 = s;
	void *old_udat = state.userdata;

	s2.pdf = s1.pdf = s.pdf = 0.f;
	wi = wo;
	vector3d_t wi1 = wi, wi2 = wi;

	state.userdata = PTR_ADD(state.userdata, reqMem);
	if(ival > 0.f)
	{
		col1 = mat1->sample(state, sp, wo, wi1, s1);
		col1.clampRGB01();
	}
	
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val > 0.f)
	{
		col2 = mat2->sample(state, sp, wo, wi2, s2);
		col2.clampRGB01();
	}
	
	wi = (wi2 + wi1).normalize();
	
	s.pdf = addPdf(s1.pdf, s2.pdf);
	s.sampledFlags = s1.sampledFlags || s2.sampledFlags;
	s.reverse = s1.reverse || s2.reverse;
	if(s.reverse)
	{
		s.pdf_back = addPdf(s1.pdf_back, s2.pdf_back);
		s.col_back = addColors(s1.col_back, s2.col_back, ival, val);
	}
	col1 = addColors(col1, col2, ival, val);

	state.userdata = old_udat;
	return col1;
}

float blendMat_t::pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const
{
	float val, ival;
	getBlendVal(state, sp, val, ival);
	
	float pdf1 = 0.f, pdf2 = 0.f;
	void *old_udat = state.userdata;
	
	state.userdata = PTR_ADD(state.userdata, reqMem);
	if(ival>0.f)
	pdf1 = mat1->pdf(state, sp, wo, wi, bsdfs);
	
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val>0.f)
	pdf2 = mat2->pdf(state, sp, wo, wi, bsdfs);

	state.userdata = old_udat;

	pdf1 = addPdf(pdf1, pdf2);
	return pdf1;
}

void blendMat_t::getSpecular(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo,
							  bool &reflect, bool &refract, vector3d_t *const dir, color_t *const col)const
{
	float val, ival;
	getBlendVal(state, sp, val, ival);

	void *old_udat = state.userdata;
	
	reflect=false;
	refract=false;
	
	bool m1_reflect=false, m1_refract=false;
	
	vector3d_t m1_dir[2];
	color_t m1_col[2];
	
	m1_col[0] = color_t(0.f);
	m1_col[1] = color_t(0.f);
	m1_dir[0] = vector3d_t(0.f);
	m1_dir[1] = vector3d_t(0.f);
	
	state.userdata = PTR_ADD(state.userdata, reqMem);
	if(ival > 0.f)
	mat1->getSpecular(state, sp, wo, m1_reflect, m1_refract, m1_dir, m1_col);
	
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val > 0.f)
	mat2->getSpecular(state, sp, wo, reflect, refract, dir, col);
	
	state.userdata = old_udat;
	
	m1_col[0].clampRGB01();
	m1_col[1].clampRGB01();
	col[0].clampRGB01();
	col[1].clampRGB01();

	if(reflect && m1_reflect)
	{
		col[0] = addColors(m1_col[0], col[0], ival, val);
		dir[0] = (dir[0] + m1_dir[0]).normalize();
	}
	else if(m1_reflect)
	{
		col[0] = m1_col[0] * ival;
		dir[0] = m1_dir[0];
	}
	else
	{
		col[0] = col[0] * val;
	}
	
	if(refract && m1_refract)
	{
		col[1] = addColors(m1_col[1], col[1], ival, val);
		dir[1] = (dir[1] + m1_dir[1]).normalize();
	}
	else if(m1_refract)
	{
		col[1] = m1_col[1] * ival;
		dir[1] = m1_dir[1];
	}
	else
	{
		col[1] = col[1] * val;
	}

	refract = refract || m1_refract;
	reflect = reflect || m1_reflect;
}

float blendMat_t::getMatIOR() const
{
	return blendedIOR;
}

bool blendMat_t::isTransparent() const
{
	return mat1->isTransparent() || mat2->isTransparent();
}

color_t blendMat_t::getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	float val, ival;
	getBlendVal(state, sp, val, ival);

	color_t col1(1.f), col2(1.f);
	
	void *old_udat = state.userdata;
	
	state.userdata = PTR_ADD(state.userdata, reqMem);
	if(ival > 0.f)
	col1 = mat1->getTransparency(state, sp, wo);
	
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val > 0.f)
	col2 = mat2->getTransparency(state, sp, wo);

	col1.clampRGB01();
	col2.clampRGB01();

	col1 = addColors(col1, col2, ival, val);
	
	state.userdata = old_udat;
	return col1;
}

CFLOAT blendMat_t::getAlpha(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	if(isTransparent())
	{
		float val, ival;
		getBlendVal(state, sp, val, ival);
		
		float al1 = 0.f, al2 = 0.f;
		
		void *old_udat = state.userdata;
		
		state.userdata = PTR_ADD(state.userdata, reqMem);
		if(ival > 0.f)
		al1 = mat1->getAlpha(state, sp, wo);
		
		state.userdata = PTR_ADD(state.userdata, mmem1);
		if(val > 0.f)
		al2 = mat2->getAlpha(state, sp, wo);
	
		al1 = (al1 * ival) + (al2 * val);
		
		state.userdata = old_udat;
		
		return al1;
	}
	
	return 1.0;
}

color_t blendMat_t::emit(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	float val, ival;
	getBlendVal(state, sp, val, ival);

	color_t col1(0.0), col2(0.0);
	void *old_udat = state.userdata;

	state.userdata = PTR_ADD(state.userdata, reqMem);
	if(ival > 0.f)
	col1 = mat1->emit(state, sp, wo);
	
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val > 0.f)
	col2 = mat2->emit(state, sp, wo);
	
	col1.clampRGB01();
	col2.clampRGB01();

	col1 = addColors(col1, col2, ival, val);
		
	state.userdata = old_udat;
	return col1;
}

bool blendMat_t::scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wi, vector3d_t &wo, pSample_t &s) const
{
	float val, ival;
	getBlendVal(state, sp, val, ival);

	void *old_udat = state.userdata;
	bool ret = false;
	
	color_t col1(0.f), col2(0.f);

	state.userdata = PTR_ADD(state.userdata, reqMem);
	if(ival > 0.f)
	{
		ret = ret || mat1->scatterPhoton(state, sp, wi, wo, s);
		col1 = s.color;
	}
	
	state.userdata = PTR_ADD(state.userdata, mmem1);
	if(val > 0.f)
	{
		ret = ret || mat2->scatterPhoton(state, sp, wi, wo, s);
		col2 = s.color;
	}
	
	col1.clampRGB01();
	col2.clampRGB01();
	
	s.color = addColors(col1, col2, ival, val);
		
	state.userdata = old_udat;
	return ret;
}

material_t* blendMat_t::factory(paraMap_t &params, std::list<paraMap_t> &eparams, renderEnvironment_t &env)
{
	const std::string *name = 0;
	const material_t *m1=0, *m2=0;
	double blend_val = 0.5;
	
	if(! params.getParam("material1", name) ) return 0;
	m1 = env.getMaterial(*name);
	if(! params.getParam("material2", name) ) return 0;
	m2 = env.getMaterial(*name);
	params.getParam("blend_value", blend_val);
	
	if(m1==0 || m2==0 ) return 0;
	
	blendMat_t *mat = new blendMat_t(m1, m2, blend_val);
	
	std::vector<shaderNode_t *> roots;
	if(mat->loadNodes(eparams, env))
	{
		if(params.getParam("mask", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end())
			{
				mat->blendS = i->second;
				mat->recalcBlend = true;
				roots.push_back(mat->blendS);
				}
			else
			{
				Y_ERROR << "Blend: Blend shader node '" << *name << "' does not exist!" << yendl;
				delete mat;
				return 0;
			}
		}
	}
	else
	{
		Y_ERROR << "Blend: loadNodes() failed!" << yendl;
		delete mat;
		return 0;
	}
	mat->solveNodesOrder(roots);
	mat->reqMem = sizeof(bool) + mat->reqNodeMem;
	return mat;
}

extern "C"
{
	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("blend_mat", blendMat_t::factory);
	}

}

__END_YAFRAY
