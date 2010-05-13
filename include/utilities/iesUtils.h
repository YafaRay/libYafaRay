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
#include <utilities/curveUtils.h>

__BEGIN_YAFRAY
//!TODO: Preblur data

#define TYPE_C 1
#define TYPE_B 2
#define TYPE_A 3

class IESData_t
{
public:

	IESData_t();
	~IESData_t();
	bool parseIESFile(const std::string file);
	float getRadiance(float hAng, float vAng) const;
	float getMaxVAngle() const { return maxVAngle; }
	
private:

	
	float* vertAngleMap; //<! vertical spherical angles
	float* horAngleMap; //<! horizontal sperical angles
	float** radMap; //<! spherical radiance map corresponding with entries to the angle maps
	int horAngles; //<! number of angles in the 2 directions
	int vertAngles;
	
	float maxRad;
	float maxVAngle;
	
	int type;
};

IESData_t::IESData_t()
{
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

//! hAng and vAng in degrees, returns the radiance at that angle
float IESData_t::getRadiance(float h, float v) const {
	
	int x = 0, y = 0;
	float rad = 0.f;
	float hAng = 0.f, vAng = 0.f;
	
	if(type == TYPE_C)
	{
		hAng = h;
		vAng = v;
	}
	else
	{
		hAng = v;
		vAng = h;
		if(type == TYPE_B)
		{
			hAng += 90;
			if(hAng > 360.f) hAng -= 360.f;
		}
	}

	if(hAng > 180.f && horAngleMap[horAngles-1] <= 180.f) hAng = 360.f - hAng;
	if(hAng > 90.f && horAngleMap[horAngles-1] <= 90.f) hAng -= 90.f;
	
	if(vAng > 90.f && vertAngleMap[vertAngles-1] <= 90.f) vAng -= 90.f;
	
	for(int i = 0;i < horAngles; i++)
	{
		if(horAngleMap[i] <= hAng && horAngleMap[i+1] > hAng)
		{
			x = i;
		}
	}

	for(int i = 0;i < vertAngles; i++)
	{
		if(vertAngleMap[i] <= vAng && vertAngleMap[i+1] > vAng)
		{
			y = i;
			break;
		}
	}
	
	
	if(hAng == horAngleMap[x] && vAng == vertAngleMap[y])
	{
		rad = radMap[x][y];
	}
	else
	{
		int x1 = x, x2 = x+1;
		int y1 = y, y2 = y+1;
		
		float dX = (hAng - horAngleMap[x1]) / (horAngleMap[x2] - horAngleMap[x1]);
		float dY = (vAng - vertAngleMap[y1]) / (vertAngleMap[y2] - vertAngleMap[y1]);
		
		float rx1 = ((1.f - dX) * radMap[x1][y1]) + (dX * radMap[x2][y1]);
		float rx2 = ((1.f - dX) * radMap[x1][y2]) + (dX * radMap[x2][y2]);
		
		rad = ((1.f - dY) * rx1) + (dY * rx2);
	}

	
	return (rad * maxRad);
}

// IES description: http://lumen.iee.put.poznan.pl/kw/iesna.txt

bool IESData_t::parseIESFile(const std::string iesFile)
{
	using namespace std;
	
	Y_INFO << "IES Parser: Parsing IES file " << iesFile << yendl;
	
	ifstream fin(iesFile.c_str(), std::ios::in);
	
	if (!fin)
	{
		Y_ERROR << "IES Parser: Could not open IES file: " << iesFile << yendl;
		return false;
	}

	string line;
	string dummy;
	
	fin >> line;
	
	while (line.find("TILT=") == string::npos)
	{
		fin >> line;
	}
	
	if(line.find("TILT=") != string::npos)
	{
		if(line == "TILT=INCLUDE")
		{
			Y_INFO << "IES Parser: Tilt data included in IES file.\nSkiping..." << yendl;
			
			int pairs = 0;
			
			fin >> line;
			fin >> pairs;
			
			for(int i = 0; i < (pairs * 2); i++) fin >> line;
			
			Y_INFO << "IES Parser: Tilt data skipped." << yendl;
		}
		else if(line == "TILT=NONE")
		{
			Y_INFO << "IES Parser: No tilt data." << yendl;
		}
		else if(line == "TILT=NONE")
		{
			Y_INFO << "IES Parser: Tilt data in another file." << yendl;
		}
	}
	else
	{
		fin.close();
	
		Y_INFO << "IES Parser: Tilt not found IES invalid!" << yendl;
		
		return false;
	}
	
	float candelaMult = 0.f;
	
	fin >> line;
	Y_INFO << "IES Parser: Number of lamps: " << line << yendl;
	fin >> line;
	Y_INFO << "IES Parser: lumens per lamp: " << line << yendl;
	fin >> candelaMult;
	candelaMult *= 0.001;
	Y_INFO << "IES Parser: Candela multiplier (kcd): " << candelaMult << yendl;
	fin >> vertAngles;
	Y_INFO << "IES Parser: Vertical Angles: " << vertAngles << yendl;
	fin >> horAngles;
	Y_INFO << "IES Parser: Horizontal Angles: " << horAngles << yendl;
	type = 0;
	fin >> type;
	Y_INFO << "IES Parser: Photometric Type: " << type << yendl;
	fin >> line;
	Y_INFO << "IES Parser: Units Type: " << line << yendl;
	
	float w = 0.f, l = 0.f, h = 0.f;
	
	fin >> w;
	fin >> l;
	fin >> h;

	Y_INFO << "IES Parser: Luminous opening dimensions:" << yendl;
	Y_INFO << "IES Parser: (Width, Length, Height) = (" << w << ", " << l << ", " << h << ")" << yendl;
	Y_INFO << "IES Parser: Lamp Geometry: ";
	
	//Check geometry type
	if(w == 0.f && l == 0.f && h == 0.f)
	{
		Y_INFO << "Point Light" << yendl;
	}
	else if(w >= 0.f && l >= 0.f && h >= 0.f)
	{
		Y_INFO << "Rectangular Light" << yendl;
	}
	else if(w < 0.f && l == 0.f && h == 0.f)
	{
		Y_INFO << "Circular Light" << yendl;
	}
	else if(w < 0.f && l == 0.f && h < 0.f)
	{
		Y_INFO << "Shpere Light" << yendl;
	}
	else if(w < 0.f && l == 0.f && h >= 0.f)
	{
		Y_INFO << "Vertical Cylindric Light" << yendl;
	}
	else if(w == 0.f && l >= 0.f && h < 0.f)
	{
		Y_INFO << "Horizontal Cylindric Light (Along width)" << yendl;
	}
	else if(w >= 0.f && l == 0.f && h < 0.f)
	{
		Y_INFO << "Horizontal Cylindric Light (Along length)" << yendl;
	}
	else if(w < 0.f && l >= 0.f && h >= 0.f)
	{
		Y_INFO << "Elipse Light (Along width)" << yendl;
	}
	else if(w >= 0.f && l < 0.f && h >= 0.f)
	{
		Y_INFO << "Elipse Light (Along length)" << yendl;
	}
	else if(w < 0.f && l >= 0.f && h < 0.f)
	{
		Y_INFO << "Elipsoid Light (Along width)" << yendl;
	}
	else if(w >= 0.f && l < 0.f && h < 0.f)
	{
		Y_INFO << "Elipsoid Light (Along length)" << yendl;
	}
	
	fin >> line;
	Y_INFO << "IES Parser: Ballast Factor: " << line << yendl;
	fin >> line;
	Y_INFO << "IES Parser: Ballast-Lamp Photometric Factor: " << line << yendl;
	fin >> line;
	Y_INFO << "IES Parser: Input Watts: " << line << yendl;

	vertAngleMap = new float[vertAngles];
	
	maxVAngle = 0.f;

	Y_INFO << "IES Parser: Vertical Angle Map:" << yendl;

	for (int i = 0; i < vertAngles; ++i)
	{
		fin >> vertAngleMap[i];
		if(maxVAngle < vertAngleMap[i]) maxVAngle = vertAngleMap[i];
		std::cout << vertAngleMap[i] << ", ";
	}
	
	std::cout << yendl;

	if(vertAngleMap[0] > 0.f)
	{
		Y_INFO << "IES Parser: Vertical Angle Map (transformed):" << yendl;
		float minus = vertAngleMap[0];
		maxVAngle -= minus;
		for (int i = 0; i < vertAngles; ++i)
		{
			vertAngleMap[i] -= minus;
			std::cout << vertAngleMap[i] << ", ";
		}
		std::cout << std::endl;
	}
	
	Y_INFO << "IES Parser: Max vertical angle (degrees): " << maxVAngle << yendl;
	
	maxVAngle = degToRad(maxVAngle);

	Y_INFO << "IES Parser: Max vertical angle (radians): " << maxVAngle << yendl;
	
	bool hAdjust = false;
	
	if(type == TYPE_C && horAngles == 1)
	{
		horAngles++;
		hAdjust = true;
	}
	
	horAngleMap = new float[horAngles];
	
	Y_INFO << "IES Parser: Horizontal Angle Map:" << yendl;
	
	for (int i = 0; i < horAngles; ++i)
	{
		if(i == horAngles - 1 && hAdjust) horAngleMap[i] = 180.f;
		else fin >> horAngleMap[i];
		std::cout << horAngleMap[i] << ", ";
	}
	std::cout << std::endl;

	maxRad = 0.f;
	
	radMap = new float*[horAngles];
	for (int i = 0; i < horAngles; ++i)
	{
		radMap[i] = new float[vertAngles];
		for (int j = 0; j < vertAngles; ++j)
		{
			if(i == horAngles - 1 && hAdjust) radMap[i][j] = radMap[i-1][j];
			else  fin >> radMap[i][j];
			if(maxRad < radMap[i][j]) maxRad = radMap[i][j];
		}
	}
	
	Y_INFO << "IES Parser: maxRad = " << maxRad << yendl;
	maxRad = 1.f / maxRad;
	
	fin.close();
	
	Y_INFO << "IES Parser: IES File parsed successfully" << yendl;
	
	return true;
}

__END_YAFRAY

#endif //IESUTILS_H
