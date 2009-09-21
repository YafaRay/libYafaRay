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
#include <iostream>

__BEGIN_YAFRAY


roughGlassMat_t::roughGlassMat_t(float IOR, color_t filtC, const color_t &srcol, bool fakeS, float exp):
		bumpS(0), mirColS(0), filterCol(filtC), specRefCol(srcol), ior(IOR), exponent(exp), absorb(false),
		disperse(false), fakeShadow(fakeS)
{
	bsdfFlags = BSDF_GLOSSY | BSDF_REFLECT | BSDF_TRANSMIT;
	if(fakeS) bsdfFlags |= BSDF_FILTER;
	tmFlags = fakeS ? BSDF_FILTER | BSDF_TRANSMIT : BSDF_GLOSSY | BSDF_TRANSMIT;
	/* if(disp_pow > 0.0)
	{
		disperse = true;
		CauchyCoefficients(IOR, disp_pow, CauchyA, CauchyB);
		bsdfFlags |= BSDF_DISPERSIVE;
	} */
}

void roughGlassMat_t::func()
{
	surfacePoint_t sp;
	sp.P = point3d_t(0,0,0);
	sp.N = sp.Ng = vector3d_t(0, 0, 1);
	createCS(sp.N, sp.NU, sp.NV);
	vector3d_t wi(0, 0, 0), wo, Hs;
	renderState_t state;
	state.userdata = malloc(USER_DATA_SIZE);
	sample_t s(0,0,BSDF_ALL);
	float reflect[32], transmit[32];
	for(int i=0; i<32; ++i)
	{
		wi.z = ((PFLOAT)i+0.5f)/32.f;
		wi.x = fSqrt(1.f - wi.z*wi.z);
		reflect[i] = 0, transmit[i] = 0;
		for(int j=0; j<32; ++j)
		{
			s.s1 = (float(j)+0.5f)/32.f;
			s.s2 = RI_vdC(j);
			Blinn_Sample(Hs, s.s1, s.s2, exponent);
			vector3d_t H = Hs.x*sp.NU + Hs.y*sp.NV + Hs.z*sp.N;
			PFLOAT cos_wo_H = wo*H;
			if (cos_wo_H < 0 ){ /* H = -H; */ cos_wo_H = -cos_wo_H; }
			s.pdf = Blinn_Pdf(Hs.z, cos_wo_H, exponent);
			vector3d_t refdir;
			CFLOAT xy =  1.f/( 8.f * std::fabs(cos_wo_H) * std::max(std::fabs(wo*sp.N), std::fabs(wi*sp.N)) );
			xy = Blinn_D(Hs.z, exponent) * xy;
			if( refract(H, wo, refdir, ior) )
			{
				CFLOAT Kr, Kt;
				fresnel(wo, H, ior, Kr, Kt);
				reflect[i] += Kr*xy/s.pdf;
				transmit[i] += Kt*xy/s.pdf;
			}
			else reflect[i] += xy/s.pdf;
			//color_t col = sample(state, sp, wi, wo, s);
			//if(s.pdf > 1e-6)
			//{
			//	if(s.sampledFlags & BSDF_REFLECT) ref += col * std::fabs(wo.z) / s.pdf;
			//	else trans += col * std::fabs(wo.z) / s.pdf;
			//}
		}
		//float sref = ref.energy()/32.f , strans = trans.energy()/32.f;
		//std::cout << "cos: " << wi.z << "\t ref: " << sref << 
		//	"\t trans: " << strans << "\tsum: " << sref+strans << std::endl;
	}
	
	free(state.userdata);
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

// n points in medium of wi!
// IOR applies to wo!
bool inv_refract(const vector3d_t &wi, const vector3d_t &wo, vector3d_t &n, PFLOAT IOR)
{
	n = (wi + IOR*wo).normalize();
	if(IOR > 1.f) n = -n;
	return (wi*n)*(wo*n) < 0.f;
}

color_t roughGlassMat_t::eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const
{
	nodeStack_t stack(state.userdata);
	PFLOAT cos_Ng_wo = sp.Ng*wo;
	PFLOAT cos_Ng_wi = sp.Ng*wi;
	vector3d_t N = (cos_Ng_wo<0) ? -sp.N : sp.N;
	color_t col(0.f);
	if(! (bsdfs & BSDF_GLOSSY) ) return col;
	
	bool transmit = ( cos_Ng_wo * cos_Ng_wi ) < 0;
	bool outside = cos_Ng_wo > 0;
	
	CFLOAT glossy, Kr, Kt;
	vector3d_t H, dummy;
	
	if(transmit)
	{
		//find normal at which transmission from wo to wi happens:
		bool can_tr;
		if(outside) can_tr = inv_refract(wo, wi, H, ior);
		else can_tr = inv_refract(wi, wo, H, ior);
		if(can_tr)
		{
			CFLOAT xy =  1.f/( 8.f * std::fabs(H*N) * std::max(std::fabs(wo*N), std::fabs(wi*N)) );
			glossy = Blinn_D(H*N, exponent);
			fresnel(wo, H, ior, Kr, Kt); //wo or wi?
			col = filterCol * Kt * glossy * xy;
		}
	}
	else
	{
		H = (wo + wi).normalize(); // half-angle
		CFLOAT xy =  1.f/( 8.f * std::fabs(H*N) * std::max(std::fabs(wo*N), std::fabs(wi*N)) );
		glossy = Blinn_D(H*N, exponent);
		bool can_tr = refract(sp.N, wo, dummy, ior); //probably make specialized function with less overhead...
		if(can_tr)
		{
			fresnel(wo, H, ior, Kr, Kt); //outside!?
			col = (mirColS ? mirColS->getColor(stack) : specRefCol) * Kr * glossy * xy;
		}
		else // total internal reflection
		{	
			col = glossy * xy;
		}
	}
	return col;
}

color_t roughGlassMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	nodeStack_t stack(state.userdata);
	vector3d_t N = /* (cos_Ng_wo<0) ? -sp.N : */ sp.N;
	vector3d_t Hs;
	float s1;
	bool transmit;
	if(s.s1 < 0.7f){ s1 = s.s1 * (1.f/.7f); transmit = true; }
	else { s1 = (s.s1 - 0.7f) * (1.f/.3f); transmit = false; }
	
	Blinn_Sample(Hs, s1, s.s2, exponent);
	vector3d_t H = Hs.x*sp.NU + Hs.y*sp.NV + Hs.z*N;
	PFLOAT cos_wo_H = wo*H;
	if (cos_wo_H < 0 ){ /* H = -H; */ cos_wo_H = -cos_wo_H; }
	vector3d_t refdir;
	if( refract(H, wo, refdir, ior) )
	{
		CFLOAT Kr, Kt;
		fresnel(wo, H, ior, Kr, Kt);
		if(transmit)
		{
			wi = refdir;
			CFLOAT xy =  1.f/( 8.f * std::fabs(cos_wo_H) * std::max(std::fabs(wo*N), std::fabs(wi*N)) );
			s.pdf = 0.7f * Blinn_Pdf(Hs.z, cos_wo_H, exponent);
			s.sampledFlags = BSDF_GLOSSY | BSDF_TRANSMIT;
			//if(cos_Ng_wo < 0) std::cout << "pdf:" << s.pdf << " col:" << filterCol * (Kt)  * Blinn_D(Hs.z, exponent) << std::endl;
			return filterCol * (Kt)  * Blinn_D(Hs.z, exponent) * xy;
		}
		else
		{
			wi = reflect_plane(H, wo);
			CFLOAT xy =  1.f/( 8.f * std::fabs(cos_wo_H) * std::max(std::fabs(wo*N), std::fabs(wi*N)) );
			s.pdf = 0.3f * Blinn_Pdf(Hs.z, cos_wo_H, exponent);
			s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;
			return (mirColS ? mirColS->getColor(stack) : specRefCol) * (Kr) * Blinn_D(Hs.z, exponent) * xy;
		}
	}
	else //total inner reflection
	{
		wi = reflect_plane(H, wo);
		CFLOAT xy =  1.f/( 8.f * std::fabs(cos_wo_H) * std::max(std::fabs(wo*N), std::fabs(wi*N)) );
		s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;
		s.pdf = Blinn_Pdf(Hs.z, cos_wo_H, exponent);
		return color_t( Blinn_D(Hs.z, exponent) * xy );
	}
}


