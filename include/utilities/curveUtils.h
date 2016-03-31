/****************************************************************************
 * 		curveUtils.cc: Curve interpolation utils
 *      This is part of the yafaray package
 *      Copyright (C) 2009 Rodrigo Placencia
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

#ifndef CURVEUTILS_H_
#define CURVEUTILS_H_

__BEGIN_YAFRAY

//////////////////////////////////////////////////////////////////////////////
// Curve Abstract declaration
//////////////////////////////////////////////////////////////////////////////

class Curve
{
public:
	virtual float getSample(float x) const = 0;
	float operator()(float x) const {return getSample(x);};
};

//////////////////////////////////////////////////////////////////////////////
// irregularCurve declaration & definition
//////////////////////////////////////////////////////////////////////////////

class  IrregularCurve: public Curve
{
private:
	float* c1;
	float* c2;
	int size;
	int index;

public:
	IrregularCurve(const float *datay, const float *datax, int n);
	IrregularCurve(const float *datay, int n);
	virtual ~IrregularCurve();
	float getSample(float wl) const;
	void addSample(float data);
};
IrregularCurve::IrregularCurve(const float *datay, const float *datax, int n):c1(nullptr), c2(nullptr), size(n), index(0)
{
	c1 = new float[n];
	c2 = new float[n];
	for(int i=0; i<n; i++)
	{
		c1[i] = datax[i];
		c2[i] = datay[i];
	}
}

IrregularCurve::IrregularCurve(const float *datay, int n):c1(nullptr), c2(nullptr), size(n), index(0)
{
	c1 = new float[n];
	c2 = new float[n];
	for(int i=0; i<n; i++) c2[i] = datay[i];
}

IrregularCurve::~IrregularCurve()
{
	if(c1) delete [] c1; c1=nullptr;
	if(c2) delete [] c2; c2=nullptr;
}

float IrregularCurve::getSample(float x) const
{
	if(x < c1[0] || x > c1[size - 1]) return 0.0;
	int zero = 0;

	for(int i = 0;i < size; i++)
	{
		if(c1[i] == x) return c2[i];
		else if(c1[i] <= x && c1[i+1] > x)
		{
			zero = i;
			break;
		}
	}

	float y = x - c1[zero];
	y *= (c2[zero+1] - c2[zero]) / (c1[zero+1] - c1[zero]);
	y += c2[zero];
	return y;
}

void IrregularCurve::addSample(float data)
{
	if(index < size) c1[index++] = data;
}

//////////////////////////////////////////////////////////////////////////////
// regularCurve declaration & definition
//////////////////////////////////////////////////////////////////////////////

class RegularCurve: public Curve
{
private:
	float *c;
	float m;
	float M;
	float step;
	int size;
	int index;

public:
	RegularCurve(const float *data, float BeginR, float EndR, int n);
	RegularCurve(float BeginR, float EndR, int n);
	virtual ~RegularCurve();
	float getSample(float x) const;
	void addSample(float data);
};

RegularCurve::RegularCurve(const float *data, float BeginR, float EndR, int n):
	c(nullptr), m(BeginR), M(EndR), step(0.0), size(n), index(0)
{
	c = new float[n];
	for(int i=0; i<n; i++) c[i] = data[i];
	step = n / (M - m);
}

RegularCurve::RegularCurve(float BeginR, float EndR, int n) : c(nullptr), m(BeginR), M(EndR), step(0.0), size(n), index(0)
{
	c = new float[n];
	step = n / (M - m);
}

RegularCurve::~RegularCurve()
{
	if(c) delete [] c; c=nullptr;
}

float RegularCurve::getSample(float x) const
{
	if(x < m || x > M) return 0.0;
	float med, x0, x1, y;
	int y0, y1;

	med = (x-m) * step;
	y0 = static_cast<int>(floor(med));
	y1 = static_cast<int>(ceil(med));

	if(y0 == y1) return c[y0];

	x0 = (y0/step) + m;
	x1 = (y1/step) + m;

	y = x - x0;
	y *= (c[y1] - c[y0]) / (x1 - x0);
	y += c[y0];

	return y;
}

void RegularCurve::addSample(float data)
{
	if(index < size) c[index++] = data;
}

__END_YAFRAY
#endif
