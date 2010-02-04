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


roughGlassMat_t::roughGlassMat_t(float IOR, color_t filtC, const color_t &srcol, bool fakeS, float exp, float disp_pow):
		bumpS(0), mirColS(0), filterCol(filtC), specRefCol(srcol), ior(IOR), exponent(exp), absorb(false),
		disperse(false), fakeShadow(fakeS)
{
	bsdfFlags = BSDF_ALL_GLOSSY;
	if(fakeS) bsdfFlags |= BSDF_FILTER;
	tmFlags = fakeS ? BSDF_FILTER | BSDF_TRANSMIT : BSDF_GLOSSY | BSDF_TRANSMIT;
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

color_t roughGlassMat_t::eval(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const
{
	nodeStack_t stack(state.userdata);
	float cos_Ng_wo = sp.Ng*wo;
	float cos_Ng_wi = sp.Ng*wi;
	vector3d_t N = sp.N;
	color_t col(0.f);
	if( !(bsdfs & BSDF_GLOSSY) ) return col;
	
	bool transmit = ( cos_Ng_wo * cos_Ng_wi ) < 0;
	bool outside = cos_Ng_wo > 0;
	
	float glossy, Kr, Kt = 1.f;
	vector3d_t H, dummy;
	
	if(!transmit)
	{
		//find normal at which transmission from wo to wi happens:
		bool can_tr;
		if(outside)
		{
			can_tr = inv_refract_test(H, wo, wi, ior);
		}
		else
		{
			can_tr = inv_refract_test(H, wi, wo, ior);
		}
		if(can_tr)
		{
			glossy = Blinn_D(H*N, exponent)/( 8.f * std::fabs(H*N) * std::max(std::fabs(wo*N), std::fabs(wi*N)) );
			fresnel(wo, H, ior, Kr, Kt); //wo or wi?
			col = filterCol * Kt * glossy;
		}
	}
	else
	{
		H = (wo + wi).normalize(); // half-angle
		glossy = Blinn_D(H*N, exponent)/( 8.f * std::fabs(H*N) * std::max(std::fabs(wo*N), std::fabs(wi*N)) );;
		bool can_tr = refract(N, wo, dummy, ior); //probably make specialized function with less overhead...
		if(can_tr)
		{
			fresnel(wo, H, ior, Kr, Kt); //outside!?
			col = (mirColS ? mirColS->getColor(stack) : specRefCol) * Kr * glossy;
		}
		else // total internal reflection
		{	
			col = glossy;
		}
	}
	return col;
}

color_t roughGlassMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s)const
{
	nodeStack_t stack(state.userdata);
	vector3d_t N = sp.N;
	vector3d_t Hs;
	float s1;
	bool transmit;

	if(s.s1 <= 0.7f){ s1 = s.s1 * (1.428571429f); transmit = true; }
	else { s1 = (s.s1 - 0.7f) * (3.333333333f); transmit = false; }
	
	Blinn_Sample(Hs, s1, s.s2, exponent);
	vector3d_t H = Hs.x*sp.NU + Hs.y*sp.NV + Hs.z*N;
	
	float glossy;
	color_t col(0.f);
	
	float cos_wo_H = wo*H;
	if (cos_wo_H < 0 ){ cos_wo_H = -cos_wo_H; }
	
	vector3d_t refdir;
	PFLOAT cur_ior = disperse ? getIOR(state.wavelength, CauchyA, CauchyB) : ior;

	if( refract(H, wo, refdir, cur_ior) )
	{
		CFLOAT Kr, Kt;
		fresnel(wo, H, ior, Kr, Kt);
		if(transmit)
		{
			wi = refdir;
			glossy = Blinn_D(H*N, exponent) / ( 8.f * std::fabs(cos_wo_H) * std::max(std::fabs(wo*N), std::fabs(wi*N)) );
			s.pdf = 0.7f * Blinn_Pdf(H*N, cos_wo_H, exponent);
			if(disperse && state.chromatic)
			{
				s.sampledFlags = BSDF_DISPERSIVE | BSDF_TRANSMIT;
			}
			else s.sampledFlags = BSDF_GLOSSY | BSDF_TRANSMIT;
			col = filterCol * Kt * glossy;
		}
		else
		{
			wi = reflect_plane(H, wo);
			glossy = Blinn_D(H*N, exponent) / ( 8.f * std::fabs(cos_wo_H) * std::max(std::fabs(wo*N), std::fabs(wi*N)) );
			s.pdf = 0.3f * Blinn_Pdf(H*N, cos_wo_H, exponent);
			s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;
			col = (mirColS ? mirColS->getColor(stack) : specRefCol) * Kr * glossy;
		}
	}
	else //total inner reflection
	{
		wi = reflect_plane(N, wo);
		glossy = Blinn_D(H*N, exponent) / ( 8.f * std::fabs(cos_wo_H) * std::max(std::fabs(wo*N), std::fabs(wi*N)) );
		s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;
		s.pdf = Blinn_Pdf(H*N, cos_wo_H, exponent);
		col = glossy;
	}
	return col;
}


float roughGlassMat_t::pdf(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, const vector3d_t &wi, BSDF_t bsdfs)const
{
	PFLOAT cos_Ng_wo = sp.Ng*wo;
	PFLOAT cos_Ng_wi = sp.Ng*wi;
	vector3d_t N = (cos_Ng_wo<0) ? -sp.N : sp.N;
	float pdf = 0.f;
	if( !(bsdfs & BSDF_GLOSSY) ) return 0.f;
	
	bool transmit = ( cos_Ng_wo * cos_Ng_wi ) < 0;
	bool outside = cos_Ng_wo > 0;
	vector3d_t H, dummy;
	
	if(!transmit)
	{
		bool can_tr;
		if(outside) can_tr = inv_refract_test(H, wo, wi, ior);
		else can_tr = inv_refract_test(H, wi, wo, ior);
		if(can_tr)
		{
			pdf = 0.7f * Blinn_Pdf(H*N, std::fabs(wo*H), exponent); //wo?wi?abs?
		}
	}
	else
	{
		H = (wo + wi).normalize(); // half-angle
		float glossy = Blinn_Pdf(H*N, std::fabs(wo*H), exponent);
		bool can_tr = refract_test(N, wo, dummy, ior);
		if(can_tr)
		{
			pdf = 0.3f * glossy;
		}
		else // total internal reflection
		{	
			pdf = glossy;
		}
	}
	return pdf;
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

bool roughGlassMat_t::volumeTransmittance(const renderState_t &state, const surfacePoint_t &sp, const ray_t &ray, color_t &col)const
{
	// absorption due to beer's law (simple RGB based)
	if(absorb)
	{
		// outgoing dir is -ray.dir, we check if it coming from backside (i.e. wo*Ng < 0)
		// so ray.dir*Ng needs to be larger than 0...
		if( (ray.dir * sp.Ng) < 0.f )
		{
			if(ray.tmax < 0.f || ray.tmax > 1e30f) //infinity check...
			{
				col = color_t( 0.f, 0.f, 0.f);
				return true;
			}
			float dist = ray.tmax; // maybe substract ray.tmin...
			color_t be(-dist*beer_sigma_a);
			col = color_t( fExp(be.getR()), fExp(be.getG()), fExp(be.getB()) );
			return true;
		}
	}
	return false;
}

float roughGlassMat_t::getMatIOR() const
{
	return ior;
}

material_t* roughGlassMat_t::factory(paraMap_t &params, std::list< paraMap_t > &paramList, renderEnvironment_t &render)
{
	double IOR=1.4;
	double filt=0.f;
	double exponent = 50.0;
	double disp_power=0.0;
	color_t filtCol(1.f), absorp(1.f), srCol(1.f);
	const std::string *name=0;
	bool fake_shad = false;
	params.getParam("IOR", IOR);
	params.getParam("filter_color", filtCol);
	params.getParam("transmit_filter", filt);
	params.getParam("mirror_color", srCol);
	params.getParam("exponent", exponent);
	params.getParam("dispersion_power", disp_power);
	params.getParam("fake_shadows", fake_shad);
	if(exponent < 2.f) exponent = 2.f;
	roughGlassMat_t *mat = new roughGlassMat_t(IOR, filt*filtCol + color_t(1.f-filt), srCol, fake_shad, exponent, disp_power);
	if( params.getParam("absorption", absorp) )
	{
		double dist=1.f;
		if(absorp.R < 1.f || absorp.G < 1.f || absorp.B < 1.f)
		{
			//deprecated method:
			color_t sigma(0.f);
			if(params.getParam("absorption_dist", dist))
			{
				const CFLOAT maxlog = log(1e38);
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
