/****************************************************************************
 * 		iesUtils.h: utilities for parsing IES data files
 *      This is part of the yafaray package
 *      Copyright (C) 2009  Bert Buchholz and Rodrigo Placencia
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

#ifndef IESUTILS_H
#define IESUTILS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <utilities/interpolation.h>

__BEGIN_YAFRAY
//!TODO: Preblur data

class IESData_t
{
public:

	IESData_t(float blur, int reso);
	~IESData_t();
	bool parseIESFile(const std::string file);
	float getRadianceBlurred(float u, float v) const;
	float getMaxVAngle() const { return maxVAngle; };
	
private:

	bool getRadiance(float hAng, float vAng, float& rad) const;
	float blurRadiance(float rad) const;
	float* vertAngleMap; //<! vertical spherical angles
	float* horAngleMap; //<! horizontal sperical angles
	float** radMap; //<! spherical radiance map corresponding with entries to the angle maps
	int horAngles; //<! number of angles in the 2 directions
	int vertAngles;
	
	float maxRad;
	float resStep;
	int resBound;
	
	float maxVAngle;
	
	bool blurred;
};
IESData_t::IESData_t(float blur, int reso)
{
	blurred = !(blur < 0.5f || reso < 2);
	
	if(blurred)
	{
		resStep = blur/(float)reso;
		resBound = reso >> 1;
	}
}

IESData_t::~IESData_t()
{
	if(vertAngleMap) delete [] vertAngleMap;
	if(horAngleMap) delete [] horAngleMap;
	if(radMap)
	{
		for(int i = 0; i < horAngles; i++) delete [] radMap[i];
		delete [] radMap;
	}
}

float IESData_t::getRadianceBlurred(float hAng, float vAng) const
{

	float ret = 0.f;

	if (!blurred)
	{
		getRadiance(hAng, vAng, ret);
	}
	else
	{
		ret = blurRadiance(hAng, vAng);
	}

	return ret * maxRad;
}

//! hAng and vAng in degrees, rad is the buffer for the radiance at that angle
bool IESData_t::getRadiance(float hAng, float vAng, float& rad) const {
	
	int i = 0;

	rad = 0.f;
	float tmp = 0.f;

	while (i < horAngles && hAng >= horAngleMap[i])
	{	
		if (vAng >= vertAngleMap[0] && vAng <= vertAngleMap[vertAngles - 1])
		{
			int j = -1;
			
			while (j < vertAngles - 2 && vAng >= vertAngleMap[j+1]) j++;
			
			float vertDiff = vertAngleMap[j+1] - vertAngleMap[j];
			float offset_v = (vAng - vertAngleMap[j]) / vertDiff;
			
			tmp = CosineInterpolate(radMap[i][j], radMap[i][j + 1], offset_v);
		}
		rad += tmp;
		i++;
	}
	
	rad /= horAngles;
	
	return true;
}

float IESData_t::blurRadiance(float hAng, float vAng) const
{
	float ret = 0.f;
	for (int i = -resBound; i < resBound; ++i)
	{
		float tmp;
		
		if (getRadiance(hAng + (resStep * (float)i), vAng + (resStep * (float)i), tmp))
		{
			ret += tmp;
			++hits;
		}
	}
	return ret/(float)hits;
}

// IES description: http://lumen.iee.put.poznan.pl/kw/iesna.txt

bool IESData_t::parseIESFile(const std::string iesFile)
{
	using namespace std;
	
	Y_INFO << "IES Parser: Parsing IES file " << iesFile << std::endl;
	
	ifstream fin(iesFile.c_str(), std::ios::in);
	
	if (!fin)
	{
		Y_ERROR << "IES Parser: Could not open IES file: " << iesFile << std::endl;
		return false;
	}

	string line;
	string dummy;
	
	fin >> line;
	
	while (line.find("TILT=") == string::npos)
	{
		fin >> line;
	}
	
	if(line == "TILT=INCLUDE")
	{
		int pairs = 0;
		fin >> line;
		fin >> pairs;
		for(int i = 0; i < (pairs * 2); i++) fin >> line;
	}

	for (int i = 0; i < 2; i++)
		fin >> line;
	
	float candelaMult = 0.f;
	fin >> candelaMult;
	fin >> vertAngles;
	fin >> horAngles;

	for (int i = 0; i < 8; i++)
		fin >> line;

	vertAngleMap = new float[vertAngles];
	
	maxVAngle = 0.f;
	
	for (int i = 0; i < vertAngles; ++i) {
		fin >> vertAngleMap[i];
		if(maxVAngle < vertAngleMap[i]) maxVAngle = vertAngleMap[i];
	}
	
	maxVAngle = degToRad(maxVAngle);
	
	horAngleMap = new float[horAngles];
	
	for (int i = 0; i < horAngles; ++i) {
		fin >> horAngleMap[i];
	}
	
	maxRad = 0.f;
	float tmpRad = 0.f;
	
	radMap = new float*[horAngles];
	
	for (int i = 0; i < horAngles; ++i) {
		radMap[i] = new float[vertAngles];
		for (int j = 0; j < vertAngles; ++j) {
			fin >> radMap[i][j];
			if(tmpRad < radMap[i][j]) tmpRad = radMap[i][j];
		}
		maxRad += tmpRad;
	}
	
	maxRad /= horAngles;
	maxRad = 1.f / maxRad;
	
	fin.close();
	
	Y_INFO << "IES Parser: IES File parsed successfully\n";
	
	return true;
}

__END_YAFRAY

#endif //IESUTILS_H
