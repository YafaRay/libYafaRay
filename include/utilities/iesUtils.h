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

struct IesData {
	float* vertAngleMap; //<! vertical spherical angles
	float* horAngleMap; //<! horizontal sperical angles
	float** radMap; //<! spherical radiance map corresponding with entries to the angle maps
	int horAngles; //<! number of angles in the 2 directions
	int vertAngles;
};

// IES description: http://lumen.iee.put.poznan.pl/kw/iesna.txt

bool parseIesFile(const std::string iesFile, IesData& iesData)
{
	using namespace std;
	
	Y_INFO << "Parsing IES file " << iesFile << std::endl;
	
	ifstream fin(iesFile.c_str(), std::ios::in);
	
	if (!fin)
	{
		Y_ERROR << "Could not open IES file: " << iesFile << std::endl;
		return false;
	}

	string line;
	string dummy;
	
	fin >> line;
	
	Y_INFO << "find tilt" << std::endl;
	
	while (line.find("TILT=NONE") == string::npos)
	{
		fin >> line;
	}

	for (int i = 0; i < 3; ++i)
		fin >> line;
	fin >> iesData.vertAngles;
	fin >> iesData.horAngles;

	if (iesData.horAngles != 1) {
		Y_ERROR << "Only circular IES lights supported." << std::endl;
		return false;
	}

	for (int i = 0; i < 8; ++i)
		fin >> line;

	iesData.vertAngleMap = new float[iesData.vertAngles];
	for (int i = 0; i < iesData.vertAngles; ++i) {
		fin >> iesData.vertAngleMap[i];
	}

	iesData.horAngleMap = new float[iesData.horAngles];
	for (int i = 0; i < iesData.horAngles; ++i) {
		fin >> iesData.horAngleMap[i];
	}

	iesData.radMap = new float*[iesData.horAngles];
	for (int i = 0; i < iesData.horAngles; ++i) {
		iesData.radMap[i] = new float[iesData.vertAngles];
		for (int j = 0; j < iesData.vertAngles; ++j) {
			fin >> iesData.radMap[i][j];
		}
	}

	fin.close();

	return true;
}

#endif //IESUTILS_H
