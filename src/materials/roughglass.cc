/****************************************************************************
 * 			roughglass.cc: a dielectric material with rough surface
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
#include <materials/roughglass.h>
#include <core_api/environment.h>
#include <materials/microfacet.h>
#include <utilities/mcqmc.h>
#include <yafraycore/spectrum.h>
#include <iostream>

__BEGIN_YAFRAY


roughGlassMat_t::roughGlassMat_t(float IOR, color_t filtC, const color_t &srcol, bool fakeS, float alpha, float disp_pow):
		bumpS(0), mirColS(0), filterCol(filtC), specRefCol(srcol), ior(IOR), a2(alpha*alpha), a(alpha), absorb(false),
		disperse(false), fakeShadow(fakeS)
{
	bsdfFlags = BSDF_ALL_GLOSSY;
	if(fakeS) bsdfFlags |= BSDF_FILTER;
	if(disp_pow > 0.0)
	{
		disperse = true;
		CauchyCoefficients(IOR, disp_pow, CauchyA, CauchyB);
		bsdfFlags |= BSDF_DISPERSIVE;
	}
}

void roughGlassMat_t::initBSDF(const renderState_t &state, const surfacePoint_t &sp, BSDF_t &bsdfTypes)const
{
	nodeStack_t stack(state.userdata);
	if(bumpS) evalBump(stack, state, sp, bumpS);
	
	//eval viewindependent nodes
	std::vector<shaderNode_t *>::const_iterator iter, end=allViewindep.end();
	for(iter = allViewindep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp);
	bsdfTypes=bsdfFlags;
}

/*
color_t roughGlassMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	nodeStack_t stack(state.userdata);
	if( !(s.flags & BSDF_GLOSSY) && !((s.flags & bsdfFlags & BSDF_DISPERSIVE) && state.chromatic) )
	{
		s.pdf = 0.f;
		return color_t(0.f);
	}
	vector3d_t refdir, N = FACE_FORWARD(sp.Ng, sp.N, wo);
	bool outside = sp.Ng*wo > 0;
	s.pdf = 1.f;

	float alpha2 = a2;
	float cosTheta, tanTheta2;
	vector3d_t Hs;
	GGX_Sample(Hs, alpha2, cosTheta, tanTheta2, s.s1, s.s2);
	vector3d_t H = Hs.x*sp.NU + Hs.y*sp.NV + Hs.z*N;
	cosTheta = H*N;
	
	float cur_ior = (disperse && state.chromatic) ? getIOR(state.wavelength, CauchyA, CauchyB) : ior;
	float glossy, glossy_D, Ht2;
	float wiN;
	float wiH;
	float woN = wo*N;
	float woH = wo*H;
	
	glossy_D = GGX_D(alpha2, cosTheta, tanTheta2);
	
	if( refract(H, wo, refdir, cur_ior) )
	{
		CFLOAT Kr, Kt;
		fresnel(wo, H, cur_ior, Kr, Kt);
		float pKr = 0.01+0.99*Kr, pKt = 0.01+0.99*Kt;
		if( true )
		{
			wi = refdir;
			wiN = wi*N;
			wiH = wi*H;
			glossy = ((std::fabs(woH * wiH)) * pKt * GGX_G(alpha2, wiN, woN) * glossy_D);
			if(outside)
			{
				Ht2 = ior * woH + wiH;
				glossy *= (ior * ior) / (woN * Ht2);
			}
			else
			{
				Ht2 = woH + ior * wiH;
				glossy /= (woN * Ht2);
			}
			s.pdf = GGX_Pdf_refracted(glossy_D, cosTheta, wiH, ior, Ht2, outside);
			s.sampledFlags = ((disperse && state.chromatic) ? BSDF_DISPERSIVE : BSDF_GLOSSY) | BSDF_TRANSMIT;
			return filterCol * glossy;//(Kt/std::fabs(sp.N*wi));
		}
		else // Reflection
		{
			wi = reflect_plane(H, wo);
			wiN = wi*N;
			wiH = wi*H;
			s.pdf = GGX_Pdf_reflected(glossy_D, cosTheta, wiH);
			s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;
			glossy = (pKr * GGX_G(alpha2, woH, wiH) * glossy_D) / (4.f * std::fabs(woN) * std::fabs(wiN));
			return (mirColS ? mirColS->getColor(stack) : specRefCol) * glossy; // * (Kr/std::fabs(sp.N*wi));
		}
	}
	else // TIR
	{
		wi = reflect_plane(H, wo);
		wiN = wi*N;
		wiH = wi*H;
		s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;
		color_t tir_col(1.f/std::fabs(sp.N*wi));
		return tir_col;
	}
	
	s.pdf = 0.f;
	return color_t(0.f);
}
*/
/*

color_t roughGlassMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	nodeStack_t stack(state.userdata);
	if( !(s.flags & BSDF_GLOSSY) && !((s.flags & bsdfFlags & BSDF_DISPERSIVE) && state.chromatic) )
	{
		s.pdf = 0.f;
		return color_t(0.f);
	}
	vector3d_t refdir, N;
	bool outside = sp.Ng*wo > 0;
	PFLOAT cos_wo_N = sp.N*wo;
	if(outside)	N = (cos_wo_N >= 0) ? sp.N : (sp.N - (1.00001*cos_wo_N)*wo).normalize();
	else		N = (cos_wo_N <= 0) ? sp.N : (sp.N - (1.00001*cos_wo_N)*wo).normalize();
	s.pdf = 1.f;

	float alpha2 = a2;
	float cosTheta, tanTheta2;
	vector3d_t Hs;
	GGX_Sample(Hs, cosTheta, tanTheta2, alpha2, s.s1, s.s2);
	vector3d_t H = Hs.x*sp.NU + Hs.y*sp.NV + Hs.z*N;
	
	float cur_ior = (disperse && state.chromatic) ? getIOR(state.wavelength, CauchyA, CauchyB) : ior;
	float glossy, glossy_D, Ht2;
	float wiN, woN, wiH, WoH;
	
	if( refract(H, wo, refdir, cur_ior) )
	{
		CFLOAT Kr, Kt;
		fresnel(wo, H, cur_ior, Kr, Kt);
		float pKr = 0.01+0.99*Kr, pKt = 0.01+0.99*Kt;
		if(s.s1 < pKt)
		{
			wi = refdir;
			s.pdf = pKt;
			s.sampledFlags = ((disperse && state.chromatic) ? BSDF_DISPERSIVE : BSDF_GLOSSY) | BSDF_TRANSMIT;
			return filterCol*(Kt/std::fabs(sp.N*wi));
		}
		else //total inner reflection
		{
			wi = reflect_plane(H, wo);
			s.pdf = pKr;
			s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;
			return (mirColS ? mirColS->getColor(stack) : specRefCol) * (Kr/std::fabs(sp.N*wi));
		}
	}
	else//total inner reflection
	{
		wi = reflect_plane(H, wo);
		s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;
		color_t tir_col(1.f/std::fabs(sp.N*wi));
		return tir_col;
	}
	s.pdf = 0.f;
	return color_t(0.f);
}


*/
color_t roughGlassMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	color_t col(0.f);
	s.pdf = 1.f;
	if( !(s.flags & BSDF_GLOSSY) && !((s.flags & bsdfFlags & BSDF_DISPERSIVE) && state.chromatic) ) return col;
	
	nodeStack_t stack(state.userdata);
	
	vector3d_t N = sp.N;//FACE_FORWARD(sp.Ng, sp.N, wo);//
	float woH, wiH, woN, wiN;
	bool outside;
	float alpha2 = a2;
	float cosTheta, tanTheta2;
	vector3d_t Hs, H, refdir, transdir;
	float glossy, glossy_D, glossy_G, Jacobian;
	float Ft, Fr;
	float cur_ior;
	float s1 = s.s1;
	
	GGX_Sample(Hs, alpha2, s1, s.s2);
	vector3d_t NU, NV;
	createCS(N, NU, NV);
	H = Hs.x*NU + Hs.y*NV + Hs.z*N;
	
	cosTheta = H*N;
	
	if(cosTheta <= 0.f) return col;
	
	float cosTheta2 = cosTheta * cosTheta;
	tanTheta2 = (1.f - cosTheta2) / cosTheta2;
	
	cur_ior = disperse ? getIOR(state.wavelength, CauchyA, CauchyB) : ior;
	
	if(cosTheta > 0) glossy_D = GGX_D(alpha2, cosTheta, tanTheta2);
	else glossy_D = 0.f;
	
	woN = wo*N;
	woH = wo*H;
	
	Jacobian = 0.f;
	
	if(dielectricFresnelRefract(cur_ior, cur_ior, H, wo, refdir, transdir, outside, Ft, Fr))
	{
		if(s1 < Ft)
		{
			wi = transdir;
			wiH = wi*H;
			wiN = wi*N;
			
			if(outside)
			{
				float ht = (ior * woH) + wiH;
				Jacobian = 1.f / (ht * ht);
			}
			else
			{
				float ht = woH + (ior * wiH);
				Jacobian = (ior * ior) / (ht * ht);
			}
			
			if((wiH/wiN) > 0 && (woH/woN) > 0) glossy_G = GGX_G(alpha2, wiN, woN);
			else glossy_G = 0.f;
			glossy = std::fabs( (woH * wiH) / (wiN * woN) ) * Ft * glossy_G * glossy_D * Jacobian;
			s.sampledFlags = ((disperse && state.chromatic) ? BSDF_DISPERSIVE : BSDF_GLOSSY)| BSDF_TRANSMIT;
			col = filterCol * glossy;
		}
		else
		{
			wi = refdir;
			wiH = wi*H;
			wiN = wi*N;
			if((wiH/wiN) > 0 && (woH/woN) > 0) glossy_G = GGX_G(alpha2, wiN, woN);
			else glossy_G = 0.f;
			Jacobian = 1.f / (4.f * std::fabs(woH));
			glossy = (Fr * glossy_G * glossy_D ) / (4.f * std::fabs(woN * wiN));
			s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;
			col = glossy * specRefCol;
		}
	}
	else
	{
		wi = refdir;
		wiH = wi*H;
		wiN = wi*N;
		if((wiH/wiN) > 0 && (woH/woN) > 0) glossy_G = GGX_G(alpha2, wiN, woN);
		else glossy_G = 0.f;
		Jacobian = 1.f / (4.f * std::fabs(woH));
		glossy = (Fr * glossy_G * glossy_D) / (4.f * std::fabs(woN * wiN));
		s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;
		col = glossy;
	}

	s.pdf = GGX_Pdf(glossy_D, cosTheta, Jacobian);
	return col;
}

