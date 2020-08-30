/****************************************************************************
 *
 *      color.cc: Color type and operators implementation
 *      This is part of the libYafaRay package
 *      Copyright (C) 2002  Alejandro Conty Estévez
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
#include "common/color.h"
using namespace std;
#include<iostream>

BEGIN_YAFARAY

void operator >> (unsigned char *data, Rgb &c)
{
	c.r_ = ((float) data[0]) / ((float)255);
	c.g_ = ((float) data[1]) / ((float)255);
	c.b_ = ((float) data[2]) / ((float)255);
}

void operator << (unsigned char *data, const Rgb &c)
{
	//	data[0]=(char) (((c.R<(float)0) ? 0 : ((c.R>(float)1) ? 255 : (((float)255)*c.R) )) );
	//	data[1]=(char) (((c.G<(float)0) ? 0 : ((c.G>(float)1) ? 255 : (((float)255)*c.G) )) );
	//	data[2]=(char) (((c.B<(float)0) ? 0 : ((c.B>(float)1) ? 255 : (((float)255)*c.B) )) );
	data[0] = (c.r_ < 0.f) ? 0 : ((c.r_ >= 1.f) ? 255 : (unsigned char)(255.f * c.r_));
	data[1] = (c.g_ < 0.f) ? 0 : ((c.g_ >= 1.f) ? 255 : (unsigned char)(255.f * c.g_));
	data[2] = (c.b_ < 0.f) ? 0 : ((c.b_ >= 1.f) ? 255 : (unsigned char)(255.f * c.b_));
}

void operator >> (unsigned char *data, Rgba &c)
{
	c.r_ = ((float) data[0]) / ((float)255);
	c.g_ = ((float) data[1]) / ((float)255);
	c.b_ = ((float) data[2]) / ((float)255);
	c.a_ = ((float) data[3]) / ((float)255);
}


void operator << (unsigned char *data, const Rgba &c)
{
	//	data[0]=(char) (((c.R<(float)0) ? 0 : ((c.R>(float)1) ? 255 : (((float)255)*c.R) )) );
	//	data[1]=(char) (((c.G<(float)0) ? 0 : ((c.G>(float)1) ? 255 : (((float)255)*c.G) )) );
	//	data[2]=(char) (((c.B<(float)0) ? 0 : ((c.B>(float)1) ? 255 : (((float)255)*c.B) )) );
	//	data[3]=(char) (((c.A<(float)0) ? 0 : ((c.A>(float)1) ? 255 : (((float)255)*c.A) )) );
	data[0] = (c.r_ < 0.f) ? 0 : ((c.r_ >= 1.f) ? 255 : (unsigned char)(255.f * c.r_));
	data[1] = (c.g_ < 0.f) ? 0 : ((c.g_ >= 1.f) ? 255 : (unsigned char)(255.f * c.g_));
	data[2] = (c.b_ < 0.f) ? 0 : ((c.b_ >= 1.f) ? 255 : (unsigned char)(255.f * c.b_));
	data[3] = (c.a_ < 0.f) ? 0 : ((c.a_ >= 1.f) ? 255 : (unsigned char)(255.f * c.a_));
}

void operator >> (float *data, Rgb &c)
{
	c.r_ = data[0];
	c.g_ = data[1];
	c.b_ = data[2];
}


void operator << (float *data, const Rgb &c)
{
	data[0] = c.r_;
	data[1] = c.g_;
	data[2] = c.b_;
}

void operator >> (float *data, Rgba &c)
{
	c.r_ = data[0];
	c.g_ = data[1];
	c.b_ = data[2];
	c.a_ = data[3];
}


void operator << (float *data, const Rgba &c)
{
	data[0] = c.r_;
	data[1] = c.g_;
	data[2] = c.b_;
	data[3] = c.a_;
}

ostream &operator << (ostream &out, const Rgb c)
{
	out << "[" << c.r_ << " " << c.g_ << " " << c.b_ << "]";
	return out;
}

ostream &operator << (ostream &out, const Rgba c)
{
	out << "[" << c.r_ << ", " << c.g_ << ", " << c.b_ << ", " << c.a_ << "]";
	return out;
}

Rgb mix__(const Rgb &a, const Rgb &b, float point)
{
	if(point <= 0.0) return b;
	if(point >= 1.0) return a;

	return (a * point + (1 - point) * b);
}

Rgba mix__(const Rgba &a, const Rgba &b, float point)
{
	if(point <= 0.0) return b;
	if(point >= 1.0) return a;

	return (a * point + (1 - point) * b);
}

Rgbe::Rgbe(const Rgb &s)
{
	float v = s.getR();
	if(s.getG() > v) v = s.getG();
	if(s.getB() > v) v = s.getB();
	if(v < 1e-32f)
		rgbe_[0] = rgbe_[1] = rgbe_[2] = rgbe_[3] = 0;
	else
	{
		int e;
		v = frexp(v, &e) * 256.0 / v;
		rgbe_[0] = (unsigned char)(s.getR() * v);
		rgbe_[1] = (unsigned char)(s.getG() * v);
		rgbe_[2] = (unsigned char)(s.getB() * v);
		rgbe_[3] = (unsigned char)(e + 128);
	}
}

END_YAFARAY
