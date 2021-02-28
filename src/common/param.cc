/****************************************************************************
 *      This is part of the libYafaRay package
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

#include "common/param.h"
#include "geometry/vector.h"
#include "color/color.h"
#include "geometry/matrix4.h"
#include "common/logger.h"

BEGIN_YAFARAY

Parameter::Parameter(const std::string &s) : type_(String) { sval_ = s; }
Parameter::Parameter(int i) : type_(Int) { ival_ = i; }
Parameter::Parameter(bool b) : type_(Bool) { bval_ = b; }
Parameter::Parameter(float f) : type_(Float) { fval_ = f; }
Parameter::Parameter(double f) : type_(Float) { fval_ = f; }

Parameter::Parameter(const Vec3 &p) : type_(Vector)
{
	vval_.resize(3); vval_.at(0) = p.x_, vval_.at(1) = p.y_, vval_.at(2) = p.z_;
}

Parameter::Parameter(const Rgba &c) : type_(Color)
{
	vval_.resize(4); vval_.at(0) = c.r_, vval_.at(1) = c.g_, vval_.at(2) = c.b_, vval_.at(3) = c.a_;
}

Parameter::Parameter(const Matrix4 &m) : type_(Matrix)
{
	vval_.resize(16);
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j)
			vval_.at(i * 4 + j) = m[i][j];
}

bool Parameter::getVal(std::string &s) const {if(type_ == String) { s = sval_; return true;} return false;}
bool Parameter::getVal(int &i) const {if(type_ == Int)  { i = ival_; return true;} return false;}
bool Parameter::getVal(bool &b) const {if(type_ == Bool)  { b = bval_; return true;} return false;}
bool Parameter::getVal(float &f) const {if(type_ == Float) { f = (float)fval_; return true;} return false;}
bool Parameter::getVal(double &f) const {if(type_ == Float) { f = fval_; return true;} return false;}

bool Parameter::getVal(Vec3 &p) const {
	if(type_ == Vector)
	{
		p.x_ = vval_.at(0), p.y_ = vval_.at(1), p.z_ = vval_.at(2);
		return true;
	}
	return false;
}

bool Parameter::getVal(Rgb &c) const {
	if(type_ == Color)
	{
		c.r_ = vval_.at(0), c.g_ = vval_.at(1), c.b_ = vval_.at(2);
		return true;
	}
	return false;
}

bool Parameter::getVal(Rgba &c) const {
	if(type_ == Color)
	{
		c.r_ = vval_.at(0), c.g_ = vval_.at(1), c.b_ = vval_.at(2), c.a_ = vval_.at(3);
		return true;
	}
	return false;
}

bool Parameter::getVal(Matrix4 &m) const
{
	if(type_ == Matrix)
	{
		for(int i = 0; i < 4; ++i)
			for(int j = 0; j < 4; ++j)
				m[i][j] = vval_.at(i * 4 + j);
		return true;
	}
	return false;
}

Parameter &Parameter::operator=(const std::string &s)
{
	if(type_ != String) { type_ = String; vval_.clear(); }
	sval_ = s;
	return *this;
}

Parameter &Parameter::operator=(int i)
{
	if(type_ != Int) { type_ = Int; vval_.clear(); sval_.clear(); }
	ival_ = i;
	return *this;
}

Parameter &Parameter::operator=(bool b)
{
	if(type_ != Bool) { type_ = Bool; vval_.clear(); sval_.clear(); }
	bval_ = b;
	return *this;
}

Parameter &Parameter::operator=(float f)
{
	if(type_ != Float) { type_ = Float; vval_.clear(); sval_.clear(); }
	fval_ = f;
	return *this;
}

Parameter &Parameter::operator=(const Vec3 &p)
{
	if(type_ != Vector) { type_ = Vector; sval_.clear(); vval_.resize(3); }
	vval_.at(0) = p.x_, vval_.at(1) = p.y_, vval_.at(2) = p.z_;
	return *this;
}
Parameter &Parameter::operator=(const Rgba &c)
{
	if(type_ != Color) { type_ = Color; sval_.clear(); vval_.resize(4); }
	vval_.at(0) = c.r_, vval_.at(1) = c.g_, vval_.at(2) = c.b_, vval_.at(3) = c.a_;
	return *this;
}

Parameter &Parameter::operator=(const Matrix4 &m)
{
	if(type_ != Matrix) { type_ = Matrix; sval_.clear(); vval_.resize(16); }
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j)
			vval_.at(i * 4 + j) = m[i][j];
	return *this;
}

std::string Parameter::print() const
{
	if(type_ == Int)
	{
		int value;
		getVal(value);
		return std::to_string(value);
	}
	else if(type_ == Bool)
	{
		bool value;
		getVal(value);
		return value == true ? "true" : "false";
	}
	else if(type_ == Float)
	{
		double value;
		getVal(value);
		return std::to_string(value);
	}
	else if(type_ == String)
	{
		std::string value;
		getVal(value);
		return value;
	}
	else if(type_ == Vector)
	{
		Point3 value;
		getVal(value);
		return "(x:" + std::to_string(value.x_) + ", y:" + std::to_string(value.x_) + ", z:" + std::to_string(value.z_) + ")";
	}
	else if(type_ == Color)
	{
		Rgba value;
		getVal(value);
		return "(r:" + std::to_string(value.r_) + ", g:" + std::to_string(value.g_) + ", b:" + std::to_string(value.b_) + ", a:" + std::to_string(value.a_) + ")";
	}
	else if(type_ == Matrix)
	{
		Matrix4 value;
		getVal(value);
		std::string result = "(";
		for(int i = 0; i < 4; ++i)
			for(int j = 0; j < 4; ++j)
			{
				result += "m" + std::to_string(i) + "," + std::to_string(j) + ":" + std::to_string(value[i][j]);
				if(i != 3 && j != 3) result += ", ";
			}
		return result;
	}
	else return "";
}

std::string Parameter::printType() const
{
	switch(type_)
	{
		case Int: return "Int";
		case Bool: return "Bool";
		case Float: return "Float";
		case String: return "String";
		case Vector: return "Vector";
		case Color: return "Color";
		case Matrix: return "Matrix";
		default: return "None/Unknown";
	}
}

std::string ParamMap::print() const
{
	std::string result;
	for(const auto &it : dicc_)
	{
		result += "'" + it.first + "' (" + it.second.printType() + ") = '" + it.second.print() + "'\n";
	}
	return result;
}

void ParamMap::printDebug() const
{
	if(Y_LOG_HAS_DEBUG)
	{
		for(const auto &it : dicc_)
		{
			Y_DEBUG << "'" + it.first + "' (" + it.second.printType() + ") = '" + it.second.print() + "'\n";
		}
	}
}


Parameter &ParamMap::operator[](const std::string &key) { return dicc_[key]; }
void ParamMap::clear() { dicc_.clear(); }

std::map<std::string, Parameter>::const_iterator ParamMap::begin() const { return dicc_.begin(); }
std::map<std::string, Parameter>::const_iterator ParamMap::end() const { return dicc_.end(); }

END_YAFARAY