float roughGlassMat_t::pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const
{
	PFLOAT cos_Ng_wo = sp.Ng*wo;
	PFLOAT cos_Ng_wi = sp.Ng*wi;
	vector3d_t N = (cos_Ng_wo<0) ? -sp.N : sp.N;
	float pdf = 0.f;
	if(! (bsdfs & BSDF_GLOSSY) ) return 0.f;
	
	bool transmit = ( cos_Ng_wo * cos_Ng_wi ) < 0;
	bool outside = cos_Ng_wo > 0;
	vector3d_t H, dummy;
	
	if(transmit)
	{
		bool can_tr;
		if(outside) can_tr = inv_refract(wo, wi, H, ior);
		else can_tr = inv_refract(wi, wo, H, ior);
		if(can_tr)
		{
			pdf = 0.5f * Blinn_Pdf(H*N, std::fabs(wo*H), exponent); //wo?wi?abs?
		}
	}
	else
	{
		H = (wo + wi).normalize(); // half-angle
		float glossy = Blinn_Pdf(H*N, wo*H, exponent);
		bool can_tr = refract(sp.N, wo, dummy, ior); //probably make specialized function with less overhead...
		if(can_tr)
		{
			pdf = 0.5f * glossy;
		}
		else // total internal reflection
		{	
			pdf = glossy;
		}
	}
	return pdf;
}



