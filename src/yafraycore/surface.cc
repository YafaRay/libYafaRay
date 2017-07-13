#include <core_api/surface.h>
#include <core_api/ray.h>
#include <utilities/interpolation.h>

__BEGIN_YAFRAY

spDifferentials_t::spDifferentials_t(const surfacePoint_t &spoint, const diffRay_t &ray): sp(spoint)
{
	if (ray.hasDifferentials)
	{
		// Estimate screen-space change in \pt and $(u,v)$
		// Compute auxiliary intersection points with plane
		float d = -(sp.N * vector3d_t(sp.P));
		vector3d_t rxv(ray.xfrom);
		float tx = -((sp.N * rxv) + d) / (sp.N * ray.xdir);
		point3d_t px = ray.xfrom + tx * ray.xdir;
		vector3d_t ryv(ray.yfrom);
		float ty = -((sp.N * ryv) + d) / (sp.N * ray.ydir);
		point3d_t py = ray.yfrom + ty * ray.ydir;
		dPdx = px - sp.P;
		dPdy = py - sp.P;
	}
	else
	{
		//dudx = dvdx = 0.;
		//dudy = dvdy = 0.;
		dPdx = dPdy = vector3d_t(0,0,0);
	}
}

void spDifferentials_t::reflectedRay(const diffRay_t &in, diffRay_t &out) const
{
	if(!in.hasDifferentials)
	{
		out.hasDifferentials = false;
		return;
	}
	// Compute ray differential _rd_ for specular reflection
	out.hasDifferentials = true;
	out.xfrom = sp.P + dPdx;
	out.yfrom = sp.P + dPdy;
	// Compute differential reflected directions
//	Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx +
//				  bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
//	Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy +
//				  bsdf->dgShading.dndv * bsdf->dgShading.dvdy;
	vector3d_t dwodx = in.dir - in.xdir, dwody = in.dir - in.ydir;
	float dDNdx = (dwodx * sp.N); // + (out.dir * dndx);
	float dDNdy = (dwody * sp.N); // + (out.dir * dndy);
	out.xdir = out.dir - dwodx + 2 * (/* (out.dir * sp.N) * dndx + */ dDNdx * sp.N);
	out.ydir = out.dir - dwody + 2 * (/* (out.dir * sp.N) * dndy + */ dDNdy * sp.N);
}

void spDifferentials_t::refractedRay(const diffRay_t &in, diffRay_t &out, float IOR) const
{
	if(!in.hasDifferentials)
	{
		out.hasDifferentials = false;
		return;
	}
	//RayDifferential rd(p, wi);
	out.hasDifferentials = true;
	out.xfrom = sp.P + dPdx;
	out.yfrom = sp.P + dPdy;
	//if (Dot(wo, n) < 0) eta = 1.f / eta;
	
	//Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx + bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
	//Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy + bsdf->dgShading.dndv * bsdf->dgShading.dvdy;
	
	vector3d_t dwodx = in.dir - in.xdir, dwody = in.dir - in.ydir;
	float dDNdx = (dwodx * sp.N); // + Dot(wo, dndx);
	float dDNdy = (dwody * sp.N); // + Dot(wo, dndy);
	
//	float mu = IOR * (in.dir * sp.N) - (out.dir * sp.N);
	float dmudx = (IOR - (IOR*IOR*(in.dir * sp.N))/(out.dir * sp.N)) * dDNdx;
	float dmudy = (IOR - (IOR*IOR*(in.dir * sp.N))/(out.dir * sp.N)) * dDNdy;
	
	out.xdir = out.dir + IOR * dwodx - (/* mu * dndx + */ dmudx * sp.N);
	out.ydir = out.dir + IOR * dwody - (/* mu * dndy + */ dmudy * sp.N);
}

float spDifferentials_t::projectedPixelArea()
{
	return (dPdx ^ dPdy).length();
}

void spDifferentials_t::dU_dV_from_dP_dPdU_dPdV(float &dU, float &dV, const point3d_t &dP, const vector3d_t &dPdU, const vector3d_t &dPdV) const
{
	float detXY = (dPdU.x*dPdV.y)-(dPdV.x*dPdU.y);
	float detXZ = (dPdU.x*dPdV.z)-(dPdV.x*dPdU.z);
	float detYZ = (dPdU.y*dPdV.z)-(dPdV.y*dPdU.z);
	
	if(fabsf(detXY) > 0.f && fabsf(detXY) > fabsf(detXZ) && fabsf(detXY) > fabsf(detYZ))
	{
		dU = ((dP.x*dPdV.y)-(dPdV.x*dP.y)) / detXY;
		dV = ((dPdU.x*dP.y)-(dP.x*dPdU.y)) / detXY;
	}

	else if(fabsf(detXZ) > 0.f && fabsf(detXZ) > fabsf(detXY) && fabsf(detXZ) > fabsf(detYZ))
	{
		dU = ((dP.x*dPdV.z)-(dPdV.x*dP.z)) / detXZ;
		dV = ((dPdU.x*dP.z)-(dP.x*dPdU.z)) / detXZ;
	}
	
	else if(fabsf(detYZ) > 0.f && fabsf(detYZ) > fabsf(detXY) && fabsf(detYZ) > fabsf(detXZ))
	{
		dU = ((dP.y*dPdV.z)-(dPdV.y*dP.z)) / detYZ;
		dV = ((dPdU.y*dP.z)-(dP.y*dPdU.z)) / detYZ;
	}
}

void spDifferentials_t::getUVdifferentials(float &dUdx, float &dVdx, float &dUdy, float &dVdy) const
{
	dU_dV_from_dP_dPdU_dPdV(dUdx, dVdx, dPdx, sp.dPdU_abs, sp.dPdV_abs);
	dU_dV_from_dP_dPdU_dPdV(dUdy, dVdy, dPdy, sp.dPdU_abs, sp.dPdV_abs);
}


surfacePoint_t blend_surface_points(surfacePoint_t const& sp_0, surfacePoint_t const& sp_1, float const alpha)
{
    surfacePoint_t sp_result(sp_0);

    sp_result.N = lerp(sp_0.N, sp_1.N, alpha);

    sp_result.NU = lerp(sp_0.NU, sp_1.NU, alpha);
    sp_result.NV = lerp(sp_0.NV, sp_1.NV, alpha);
    sp_result.dPdU = lerp(sp_0.dPdU, sp_1.dPdU, alpha);
    sp_result.dPdV = lerp(sp_0.dPdV, sp_1.dPdV, alpha);
    sp_result.dSdU = lerp(sp_0.dSdU, sp_1.dSdU, alpha);
    sp_result.dSdV = lerp(sp_0.dSdV, sp_1.dSdV, alpha);

    return sp_result;
}

__END_YAFRAY
