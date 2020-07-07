/****************************************************************************
 * 		roughglass.cc: a dielectric material with rough surface
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
#include <core_api/color_ramp.h>
#include <core_api/params.h>
#include <iostream>

__BEGIN_YAFRAY


roughGlassMat_t::roughGlassMat_t(float IOR, color_t filtC, const color_t &srcol, bool fakeS, float alpha, float disp_pow, visibility_t eVisibility):
		filterCol(filtC), specRefCol(srcol), ior(IOR), a2(alpha*alpha), a(alpha), fakeShadow(fakeS), dispersion_power(disp_pow)
{
    mVisibility = eVisibility;
	bsdfFlags = BSDF_ALL_GLOSSY;
	if(fakeS) bsdfFlags |= BSDF_FILTER;
	if(disp_pow > 0.0)
	{
		disperse = true;
		CauchyCoefficients(IOR, disp_pow, CauchyA, CauchyB);
		bsdfFlags |= BSDF_DISPERSIVE;
	}
	
	mVisibility = eVisibility;
}

void roughGlassMat_t::initBSDF(const renderState_t &state, surfacePoint_t &sp, BSDF_t &bsdfTypes) const
{
	nodeStack_t stack(state.userdata);
	if(bumpS) evalBump(stack, state, sp, bumpS);

	//eval viewindependent nodes
	auto end=allViewindep.end();
	for(auto iter = allViewindep.begin(); iter!=end; ++iter) (*iter)->eval(stack, state, sp);
	bsdfTypes=bsdfFlags;
}

color_t roughGlassMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t &wi, sample_t &s, float &W) const
{
	nodeStack_t stack(state.userdata);
	vector3d_t refdir, N = FACE_FORWARD(sp.Ng, sp.N, wo);
	bool outside = sp.Ng * wo > 0.f;

	s.pdf = 1.f;

    float alpha_texture = roughnessS ? roughnessS->getScalar(stack)+0.001f : 0.001f;
	float alpha2 = roughnessS ? alpha_texture*alpha_texture : a2;
	float cosTheta, tanTheta2;

	vector3d_t H(0.f);
	GGX_Sample(H, alpha2, s.s1, s.s2);
	H = H.x*sp.NU + H.y*sp.NV + H.z*N;
	H.normalize();

    float cur_ior = ior;

    if(iorS)
    {
        cur_ior += iorS->getScalar(stack);
    }

    if(disperse && state.chromatic)
    {   
        float cur_cauchyA = CauchyA;
        float cur_cauchyB = CauchyB;

        if(iorS) CauchyCoefficients(cur_ior, dispersion_power, cur_cauchyA, cur_cauchyB);
        cur_ior = getIOR(state.wavelength, cur_cauchyA, cur_cauchyB);
    }

	float glossy;
	float glossy_D = 0.f;
	float glossy_G = 0.f;
	float wiN, woN, wiH, woH;
	float Jacobian = 0.f;

	cosTheta = H * N;
	float cosTheta2 = cosTheta * cosTheta;
	tanTheta2 = (1.f - cosTheta2) / std::max(1.0e-8f,cosTheta2);

	if(cosTheta > 0.f) glossy_D = GGX_D(alpha2, cosTheta2, tanTheta2);

	woH = wo*H;
	woN = wo*N;

	float Kr, Kt;

	color_t ret(0.f);

	if(refractMicrofacet(((outside) ? 1.f / cur_ior : cur_ior), wo, wi, H, woH, woN, Kr, Kt) )
	{
		if(s.s1 < Kt && (s.flags & BSDF_TRANSMIT))
		{
			wiN = wi*N;
			wiH = wi*H;

			if((wiH*wiN) > 0.f && (woH*woN) > 0.f) glossy_G = GGX_G(alpha2, wiN, woN);

			float IORwi = 1.f;
			float IORwo = 1.f;

			if(outside)	IORwi = cur_ior;
			else IORwo = cur_ior;

			float ht = IORwo * woH + IORwi * wiH;
			Jacobian = (IORwi * IORwi) / std::max(1.0e-8f, ht * ht);

			glossy = std::fabs( (woH * wiH) / (wiN * woN) ) * Kt * glossy_G * glossy_D * Jacobian;

			s.pdf = GGX_Pdf(glossy_D, cosTheta, Jacobian * std::fabs(wiH));
			s.sampledFlags = ((disperse && state.chromatic) ? BSDF_DISPERSIVE : BSDF_GLOSSY) | BSDF_TRANSMIT;

			ret = (glossy * (filterColS ? filterColS->getColor(stack) : filterCol));
			W = std::fabs(wiN) / std::max(0.1f,s.pdf); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
		}
		else if(s.flags & BSDF_REFLECT)
		{
            reflectMicrofacet(wo, wi, H, woH);

			wiN = wi*N;
			wiH = wi*H;

			glossy_G = GGX_G(alpha2, wiN, woN);

			Jacobian = 1.f / std::max(1.0e-8f,(4.f * std::fabs(wiH)));
			glossy = (Kr * glossy_G * glossy_D) / std::max(1.0e-8f,(4.f * std::fabs(woN * wiN)));

			s.pdf = GGX_Pdf(glossy_D, cosTheta, Jacobian);
			s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;

			ret = (glossy * (mirColS ? mirColS->getColor(stack) : specRefCol));

			W = std::fabs(wiN) / std::max(0.1f,s.pdf); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
		}
	}
	else // TIR
	{
		wi = wo;
		wi.reflect(H);
		s.sampledFlags = BSDF_GLOSSY | BSDF_REFLECT;
		ret = 1.f;
		W = 1.f;
	}

	float wireFrameAmount = (mWireFrameShader ? mWireFrameShader->getScalar(stack) * mWireFrameAmount : mWireFrameAmount);
	applyWireFrame(ret, wireFrameAmount, sp);

	return ret;
}

color_t roughGlassMat_t::sample(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, vector3d_t *const dir, color_t &tcol, sample_t &s, float *const W)const
{
	nodeStack_t stack(state.userdata);
	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	bool outside = sp.Ng * wo > 0.f;

	s.pdf = 1.f;

    float alpha_texture = roughnessS ? roughnessS->getScalar(stack)+0.001f : 0.001f;
	float alpha2 = roughnessS ? alpha_texture*alpha_texture : a2;
	float cosTheta, tanTheta2;

	vector3d_t H(0.f);
	vector3d_t wi;
	GGX_Sample(H, alpha2, s.s1, s.s2);
	H = H.x*sp.NU + H.y*sp.NV + H.z*N;
	H.normalize();

    float cur_ior = ior;

    if(iorS)
    {
        cur_ior += iorS->getScalar(stack);
    }

    if(disperse && state.chromatic)
    {   
        float cur_cauchyA = CauchyA;
        float cur_cauchyB = CauchyB;

        if(iorS) CauchyCoefficients(cur_ior, dispersion_power, cur_cauchyA, cur_cauchyB);
        cur_ior = getIOR(state.wavelength, cur_cauchyA, cur_cauchyB);
    }
    
	float glossy;
	float glossy_D = 0.f;
	float glossy_G = 0.f;
	float wiN, woN, wiH, woH;
	float Jacobian = 0.f;

	cosTheta = H * N;
	float cosTheta2 = cosTheta * cosTheta;
	tanTheta2 = (1.f - cosTheta2) / std::max(1.0e-8f,cosTheta2);

	if(cosTheta > 0.f) glossy_D = GGX_D(alpha2, cosTheta2, tanTheta2);

	woH = wo*H;
	woN = wo*N;

	float Kr, Kt;

	color_t ret(0.f);
	s.sampledFlags = 0;

	if(refractMicrofacet(((outside) ? 1.f / cur_ior : cur_ior), wo, wi, H, woH, woN, Kr, Kt) )
	{
		if(s.flags & BSDF_TRANSMIT)
		{
			wiN = wi*N;
			wiH = wi*H;

			if((wiH*wiN) > 0.f && (woH*woN) > 0.f) glossy_G = GGX_G(alpha2, wiN, woN);

			float IORwi = 1.f;
			float IORwo = 1.f;

			if(outside)	IORwi = cur_ior;
			else IORwo = cur_ior;

			float ht = IORwo * woH + IORwi * wiH;
			Jacobian = (IORwi * IORwi) / std::max(1.0e-8f, ht * ht);

			glossy = std::fabs( (woH * wiH) / (wiN * woN) ) * Kt * glossy_G * glossy_D * Jacobian;

			s.pdf = GGX_Pdf(glossy_D, cosTheta, Jacobian * std::fabs(wiH));
			s.sampledFlags = ((disperse && state.chromatic) ? BSDF_DISPERSIVE : BSDF_GLOSSY) | BSDF_TRANSMIT;

			ret = (glossy * (filterColS ? filterColS->getColor(stack) : filterCol));
			W[0] = std::fabs(wiN) / std::max(0.1f,s.pdf); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
			dir[0] = wi;

		}
		if(s.flags & BSDF_REFLECT)
		{
			reflectMicrofacet(wo, wi, H, woH);

			wiN = wi*N;
			wiH = wi*H;

			glossy_G = GGX_G(alpha2, wiN, woN);

			Jacobian = 1.f / std::max(1.0e-8f,(4.f * std::fabs(wiH)));
			glossy = (Kr * glossy_G * glossy_D) / std::max(1.0e-8f,(4.f * std::fabs(woN * wiN)));

			s.pdf = GGX_Pdf(glossy_D, cosTheta, Jacobian);
			s.sampledFlags |= BSDF_GLOSSY | BSDF_REFLECT;

			tcol = (glossy * (mirColS ? mirColS->getColor(stack) : specRefCol));

			W[1] = std::fabs(wiN) / std::max(0.1f,s.pdf); //FIXME: I have to put a lower limit to s.pdf to avoid white dots (high values) piling up in the recursive render stage. Why is this needed?
			dir[1] = wi;
		}
	}
	else // TIR
	{
		wi = wo;
		wi.reflect(H);
		s.sampledFlags |= BSDF_GLOSSY | BSDF_REFLECT;
		dir[0] = wi;
		ret = 1.f;
		W[0] = 1.f;
	}

	float wireFrameAmount = (mWireFrameShader ? mWireFrameShader->getScalar(stack) * mWireFrameAmount : mWireFrameAmount);
	applyWireFrame(ret, wireFrameAmount, sp);

	return ret;
}

color_t roughGlassMat_t::getTransparency(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
	nodeStack_t stack(state.userdata);
	vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
	float Kr, Kt;
	fresnel(wo, N, (iorS ? iorS->getScalar(stack):ior), Kr, Kt);
	color_t result = Kt*(filterColS ? filterColS->getColor(stack) : filterCol);

    float wireFrameAmount = (mWireFrameShader ? mWireFrameShader->getScalar(stack) * mWireFrameAmount : mWireFrameAmount);
    applyWireFrame(result, wireFrameAmount, sp);
    return result;	
}

float roughGlassMat_t::getAlpha(const renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo)const
{
    nodeStack_t stack(state.userdata);

	float alpha = std::max(0.f, std::min(1.f, 1.f - getTransparency(state, sp, wo).energy()));
	
	float wireFrameAmount = (mWireFrameShader ? mWireFrameShader->getScalar(stack) * mWireFrameAmount : mWireFrameAmount);
	applyWireFrame(alpha, wireFrameAmount, sp);
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
	const std::string *name = nullptr;
	bool fake_shad = false;
	std::string sVisibility = "normal";
	visibility_t visibility = NORMAL_VISIBLE;
	int mat_pass_index = 0;
	bool receive_shadows = true;
	int additionaldepth = 0;
	float samplingfactor = 1.f;
    float WireFrameAmount = 0.f;           //!< Wireframe shading amount   
    float WireFrameThickness = 0.01f;      //!< Wireframe thickness
    float WireFrameExponent = 0.f;         //!< Wireframe exponent (0.f = solid, 1.f=linearly gradual, etc)
    color_t WireFrameColor = color_t(1.f); //!< Wireframe shading color
	
	params.getParam("IOR", IOR);
	params.getParam("filter_color", filtCol);
	params.getParam("transmit_filter", filt);
	params.getParam("mirror_color", srCol);
	params.getParam("alpha", alpha);
	params.getParam("dispersion_power", disp_power);
	params.getParam("fake_shadows", fake_shad);
	
	params.getParam("receive_shadows", receive_shadows);
	params.getParam("visibility", sVisibility);
	params.getParam("mat_pass_index",   mat_pass_index);
	params.getParam("additionaldepth",   additionaldepth);
	params.getParam("samplingfactor",   samplingfactor);
	
    params.getParam("wireframe_amount",  WireFrameAmount);
    params.getParam("wireframe_thickness",  WireFrameThickness);
    params.getParam("wireframe_exponent",  WireFrameExponent);
    params.getParam("wireframe_color",  WireFrameColor);
	
	if(sVisibility == "normal") visibility = NORMAL_VISIBLE;
	else if(sVisibility == "no_shadows") visibility = VISIBLE_NO_SHADOWS;
	else if(sVisibility == "shadow_only") visibility = INVISIBLE_SHADOWS_ONLY;
	else if(sVisibility == "invisible") visibility = INVISIBLE;
	else visibility = NORMAL_VISIBLE;

	alpha = std::max(1e-4f, std::min(alpha * 0.5f, 1.f));

	roughGlassMat_t *mat = new roughGlassMat_t(IOR, filt*filtCol + color_t(1.f-filt), srCol, fake_shad, alpha, disp_power, visibility);

	mat->setMaterialIndex(mat_pass_index);
	mat->mReceiveShadows = receive_shadows;
	mat->additionalDepth = additionaldepth;

    mat->mWireFrameAmount = WireFrameAmount;
    mat->mWireFrameThickness = WireFrameThickness;
    mat->mWireFrameExponent = WireFrameExponent;
    mat->mWireFrameColor = WireFrameColor;

	mat->setSamplingFactor(samplingfactor);

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

	// Prepare our node list
	nodeList["mirror_color_shader"] = nullptr;
	nodeList["bump_shader"] = nullptr;
    nodeList["filter_color_shader"] = nullptr;
    nodeList["IOR_shader"] = nullptr;
    nodeList["wireframe_shader"] = nullptr;
    nodeList["roughness_shader"] = nullptr;
    
	if(mat->loadNodes(paramList, render))
	{
        mat->parseNodes(params, roots, nodeList);
	}
	else Y_ERROR << "RoughGlass: loadNodes() failed!" << yendl;

	mat->mirColS = nodeList["mirror_color_shader"];
	mat->bumpS = nodeList["bump_shader"];
    mat->filterColS = nodeList["filter_color_shader"];
    mat->iorS = nodeList["IOR_shader"];
    mat->mWireFrameShader = nodeList["wireframe_shader"];
    mat->roughnessS = nodeList["roughness_shader"];

	// solve nodes order
	if(!roots.empty())
	{
		mat->solveNodesOrder(roots);
		std::vector<shaderNode_t *> colorNodes;
		if(mat->mirColS) mat->getNodeList(mat->mirColS, colorNodes);
        if(mat->roughnessS) mat->getNodeList(mat->roughnessS, colorNodes);
        if(mat->iorS) mat->getNodeList(mat->iorS, colorNodes);
        if(mat->mWireFrameShader)    mat->getNodeList(mat->mWireFrameShader, colorNodes);
        if(mat->filterColS) mat->getNodeList(mat->filterColS, colorNodes);
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

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("rough_glass", roughGlassMat_t::factory);
	}
}

__END_YAFRAY