material_t* roughGlassMat_t::factory(paraMap_t &params, std::list< paraMap_t > &paramList, renderEnvironment_t &render)
{
	double IOR=1.4;
	double filt=0.f;
	double exponent = 50.0;
	//double disp_power=0.0;
	color_t filtCol(1.f), absorp(1.f), srCol(1.f);
	const std::string *name=0;
	bool fake_shad = false;
	params.getParam("IOR", IOR);
	params.getParam("filter_color", filtCol);
	params.getParam("transmit_filter", filt);
	params.getParam("mirror_color", srCol);
	params.getParam("exponent", exponent);
	//params.getParam("dispersion_power", disp_power);
	params.getParam("fake_shadows", fake_shad);
	roughGlassMat_t *mat = new roughGlassMat_t(IOR, filt*filtCol + color_t(1.f-filt), srCol, fake_shad, exponent);
	if( params.getParam("absorption", absorp) )
	{
		double dist=1.f;
		if(absorp.R < 1.f || absorp.G < 1.f || absorp.B < 1.f)
		{
			if(params.getParam("absorption_dist", dist))
			{
				const CFLOAT maxlog = log(1e38);
				absorp.R = (absorp.R > 1e-38) ? -log(absorp.R) : maxlog;
				absorp.G = (absorp.G > 1e-38) ? -log(absorp.G) : maxlog;
				absorp.B = (absorp.B > 1e-38) ? -log(absorp.B) : maxlog;
				if (dist!=0.f) absorp *= 1.f/dist;
			}
			mat->absorb = true;
			mat->beer_sigma_a = absorp;
			mat->bsdfFlags |= BSDF_VOLUMETRIC;
		}
	}
	std::vector<shaderNode_t *> roots;
	if(mat->loadNodes(paramList, render))
	{
		if(params.getParam("mirror_color_shader", name))
		{
			std::map<std::string,shaderNode_t *>::const_iterator i=mat->shader_table.find(*name);
			if(i!=mat->shader_table.end()){ mat->mirColS = i->second; roots.push_back(mat->mirColS); }
			else std::cout << "[WARNING]: mirror col. shader node '"<<*name<<"' does not exist!\n";
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
	//mat->func();
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