color_t roughGlassMat_t::getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	float Kr, Kt;
	fresnel(wo, N, ior, Kr, Kt);
	return Kt*filterCol;
}

float roughGlassMat_t::getAlpha(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	float alpha = std::max(0.f, std::min(1.f, 1.f - getTransparency(state, sp, wo).energy()));
	return alpha;
}

float roughGlassMat_t::getMatIOR() const
{
	return ior;
}

material_t* roughGlassMat_t::factory(paraMap_t &params, std::list< paraMap_t > &paramList, renderEnvironment_t &render)
{
	float IOR=1.4;
	float filt=0.f;
	float alpha = 0.5f;
	float disp_power=0.0;
	color_t filtCol(1.f), absorp(1.f), srCol(1.f);
	const std::string *name=0;
	bool fake_shad = false;
	params.getParam("IOR", IOR);
	params.getParam("filter_color", filtCol);
	params.getParam("transmit_filter", filt);
	params.getParam("mirror_color", srCol);
	params.getParam("alpha", alpha);
	params.getParam("dispersion_power", disp_power);
	params.getParam("fake_shadows", fake_shad);
	
	alpha = std::max(1e-6f, std::min(alpha, 1.f));
	
	roughGlassMat_t *mat = new roughGlassMat_t(IOR, filt*filtCol + color_t(1.f-filt), srCol, fake_shad, alpha, disp_power);
	
	if( params.getParam("absorption", absorp) )
	{
		double dist=1.f;
		if(absorp.R < 1.f || absorp.G < 1.f || absorp.B < 1.f)
		{
			//deprecated method:
			color_t sigma(0.f);
			if(params.getParam("absorption_dist", dist))
			{
				const float maxlog = log(1e38);
				sigma.R = (absorp.R > 1e-38) ? -log(absorp.R) : maxlog;
				sigma.G = (absorp.G > 1e-38) ? -log(absorp.G) : maxlog;
				sigma.B = (absorp.B > 1e-38) ? -log(absorp.B) : maxlog;
				if (dist!=0.f) sigma *= 1.f/dist;
			}
			mat->absorb = true;
			mat->beer_sigma_a = sigma;
			mat->bsdfFlags |= BSDF_VOLUMETRIC;
			// creat volume handler (backwards compatibility)
			if(params.getParam("name", name))
			{
				paraMap_t map;
				map["type"] = std::string("beer");
				map["absorption_col"] = absorp;
				map["absorption_dist"] = parameter_t(dist);
				mat->volI = render.createVolumeH(*name, map);
				mat->bsdfFlags |= BSDF_VOLUMETRIC;
			}
		}
	}

	std::vector<shaderNode_t *> roots;
	std::map<std::string, shaderNode_t *> nodeList;
	std::map<std::string, shaderNode_t *>::iterator actNode;
	
	// Prepare our node list
	nodeList["mirror_color_shader"] = NULL;
	nodeList["bump_shader"] = NULL;
	
	if(mat->loadNodes(paramList, render))
	{
		for(actNode = nodeList.begin(); actNode != nodeList.end(); actNode++)
		{
			if(params.getParam(actNode->first, name))
			{
				std::map<std::string,shaderNode_t *>::const_iterator i = mat->shader_table.find(*name);
				
				if(i!=mat->shader_table.end())
				{
					actNode->second = i->second;
					roots.push_back(actNode->second);
				}
				else Y_WARNING << "RoughGlass: Shader node " << actNode->first << " '" << *name << "' does not exist!" << yendl;
			}
		}
	}
	else Y_ERROR << "RoughGlass: loadNodes() failed!" << yendl;

	mat->mirColS = nodeList["mirror_color_shader"];
	mat->bumpS = nodeList["bump_shader"];

	// solve nodes order
	if(!roots.empty())
	{
		mat->solveNodesOrder(roots);
		std::vector<shaderNode_t *> colorNodes;
		if(mat->mirColS) mat->getNodeList(mat->mirColS, colorNodes);
		mat->filterNodes(colorNodes, mat->allViewdep, VIEW_DEP);
		mat->filterNodes(colorNodes, mat->allViewindep, VIEW_INDEP);
		if(mat->bumpS)
		{
			mat->getNodeList(mat->bumpS, mat->bumpNodes);
		}
	}
	mat->reqMem = mat->reqNodeMem;

	return mat;
}


/* 
extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("rough_glass", roughGlassMat_t::factory);
	}
}
 */
__END_YAFRAY
