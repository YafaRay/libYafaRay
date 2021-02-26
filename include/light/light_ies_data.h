#pragma once
/****************************************************************************
 *      iesUtils.h: utilities for parsing IES data files
 *      This is part of the libYafaRay package
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

#ifndef YAFARAY_LIGHT_IES_DATA_H
#define YAFARAY_LIGHT_IES_DATA_H

#include "constants.h"
#include <fstream>
#include "common/logger.h"

BEGIN_YAFARAY
//!TODO: Preblur data

class IesData final
{
	public:
		IesData() = default;
		bool parseIesFile(const std::string &file);
		float getRadiance(float h_ang, float v_ang) const;
		float getMaxVAngle() const { return max_v_angle_; }

	private:
		enum Type : int { TypeC = 1, TypeB = 2, TypeA = 3, };
		std::unique_ptr<float[]> vert_angle_map_; //<! vertical spherical angles
		std::unique_ptr<float[]> hor_angle_map_; //<! horizontal sperical angles
		std::unique_ptr<std::unique_ptr<float[]>[]> rad_map_; //<! spherical radiance map corresponding with entries to the angle maps
		int hor_angles_; //<! number of angles in the 2 directions
		int vert_angles_;
		float max_rad_;
		float max_v_angle_;
		int type_;
};

//! hAng and vAng in degrees, returns the radiance at that angle
float IesData::getRadiance(float h_ang, float v_ang) const
{

	int x = 0, y = 0;
	float rad = 0.f;
	float ang_1 = 0.f, ang_2 = 0.f;

	if(type_ == TypeC)
	{
		ang_1 = h_ang;
		ang_2 = v_ang;
	}
	else
	{
		ang_1 = v_ang;
		ang_2 = h_ang;
		if(type_ == TypeB)
		{
			ang_1 += 90;
			if(ang_1 > 360.f) ang_1 -= 360.f;
		}
	}

	if(ang_1 > 180.f && hor_angle_map_[hor_angles_ - 1] <= 180.f) ang_1 = 360.f - ang_1;
	if(ang_1 > 90.f && hor_angle_map_[hor_angles_ - 1] <= 90.f) ang_1 -= 90.f;

	if(ang_2 > 90.f && vert_angle_map_[vert_angles_ - 1] <= 90.f) ang_2 -= 90.f;

	for(int i = 0; i < hor_angles_; i++)
	{
		if(hor_angle_map_[i] <= ang_1 && hor_angle_map_[i + 1] > ang_1)
		{
			x = i;
		}
	}

	for(int i = 0; i < vert_angles_; i++)
	{
		if(vert_angle_map_[i] <= ang_2 && vert_angle_map_[i + 1] > ang_2)
		{
			y = i;
			break;
		}
	}


	if(ang_1 == hor_angle_map_[x] && ang_2 == vert_angle_map_[y])
	{
		rad = rad_map_[x][y];
	}
	else
	{
		int x_1 = x, x_2 = x + 1;
		int y_1 = y, y_2 = y + 1;

		float d_x = (ang_1 - hor_angle_map_[x_1]) / (hor_angle_map_[x_2] - hor_angle_map_[x_1]);
		float d_y = (ang_2 - vert_angle_map_[y_1]) / (vert_angle_map_[y_2] - vert_angle_map_[y_1]);

		float rx_1 = ((1.f - d_x) * rad_map_[x_1][y_1]) + (d_x * rad_map_[x_2][y_1]);
		float rx_2 = ((1.f - d_x) * rad_map_[x_1][y_2]) + (d_x * rad_map_[x_2][y_2]);

		rad = ((1.f - d_y) * rx_1) + (d_y * rx_2);
	}


	return (rad * max_rad_);
}

// IES description: http://lumen.iee.put.poznan.pl/kw/iesna.txt

bool IesData::parseIesFile(const std::string &file)
{
	//FIXME: migrate to new file_t system
	using namespace std;

	Y_VERBOSE << "IES Parser: Parsing IES file " << file << YENDL;

	ifstream fin(file.c_str(), std::ios::in);

	if(!fin)
	{
		Y_ERROR << "IES Parser: Could not open IES file: " << file << YENDL;
		return false;
	}

	//get length of file:
	fin.seekg(0, fin.end);
	int length = fin.tellg();
	fin.seekg(0, fin.beg);

	if(length < 7)
	{
		Y_ERROR << "IES Parser: file is too small, only " << length << " bytes long." << YENDL;
		return false;
	}

	//check header is correct
	char ies_check_characters[7];
	fin.get(ies_check_characters, 7);
	Y_DEBUG << "std::string(ies_check_characters)=" << std::string(ies_check_characters) << YENDL;

	if(std::string(ies_check_characters) != "IESNA:")
	{
		Y_ERROR << "IES Parser: wrong file format, first characters are not \"IESNA:\"" << YENDL;
		return false;
	}

	//rewind file to beginning again
	fin.seekg(0, fin.beg);

	string line;
	string dummy;

	fin >> line;

	while(line.find("TILT=") == string::npos)
	{
		fin >> line;
	}

	if(line.find("TILT=") != string::npos)
	{
		if(line == "TILT=INCLUDE")
		{
			Y_VERBOSE << "IES Parser: Tilt data included in IES file." << YENDL << "Skiping..." << YENDL;

			int pairs = 0;

			fin >> line;
			fin >> pairs;

			for(int i = 0; i < (pairs * 2); i++) fin >> line;

			Y_VERBOSE << "IES Parser: Tilt data skipped." << YENDL;
		}
		else if(line == "TILT=NONE")
		{
			Y_VERBOSE << "IES Parser: No tilt data." << YENDL;
		}
		else if(line == "TILT=NONE")
		{
			Y_VERBOSE << "IES Parser: Tilt data in another file." << YENDL;
		}
	}
	else
	{
		fin.close();

		Y_WARNING << "IES Parser: Tilt not found IES invalid!" << YENDL;

		return false;
	}

	float candela_mult = 0.f;

	fin >> line;
	Y_VERBOSE << "IES Parser: Number of lamps: " << line << YENDL;
	fin >> line;
	Y_VERBOSE << "IES Parser: lumens per lamp: " << line << YENDL;
	fin >> candela_mult;
	candela_mult *= 0.001;
	Y_VERBOSE << "IES Parser: Candela multiplier (kcd): " << candela_mult << YENDL;
	fin >> vert_angles_;
	Y_VERBOSE << "IES Parser: Vertical Angles: " << vert_angles_ << YENDL;
	fin >> hor_angles_;
	Y_VERBOSE << "IES Parser: Horizontal Angles: " << hor_angles_ << YENDL;
	type_ = 0;
	fin >> type_;
	Y_VERBOSE << "IES Parser: Photometric Type: " << type_ << YENDL;
	fin >> line;
	Y_VERBOSE << "IES Parser: Units Type: " << line << YENDL;

	float w = 0.f, l = 0.f, h = 0.f;

	fin >> w;
	fin >> l;
	fin >> h;

	Y_VERBOSE << "IES Parser: Luminous opening dimensions:" << YENDL;
	Y_VERBOSE << "IES Parser: (Width, Length, Height) = (" << w << ", " << l << ", " << h << ")" << YENDL;
	Y_VERBOSE << "IES Parser: Lamp Geometry: ";

	//Check geometry type
	if(w == 0.f && l == 0.f && h == 0.f)
	{
		Y_VERBOSE << "Point Light" << YENDL;
	}
	else if(w >= 0.f && l >= 0.f && h >= 0.f)
	{
		Y_VERBOSE << "Rectangular Light" << YENDL;
	}
	else if(w < 0.f && l == 0.f && h == 0.f)
	{
		Y_VERBOSE << "Circular Light" << YENDL;
	}
	else if(w < 0.f && l == 0.f && h < 0.f)
	{
		Y_VERBOSE << "Shpere Light" << YENDL;
	}
	else if(w < 0.f && l == 0.f && h >= 0.f)
	{
		Y_VERBOSE << "Vertical Cylindric Light" << YENDL;
	}
	else if(w == 0.f && l >= 0.f && h < 0.f)
	{
		Y_VERBOSE << "Horizontal Cylindric Light (Along width)" << YENDL;
	}
	else if(w >= 0.f && l == 0.f && h < 0.f)
	{
		Y_VERBOSE << "Horizontal Cylindric Light (Along length)" << YENDL;
	}
	else if(w < 0.f && l >= 0.f && h >= 0.f)
	{
		Y_VERBOSE << "Elipse Light (Along width)" << YENDL;
	}
	else if(w >= 0.f && l < 0.f && h >= 0.f)
	{
		Y_VERBOSE << "Elipse Light (Along length)" << YENDL;
	}
	else if(w < 0.f && l >= 0.f && h < 0.f)
	{
		Y_VERBOSE << "Elipsoid Light (Along width)" << YENDL;
	}
	else if(w >= 0.f && l < 0.f && h < 0.f)
	{
		Y_VERBOSE << "Elipsoid Light (Along length)" << YENDL;
	}

	fin >> line;
	Y_VERBOSE << "IES Parser: Ballast Factor: " << line << YENDL;
	fin >> line;
	Y_VERBOSE << "IES Parser: Ballast-Lamp Photometric Factor: " << line << YENDL;
	fin >> line;
	Y_VERBOSE << "IES Parser: Input Watts: " << line << YENDL;

	vert_angle_map_ = std::unique_ptr<float[]>(new float[vert_angles_]);

	max_v_angle_ = 0.f;

	Y_VERBOSE << "IES Parser: Vertical Angle Map:" << YENDL;

	for(int i = 0; i < vert_angles_; ++i)
	{
		fin >> vert_angle_map_[i];
		if(max_v_angle_ < vert_angle_map_[i]) max_v_angle_ = vert_angle_map_[i];
		std::cout << vert_angle_map_[i] << ", ";
	}

	std::cout << YENDL;

	if(vert_angle_map_[0] > 0.f)
	{
		Y_VERBOSE << "IES Parser: Vertical Angle Map (transformed):" << YENDL;
		float minus = vert_angle_map_[0];
		max_v_angle_ -= minus;
		for(int i = 0; i < vert_angles_; ++i)
		{
			vert_angle_map_[i] -= minus;
			std::cout << vert_angle_map_[i] << ", ";
		}
		std::cout << std::endl;
	}

	Y_VERBOSE << "IES Parser: Max vertical angle (degrees): " << max_v_angle_ << YENDL;

	max_v_angle_ = math::degToRad(max_v_angle_);

	Y_VERBOSE << "IES Parser: Max vertical angle (radians): " << max_v_angle_ << YENDL;

	bool h_adjust = false;

	if(type_ == TypeC && hor_angles_ == 1)
	{
		hor_angles_++;
		h_adjust = true;
	}

	hor_angle_map_ = std::unique_ptr<float[]>(new float[hor_angles_]);

	Y_VERBOSE << "IES Parser: Horizontal Angle Map:" << YENDL;

	for(int i = 0; i < hor_angles_; ++i)
	{
		if(i == hor_angles_ - 1 && h_adjust) hor_angle_map_[i] = 180.f;
		else fin >> hor_angle_map_[i];
		std::cout << hor_angle_map_[i] << ", ";
	}
	std::cout << std::endl;

	max_rad_ = 0.f;

	rad_map_ = std::unique_ptr<std::unique_ptr<float[]>[]>(new std::unique_ptr<float[]>[hor_angles_]);
	for(int i = 0; i < hor_angles_; ++i)
	{
		rad_map_[i] = std::unique_ptr<float[]>(new float[vert_angles_]);
		for(int j = 0; j < vert_angles_; ++j)
		{
			if(i == hor_angles_ - 1 && h_adjust) rad_map_[i][j] = rad_map_[i - 1][j];
			else  fin >> rad_map_[i][j];
			if(max_rad_ < rad_map_[i][j]) max_rad_ = rad_map_[i][j];
		}
	}

	Y_VERBOSE << "IES Parser: maxRad = " << max_rad_ << YENDL;
	max_rad_ = 1.f / max_rad_;

	fin.close();

	Y_VERBOSE << "IES Parser: IES File parsed successfully" << YENDL;

	return true;
}


END_YAFARAY

#endif //YAFARAY_LIGHT_IES_DATA_H
