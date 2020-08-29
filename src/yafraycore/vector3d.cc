/****************************************************************************
 *
 * 			vector3d.cc: Vector 3d and point manipulation implementation
 *      This is part of the yafray package
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

#include <core_api/vector3d.h>
#include <core_api/matrix4.h>

BEGIN_YAFRAY

std::ostream &operator << (std::ostream &out, const Vec3 &v)
{
	out << "(" << v.x_ << "," << v.y_ << "," << v.z_ << ")";
	return out;
}


std::ostream &operator << (std::ostream &out, const Point3 &p)
{
	out << "(" << p.x_ << "," << p.y_ << "," << p.z_ << ")";
	return out;
}


bool  operator == (const Vec3 &a, const Vec3 &b)
{
	if(a.x_ != b.x_) return false;
	if(a.y_ != b.y_) return false;
	if(a.z_ != b.z_) return false;
	return true;
}

bool  operator != (const Vec3 &a, const Vec3 &b)
{
	if(a.x_ != b.x_) return true;
	if(a.y_ != b.y_) return true;
	if(a.z_ != b.z_) return true;
	return false;
}

/* vector3d_t refract(const vector3d_t &n,const vector3d_t &v,float IOR)
{
	vector3d_t N=n,I,T;
	float eta=IOR;
	I=-v;
	if((v*n)<0)
	{
		N=-n;
		eta=IOR;
	}
	else
	{
		N=n;
		eta=1.0/IOR;
	}
	float IdotN = v*N;
	float k = 1 - eta*eta*(1 - IdotN*IdotN);
	T= (k < 0) ? vector3d_t(0,0,0) : (eta*I + (eta*IdotN - sqrt(k))*N);
	T.normalize();
	return T;
} */

/*! refract a ray given the IOR. All directions (n, wi and wo) point away from the intersection point.
	\return true when refraction was possible, false when total inner reflrection occurs (wo is not computed then)
	\param ior Index of refraction, or precisely the ratio of eta_t/eta_i, where eta_i is by definition the
				medium in which n points. E.g. "outside" is air, "inside" is water, the normal points outside,
				IOR = eta_air / eta_water = 1.33
*/
bool refract__(const Vec3 &n, const Vec3 &wi, Vec3 &wo, float ior)
{
	Vec3 N = n, i;
	float eta = ior;
	i = -wi;
	float cos_v_n = wi * n;
	if((cos_v_n) < 0)
	{
		N = -n;
		cos_v_n = -cos_v_n;
	}
	else
	{
		eta = 1.0 / ior;
	}
	float k = 1 - eta * eta * (1 - cos_v_n * cos_v_n);
	if(k <= 0.f) return false;

	wo = eta * i + (eta * cos_v_n - fSqrt__(k)) * N;
	wo.normalize();

	return true;
}

void fresnel__(const Vec3 &i, const Vec3 &n, float ior, float &kr, float &kt)
{
	float eta;
	Vec3 N;

	if((i * n) < 0)
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
	float c = i * N;
	float g = eta * eta + c * c - 1;
	if(g <= 0)
		g = 0;
	else
		g = fSqrt__(g);
	float aux = c * (g + c);

	kr = ((0.5 * (g - c) * (g - c)) / ((g + c) * (g + c))) *
		 (1 + ((aux - 1) * (aux - 1)) / ((aux + 1) * (aux + 1)));
	if(kr < 1.0)
		kt = 1 - kr;
	else
		kt = 0;
}


// 'Faster' Schlick fresnel approximation,
void fastFresnel__(const Vec3 &i, const Vec3 &n, float iorf,
				   float &kr, float &kt)
{
	float t = 1 - (i * n);
	//t = (t<0)?0:((t>1)?1:t);
	float t_2 = t * t;
	kr = iorf + (1 - iorf) * t_2 * t_2 * t;
	kt = 1 - kr;
}

// P.Shirley's concentric disk algorithm, maps square to disk
void shirleyDisk__(float r_1, float r_2, float &u, float &v)
{
	float phi = 0, r = 0, a = 2 * r_1 - 1, b = 2 * r_2 - 1;
	if(a > -b)
	{
		if(a > b)  	// Reg.1
		{
			r = a;
			phi = M_PI_4 * (b / a);
		}
		else  			// Reg.2
		{
			r = b;
			phi = M_PI_4 * (2 - a / b);
		}
	}
	else
	{
		if(a < b)  	// Reg.3
		{
			r = -a;
			phi = M_PI_4 * (4 + b / a);
		}
		else  			// Reg.4
		{
			r = -b;
			if(b != 0)
				phi = M_PI_4 * (6 - a / b);
			else
				phi = 0;
		}
	}
	u = r * fCos__(phi);
	v = r * fSin__(phi);
}



YAFRAYCORE_EXPORT int myseed__ = 123212;

Vec3 randomVectorCone__(const Vec3 &d,
						const Vec3 &u, const Vec3 &v,
						float cosang, float z_1, float z_2)
{
	float t_1 = M_2PI * z_1, t_2 = 1.0 - (1.0 - cosang) * z_2;
	return (u * fCos__(t_1) + v * fSin__(t_1)) * fSqrt__(1.0 - t_2 * t_2) + d * t_2;
}

Vec3 randomVectorCone__(const Vec3 &dir, float cangle, float r_1, float r_2)
{
	Vec3 u, v;
	createCs__(dir, u, v);
	return randomVectorCone__(dir, u, v, cangle, r_1, r_2);
}

Vec3 discreteVectorCone__(const Vec3 &dir, float cangle, int sample, int square)
{
	float r_1 = (float)(sample / square) / (float)square;
	float r_2 = (float)(sample % square) / (float)square;
	float tt = M_2PI * r_1;
	float ss = fAcos__(1.0 - (1.0 - cangle) * r_2);
	Vec3	vx(fCos__(ss), fSin__(ss) * fCos__(tt), fSin__(ss) * fSin__(tt));
	Vec3	i(1, 0, 0), c;
	Matrix4 m(1);
	if((std::fabs(dir.y_) > 0.0) || (std::fabs(dir.z_) > 0.0))
	{
		m[0][0] = dir.x_;
		m[1][0] = dir.y_;
		m[2][0] = dir.z_;
		c = i ^ dir;
		c.normalize();
		m[0][1] = c.x_;
		m[1][1] = c.y_;
		m[2][1] = c.z_;
		c = dir ^ c;
		c.normalize();
		m[0][2] = c.x_;
		m[1][2] = c.y_;
		m[2][2] = c.z_;
	}
	else if(dir.x_ < 0.0) m[0][0] = -1.0;
	return m * vx;
}

END_YAFRAY
