/****************************************************************************
 *
 *      vector3d.cc: Vector 3d and point manipulation implementation
 *      This is part of the libYafaRay package
 *      Copyright (C) 2002 Alejandro Conty Estévez
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
 *
 */

#include "geometry/vector.h"
#include "geometry/matrix4.h"

BEGIN_YAFARAY

std::ostream &operator << (std::ostream &out, const Vec3 &v)
{
	out << "(" << v[0] << "," << v[1] << "," << v[2] << ")";
	return out;
}

std::ostream &operator << (std::ostream &out, const Point3 &p)
{
	out << "(" << p[0] << "," << p[1] << "," << p[2] << ")";
	return out;
}

bool  operator == (const Vec3 &a, const Vec3 &b)
{
	for(size_t i = 0; i < 3; ++i)
	{
		if(a[i] != b[i]) return false;
	}
	return true;
}

bool  operator != (const Vec3 &a, const Vec3 &b)
{
	for(size_t i = 0; i < 3; ++i)
	{
		if(a[i] != b[i]) return true;
	}
	return false;
}

/*! refract a ray given the IOR. All directions (n, wi and wo) point away from the intersection point.
	\return true when refraction was possible, false when total inner reflrection occurs (wo is not computed then)
	\param ior Index of refraction, or precisely the ratio of eta_t/eta_i, where eta_i is by definition the
				medium in which n points. E.g. "outside" is air, "inside" is water, the normal points outside,
				IOR = eta_air / eta_water = 1.33
*/
bool Vec3::refract(const Vec3 &n, const Vec3 &wi, Vec3 &wo, float ior)
{
	Vec3 N{n};
	float eta = ior;
	const Vec3 i{-wi};
	float cos_v_n = wi * n;
	if((cos_v_n) < 0)
	{
		N = -n;
		cos_v_n = -cos_v_n;
	}
	else
	{
		eta = 1.f / ior;
	}
	const float k = 1 - eta * eta * (1 - cos_v_n * cos_v_n);
	if(k <= 0.f) return false;

	wo = eta * i + (eta * cos_v_n - math::sqrt(k)) * N;
	wo.normalize();

	return true;
}

void Vec3::fresnel(const Vec3 &i, const Vec3 &n, float ior, float &kr, float &kt)
{
	float eta;
	Vec3 N;

	if((i * n) < 0.f)
	{
		//eta=1.0/IOR;
		eta = ior;
		N = -n;
	}
	else
	{
		eta = ior;
		N = n;
	}
	const float c = i * N;
	float g = eta * eta + c * c - 1.f;
	if(g <= 0) g = 0;
	else g = math::sqrt(g);
	const float aux = c * (g + c);
	kr = ((0.5f * (g - c) * (g - c)) / ((g + c) * (g + c))) *
		 (1.f + ((aux - 1.f) * (aux - 1.f)) / ((aux + 1.f) * (aux + 1.f)));
	if(kr < 1.f)
		kt = 1.f - kr;
	else
		kt = 0.f;
}

// 'Faster' Schlick fresnel approximation,
void Vec3::fastFresnel(const Vec3 &i, const Vec3 &n, float iorf, float &kr, float &kt)
{
	const float t = 1.f - (i * n);
	//t = (t<0)?0:((t>1)?1:t);
	const float t_2 = t * t;
	kr = iorf + (1.f - iorf) * t_2 * t_2 * t;
	kt = 1.f - kr;
}

// P.Shirley's concentric disk algorithm, maps square to disk
void Vec3::shirleyDisk(float r_1, float r_2, float &u, float &v)
{
	float phi = 0.f;
	float r = 0.f;
	const float a = 2.f * r_1 - 1.f;
	const float b = 2.f * r_2 - 1.f;
	if(a > -b)
	{
		if(a > b)  	// Reg.1
		{
			r = a;
			phi = math::div_pi_by_4 * (b / a);
		}
		else  			// Reg.2
		{
			r = b;
			phi = math::div_pi_by_4 * (2.f - a / b);
		}
	}
	else
	{
		if(a < b)  	// Reg.3
		{
			r = -a;
			phi = math::div_pi_by_4 * (4.f + b / a);
		}
		else  			// Reg.4
		{
			r = -b;
			if(b != 0)
				phi = math::div_pi_by_4 * (6.f - a / b);
			else
				phi = 0.f;
		}
	}
	u = r * math::cos(phi);
	v = r * math::sin(phi);
}

Vec3 Vec3::randomVectorCone(const Vec3 &d, const Vec3 &u, const Vec3 &v, float cosang, float z_1, float z_2)
{
	const float t_1 = math::mult_pi_by_2 * z_1, t_2 = 1.f - (1.f - cosang) * z_2;
	return (u * math::cos(t_1) + v * math::sin(t_1)) * math::sqrt(1.f - t_2 * t_2) + d * t_2;
}

Vec3 Vec3::randomVectorCone(const Vec3 &dir, float cangle, float r_1, float r_2)
{
	const auto [u, v]{createCoordsSystem(dir)};
	return Vec3::randomVectorCone(dir, u, v, cangle, r_1, r_2);
}

Vec3 Vec3::discreteVectorCone(const Vec3 &dir, float cangle, int sample, int square)
{
	const float r_1 = static_cast<float>(sample / square) / static_cast<float>(square);
	const float r_2 = static_cast<float>(sample % square) / static_cast<float>(square);
	const float tt = math::mult_pi_by_2 * r_1;
	const float ss = math::acos(1.f - (1.f - cangle) * r_2);
	const Vec3 vx(math::cos(ss), math::sin(ss) * math::cos(tt), math::sin(ss) * math::sin(tt));
	const Vec3 i(1.f, 0.f, 0.f);
	Matrix4 m(1.f);
	if((std::abs(dir.vec_[1]) > 0.f) || (std::abs(dir.vec_[2]) > 0.f))
	{
		m[0][0] = dir.vec_[0];
		m[1][0] = dir.vec_[1];
		m[2][0] = dir.vec_[2];
		Vec3 c = i ^ dir;
		c.normalize();
		m[0][1] = c.vec_[0];
		m[1][1] = c.vec_[1];
		m[2][1] = c.vec_[2];
		c = dir ^ c;
		c.normalize();
		m[0][2] = c.vec_[0];
		m[1][2] = c.vec_[1];
		m[2][2] = c.vec_[2];
	}
	else if(dir.vec_[0] < 0.f) m[0][0] = -1.f;
	return m * vx;
}

END_YAFARAY
