/****************************************************************************
 * 			glossymat.cc: a glossy material based on Ashikhmin&Shirley's Paper
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
 
#include <yafray_config.h>
#include <yafraycore/nodematerial.h>
#include <core_api/environment.h>
#include <utilities/sample_utils.h>
#include <materials/microfacet.h>


__BEGIN_YAFRAY

class glossyMat_t: public nodeMaterial_t
{
	public:
		glossyMat_t(const color_t &col, const color_t &dcol, float reflect, float diff, float expo, bool as_diffuse);
		virtual void initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const;
		virtual color_t eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const;
		virtual color_t sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const;
		virtual float pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const;
		virtual bool scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wi, vector3d_t &wo, pSample_t &s) const;
		static material_t* factory(paraMap_t &, std::list< paraMap_t > &, renderEnvironment_t &);
		
		struct MDat_t
		{
			float mDiffuse, mGlossy, pDiffuse;
			void *stack;
		};

	protected:
		shaderNode_t* diffuseS;
		shaderNode_t* glossyS;
		shaderNode_t* glossyRefS;
		shaderNode_t* bumpS;
		color_t gloss_color, diff_color;
		float exponent, exp_u, exp_v;
		float reflectivity;
		float mDiffuse;
		bool as_diffuse, with_diffuse, anisotropic;
};

glossyMat_t::glossyMat_t(const color_t &col, const color_t &dcol, float reflect, float diff, float expo, bool as_diff):
			diffuseS(0), glossyS(0), glossyRefS(0), bumpS(0), gloss_color(col), diff_color(dcol), exponent(expo),
			reflectivity(reflect), mDiffuse(diff), as_diffuse(as_diff), with_diffuse(false), anisotropic(false)
{
	bsdfFlags = BSDF_NONE;
	
	if(diff > 0)
	{
		bsdfFlags = BSDF_DIFFUSE | BSDF_REFLECT;
		with_diffuse = true;
	}
	
	bsdfFlags |= as_diffuse ? (BSDF_DIFFUSE | BSDF_REFLECT) : (BSDF_GLOSSY | BSDF_REFLECT);
}

void glossyMat_t::initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const
{
	MDat_t *dat = (MDat_t *)state.userdata;
	dat->stack = (char*)state.userdata + sizeof(MDat_t);
	nodeStack_t stack(dat->stack);
	if(bumpS) evalBump(stack, state, sp, bumpS);
	
	std::vector<shaderNode_t *>::const_iterator iter, end=allViewindep.end();
	for(iter = allViewindep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp);
	bsdfTypes=bsdfFlags;
	dat->mDiffuse = mDiffuse;
	dat->mGlossy = glossyRefS ? glossyRefS->getScalar(stack) : reflectivity;
	dat->pDiffuse = std::min(0.6f , 1.f - (dat->mGlossy/(dat->mGlossy + (1.f-dat->mGlossy)*dat->mDiffuse)) );
}

color_t glossyMat_t::eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const
{
	MDat_t *dat = (MDat_t *)state.userdata;
	color_t col(0.f);
	bool diffuse_flag = bsdfs & BSDF_DIFFUSE;
	
	if( !diffuse_flag ) return col;
	
	nodeStack_t stack(dat->stack);
	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);

	float wiN = std::fabs(wi * N);
	float woN = std::fabs(wo * N);
	
	
	if( (as_diffuse && diffuse_flag) || (!as_diffuse && (bsdfs & BSDF_GLOSSY)) )
	{
		vector3d_t H = (wo + wi).normalize(); // half-angle
		float cos_wi_H = wi*H;
		float glossy;

		if(anisotropic)
		{
			vector3d_t Hs(H*sp.NU, H*sp.NV, H*N);
			glossy = AS_Aniso_D(Hs, exp_u, exp_v) * SchlickFresnel(cos_wi_H, dat->mGlossy) / ( 8.f * std::fabs(cos_wi_H) * std::max(woN, wiN) );
		}
		else
		{
			glossy = Blinn_D(H*N, exponent) * SchlickFresnel(cos_wi_H, dat->mGlossy) / ( 8.f * std::fabs(cos_wi_H) * std::max(woN, wiN) );

		}

		col = glossy*(glossyS ? glossyS->getColor(stack) : gloss_color);
	}

	if(with_diffuse && diffuse_flag)
	{
		PFLOAT f_wi = fPow(1.f - (0.5f * wiN), 5.f);
		PFLOAT f_wo = fPow(1.f - (0.5f * woN), 5.f);
		PFLOAT diffuse = DIFFUSE_RATIO * (1.f - dat->mGlossy) * (1.f - f_wi) * (1.f - f_wo);
		color_t diff_base = (diffuseS ? diffuseS->getColor(stack) : diff_color);
		col += diffuse * dat->mDiffuse * diff_base;
	}
	return col;
}


color_t glossyMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	MDat_t *dat = (MDat_t *)state.userdata;
	float cos_Ng_wo = sp.Ng*wo;
	float cos_Ng_wi;
	vector3d_t N = (cos_Ng_wo<0) ? -sp.N : sp.N;
	vector3d_t Hs;
	s.pdf = 0.f;
	float wiN = 0.f;
	float woN = std::fabs(wo * N);
	float cos_wo_H = 0.f;
	
	color_t scolor(0.f);

	float s1 = s.s1;
	float cur_pDiffuse = dat->pDiffuse;
	bool use_glossy = as_diffuse ? (s.flags & BSDF_DIFFUSE) : (s.flags & BSDF_GLOSSY);
	bool use_diffuse = with_diffuse && (s.flags & BSDF_DIFFUSE);
	nodeStack_t stack(dat->stack);
	float glossy = 0.f;
	
	if(use_diffuse)
	{
		float s_pDiffuse = use_glossy ? cur_pDiffuse : 1.f;
		if(s1 < s_pDiffuse)
		{
			s1 /= s_pDiffuse;
			wi = SampleCosHemisphere(N, sp.NU, sp.NV, s1, s.s2);
			wiN = std::fabs(wi * N);
			cos_Ng_wi = sp.Ng*wi;
			s.pdf = wiN;
			if(use_glossy)
			{
				vector3d_t H = (wi+wo).normalize();
				cos_wo_H = wo*H;
				float cos_N_H = N*H;
				if(anisotropic)
				{
					vector3d_t Hs(H*sp.NU, H*sp.NV, cos_N_H);
					s.pdf = s.pdf*cur_pDiffuse + AS_Aniso_Pdf(Hs, cos_wo_H, exp_u, exp_v)*(1.f-cur_pDiffuse);
					glossy = AS_Aniso_D(Hs, exp_u, exp_v) * SchlickFresnel(cos_wo_H, dat->mGlossy) / ( 8.f * std::fabs(cos_wo_H) * std::max(woN, wiN) );
				}
				else
				{
					s.pdf = s.pdf*cur_pDiffuse + Blinn_Pdf(cos_N_H, cos_wo_H, exponent)*(1.f-cur_pDiffuse);
					glossy = Blinn_D(cos_N_H, exponent) * SchlickFresnel(cos_wo_H, dat->mGlossy) / ( 8.f * std::fabs(cos_wo_H) * std::max(woN, wiN) );
				}
			}
			s.sampledFlags = BSDF_DIFFUSE | BSDF_REFLECT;
			scolor = glossy * (glossyS ? glossyS->getColor(stack) : gloss_color);
			return scolor;
		}
		s1 -= cur_pDiffuse;
		s1 /= (1.f - cur_pDiffuse);
	}
	
	if(use_glossy)
	{
		if(anisotropic)
		{
			AS_Aniso_Sample(Hs, s1, s.s2, exp_u, exp_v);
			vector3d_t H = Hs.x*sp.NU + Hs.y*sp.NV + Hs.z*N;
			cos_wo_H = wo*H;
			if ( cos_wo_H < 0.f )
			{
				H = reflect_plane(N, H);
				cos_wo_H = wo*H;
			}
			// Compute incident direction by reflecting wo about H
			wi = reflect_dir(H, wo);
			cos_Ng_wi = sp.Ng*wi;
			if(cos_Ng_wo*cos_Ng_wi < 0) return color_t(0.f);
			wiN = std::fabs(wi * N);

			s.pdf = AS_Aniso_Pdf(Hs, cos_wo_H, exp_u, exp_v);
			glossy = AS_Aniso_D(Hs, exp_u, exp_v) * SchlickFresnel(cos_wo_H, dat->mGlossy) / ( 8.f * std::fabs(cos_wo_H) * std::max(woN, wiN) );
		}
		else
		{
 			Blinn_Sample(Hs, s1, s.s2, exponent);
			vector3d_t H = Hs.x*sp.NU + Hs.y*sp.NV + Hs.z*N;
			cos_wo_H = wo*H;
			if ( cos_wo_H < 0.f )
			{
				H = reflect_plane(N, H);
				cos_wo_H = wo*H;
			}
			// Compute incident direction by reflecting wo about H
			wi = reflect_dir(H, wo);
			cos_Ng_wi = sp.Ng*wi;
			if(cos_Ng_wo*cos_Ng_wi < 0) return color_t(0.f);
			wiN = std::fabs(wi * N);

			s.pdf = Blinn_Pdf(Hs.z, cos_wo_H, exponent);
			glossy = Blinn_D(H*N, exponent) * SchlickFresnel(cos_wo_H, dat->mGlossy)  / ( 8.f * std::fabs(cos_wo_H) * std::max(woN, wiN) );
		}
		
		scolor = glossy * (glossyS ? glossyS->getColor(stack) : gloss_color);
		s.sampledFlags = as_diffuse ? BSDF_DIFFUSE | BSDF_REFLECT : BSDF_GLOSSY | BSDF_REFLECT;
	
		if(use_diffuse)
		{
			s.pdf = wiN * cur_pDiffuse + s.pdf * (1.f-cur_pDiffuse);
			PFLOAT f_wi = fPow(1.f - (0.5f * wiN), 5.f);
			PFLOAT f_wo = fPow(1.f - (0.5f * woN), 5.f);
			PFLOAT diffuse = DIFFUSE_RATIO * (1.f - dat->mGlossy) * (1.f - f_wi) * (1.f - f_wo);
			color_t diff_base = (diffuseS ? diffuseS->getColor(stack) : diff_color);
			scolor += (float)diffuse * dat->mDiffuse * diff_base;
		}
	}
	
	return scolor;
}

float glossyMat_t::pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t flags)const
{
	MDat_t *dat = (MDat_t *)state.userdata;
	if((sp.Ng * wo) * (sp.Ng * wi) < 0.f) return 0.f;
	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	float pdf = 0.f;
	float cos_wo_H = 0.f;
	float cos_N_H = 0.f;
	
	float cur_pDiffuse = dat->pDiffuse;
	bool use_glossy = as_diffuse ? (flags & BSDF_DIFFUSE) : (flags & BSDF_GLOSSY);
	bool use_diffuse = with_diffuse && (flags & BSDF_DIFFUSE);
	
	if(use_diffuse)
	{
		pdf = std::fabs(wi*N);
		if(use_glossy)
		{
			vector3d_t H = (wi+wo).normalize();
			cos_wo_H = wo*H;
			cos_N_H = N*H;
			if(anisotropic)
			{
				vector3d_t Hs(H*sp.NU, H*sp.NV, cos_N_H);
				pdf = pdf*cur_pDiffuse + AS_Aniso_Pdf(Hs, cos_wo_H, exp_u, exp_v)*(1.f-cur_pDiffuse);
			}
			else pdf = pdf*cur_pDiffuse + Blinn_Pdf(cos_N_H, cos_wo_H, exponent)*(1.f-cur_pDiffuse);
		}
	}
	else if(use_glossy)
	{
		vector3d_t H = (wi+wo).normalize();
		cos_wo_H = wo*H;
		cos_N_H = N*H;
		if(anisotropic)
		{
			vector3d_t Hs(H*sp.NU, H*sp.NV, cos_N_H);
			pdf = AS_Aniso_Pdf(Hs, cos_wo_H, exp_u, exp_v);
		}
		else pdf = Blinn_Pdf(cos_N_H, cos_wo_H, exponent);
	}
	return pdf;
}


bool glossyMat_t::scatterPhoton(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wi,
								vector3d_t &wo, pSample_t &s) const
{
	color_t scol = sample(state, sp, wi, wo, s);
	if(s.pdf > 1.0e-6f)
	{
		color_t cnew = s.lcol * s.alpha * scol * (std::fabs(wo*sp.N)/s.pdf);
		CFLOAT new_max = cnew.maximum();
		CFLOAT old_max = s.lcol.maximum();
		float prob = std::min(1.f, new_max/old_max);
		if(s.s3 <= prob)
		{
			s.color = cnew / prob;
			return true;
		}
	}
	return false;
}

material_t* glossyMat_t::factory(paraMap_t &params, std::list< paraMap_t > &paramList, renderEnvironment_t &render)
{
	color_t col(1.f), dcol(1.f);
	float refl=1.f;
	float diff=0.f;
	float exponent=50.f; //wild guess, do sth better
	bool as_diff=true;
	bool aniso=false;
	const std::string *name=0;
	params.getParam("color", col);
	params.getParam("diffuse_color", dcol);
	params.getParam("diffuse_reflect", diff);
	params.getParam("glossy_reflect", refl);
	params.getParam("as_diffuse", as_diff);
	params.getParam("exponent", exponent);
	params.getParam("anisotropic", aniso);
	glossyMat_t *mat = new glossyMat_t(col, dcol , refl, diff, exponent, as_diff);
	if(aniso)
	{
		float e_u=50.0, e_v=50.0;
		params.getParam("exp_u", e_u);
		params.getParam("exp_v", e_v);
		mat->anisotropic = true;
		mat->exp_u = e_u;
		mat->exp_v = e_v;
	}
	//shaderNode_t *diffuseS=0, *glossyS=0, *bumpS=0;
	std::vector<shaderNode_t *> roots;
	if(mat->loadNodes(paramList, render))
	{
		if(params.getParam("diffuse_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->diffuseS = i->second; roots.push_back(mat->diffuseS); }
			else std::cout << "[WARNING]: diffuse shader node '"<<*name<<"' does not exist!\n";
			std::cout << "diffuse shader: " << name << "(" << (void*)mat->diffuseS << ")\n";
		}
		if(params.getParam("glossy_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->glossyS = i->second; roots.push_back(mat->glossyS); }
			else std::cout << "[WARNING]: glossy shader node '"<<*name<<"' does not exist!\n";
			std::cout << "glossy shader: " << name << "(" << (void*)mat->glossyS << ")\n";
		}
		if(params.getParam("glossy_reflect_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->glossyRefS = i->second; roots.push_back(mat->glossyRefS); }
			else std::cout << "[WARNING]: glossy ref. shader node '"<<*name<<"' does not exist!\n";
			std::cout << "glossy ref. shader: " << name << "(" << (void*)mat->glossyRefS << ")\n";
		}
		if(params.getParam("bump_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->bumpS = i->second; roots.push_back(mat->bumpS); }
			else std::cout << "[WARNING]: bump shader node '"<<*name<<"' does not exist!\n";
			std::cout << "bump shader: " << name << "(" << (void*)mat->bumpS << ")\n";
		}
	}
	else std::cout << "loadNodes() failed!\n";
	// solve nodes order
	if(!roots.empty())
	{
		mat->solveNodesOrder(roots);
		std::cout << "evaluation order:\n";
		for(unsigned int k=0; k<mat->allSorted.size(); ++k) std::cout << (void*)mat->allSorted[k]<<"\n";
		std::vector<shaderNode_t *> colorNodes;
		if(mat->diffuseS) mat->getNodeList(mat->diffuseS, colorNodes);
		if(mat->glossyS) mat->getNodeList(mat->glossyS, colorNodes);
		if(mat->glossyRefS) mat->getNodeList(mat->glossyRefS, colorNodes);
		mat->filterNodes(colorNodes, mat->allViewdep, VIEW_DEP);
		mat->filterNodes(colorNodes, mat->allViewindep, VIEW_INDEP);
		if(mat->bumpS) mat->getNodeList(mat->bumpS, mat->bumpNodes);
	}
	mat->reqMem = mat->reqNodeMem + sizeof(MDat_t);
	return mat;
}

extern "C"
{	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("glossy", glossyMat_t::factory);
	}
}

__END_YAFRAY
