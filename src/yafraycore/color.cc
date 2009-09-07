/****************************************************************************
 *
 * 			color.cc: Color type and operators implementation 
 *      This is part of the yafray package
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
#include <core_api/color.h>
using namespace std;
#include<iostream>

__BEGIN_YAFRAY

void operator >> (unsigned char *data,color_t &c)
{
	c.R= ((CFLOAT) data[0])/((CFLOAT)255);
	c.G= ((CFLOAT) data[1])/((CFLOAT)255);
	c.B= ((CFLOAT) data[2])/((CFLOAT)255);
}

void operator << (unsigned char *data,const color_t &c)
{
//	data[0]=(char) (((c.R<(CFLOAT)0) ? 0 : ((c.R>(CFLOAT)1) ? 255 : (((CFLOAT)255)*c.R) )) );
//	data[1]=(char) (((c.G<(CFLOAT)0) ? 0 : ((c.G>(CFLOAT)1) ? 255 : (((CFLOAT)255)*c.G) )) );
//	data[2]=(char) (((c.B<(CFLOAT)0) ? 0 : ((c.B>(CFLOAT)1) ? 255 : (((CFLOAT)255)*c.B) )) );
	data[0]= (c.R<0.f) ? 0 : ((c.R>=1.f) ? 255 : (unsigned char)(255.f*c.R) );
	data[1]= (c.G<0.f) ? 0 : ((c.G>=1.f) ? 255 : (unsigned char)(255.f*c.G) );
	data[2]= (c.B<0.f) ? 0 : ((c.B>=1.f) ? 255 : (unsigned char)(255.f*c.B) ); 
}

void operator >> (unsigned char *data,colorA_t &c)
{
	c.R = ((CFLOAT) data[0])/((CFLOAT)255);
	c.G = ((CFLOAT) data[1])/((CFLOAT)255);
	c.B = ((CFLOAT) data[2])/((CFLOAT)255);
	c.A = ((CFLOAT) data[3])/((CFLOAT)255);
}


void operator << (unsigned char *data,const colorA_t &c)
{
//	data[0]=(char) (((c.R<(CFLOAT)0) ? 0 : ((c.R>(CFLOAT)1) ? 255 : (((CFLOAT)255)*c.R) )) );
//	data[1]=(char) (((c.G<(CFLOAT)0) ? 0 : ((c.G>(CFLOAT)1) ? 255 : (((CFLOAT)255)*c.G) )) );
//	data[2]=(char) (((c.B<(CFLOAT)0) ? 0 : ((c.B>(CFLOAT)1) ? 255 : (((CFLOAT)255)*c.B) )) );
//	data[3]=(char) (((c.A<(CFLOAT)0) ? 0 : ((c.A>(CFLOAT)1) ? 255 : (((CFLOAT)255)*c.A) )) );
	data[0]= (c.R<0.f) ? 0 : ((c.R>=1.f) ? 255 : (unsigned char)(255.f*c.R) );
	data[1]= (c.G<0.f) ? 0 : ((c.G>=1.f) ? 255 : (unsigned char)(255.f*c.G) );
	data[2]= (c.B<0.f) ? 0 : ((c.B>=1.f) ? 255 : (unsigned char)(255.f*c.B) );
	data[3]= (c.A<0.f) ? 0 : ((c.A>=1.f) ? 255 : (unsigned char)(255.f*c.A) ); 
}

void operator >> (float *data,color_t &c)
{
	c.R = data[0];
	c.G = data[1];
	c.B = data[2];
}


void operator << (float *data,const color_t &c)
{
	data[0] = c.R;
	data[1] = c.G;
	data[2] = c.B;
}

void operator >> (float *data,colorA_t &c)
{
	c.R = data[0];
	c.G = data[1];
	c.B = data[2];
	c.A = data[3];
}


void operator << (float *data,const colorA_t &c)
{
	data[0] = c.R;
	data[1] = c.G;
	data[2] = c.B;
	data[3] = c.A;
}

ostream & operator << (ostream & out,const color_t c)
{
	out<<"["<<c.R<<" "<<c.G<<" "<<c.B<<"]";
	return out;
}

ostream & operator << (ostream & out,const colorA_t c)
{
	out << "[" << c.R << ", " << c.G << ", " << c.B << ", " << c.A << "]";
	return out;
}

color_t mix(const color_t &a,const color_t &b,CFLOAT point)
{
	if(point<0.0) return b;
	if(point>1.0) return a;

	return ( a*point + (1-point)*b );
}

colorA_t mix(const colorA_t &a,const colorA_t &b,CFLOAT point)
{
	if(point<0.0) return b;
	if(point>1.0) return a;

	return ( a*point + (1-point)*b );
}

color_t convergenceAccell(const color_t &cn_1,const color_t &cn0,const color_t &cn1)
{
	CFLOAT r=(cn1.R-2.0*cn0.R+cn_1.R);
	if(r!=0.0)
		r=cn1.R - ((cn1.R-cn0.R)*(cn1.R-cn0.R))/r;
	else
		r=cn1.R;
	CFLOAT g=(cn1.G-2.0*cn0.G+cn_1.G);
	if(g!=0.0)
		g=cn1.G - ((cn1.G-cn0.G)*(cn1.G-cn0.G))/g;
	else
		g=cn1.G;
	CFLOAT b=(cn1.B-2.0*cn0.B+cn_1.B);
	if(b!=0.0)
		b=cn1.B - ((cn1.B-cn0.B)*(cn1.B-cn0.B))/b;
	else
		b=cn1.B;

	return color_t(r,g,b);
}

rgbe_t::rgbe_t(const color_t &s)
{
	CFLOAT v = s.getR();
	if (s.getG() > v) v = s.getG();
	if (s.getB() > v) v = s.getB();
	if (v<1e-32f)
		rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
	else {
		int e;
		v = frexp(v, &e) * 256.0/v;
		rgbe[0] = (unsigned char) (s.getR() * v);
		rgbe[1] = (unsigned char) (s.getG() * v);
		rgbe[2] = (unsigned char) (s.getB() * v);
		rgbe[3] = (unsigned char) (e + 128);
	}
}

__END_YAFRAY
