#pragma once
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

#ifndef YAFARAY_PARAM_H
#define YAFARAY_PARAM_H

#include "yafaray_common.h"
#include <map>
#include <vector>
#include <string>

BEGIN_YAFARAY

class Rgb;
class Rgba;
class Vec3;
class Matrix4;
class Logger;

/*! a class that can hold exactly one value of a range types.
*/
class Parameter
{
	public:
		enum Type : int { None = -1, Int = 1, Bool, Float, String, Vector, Color, Matrix };
		//! return the type of the parameter_t
		Type type() const { return type_; }
		std::string print() const;
		std::string printType() const;

		Parameter() = default;
		Parameter(const std::string &s);
		Parameter(int i);
		Parameter(bool b);
		Parameter(float f);
		Parameter(double f);
		Parameter(const Vec3 &p);
		Parameter(const Rgba &c);
		Parameter(const Matrix4 &m);

		// return parameter value in reference parameter, return true if
		// the parameter type matches, false otherwise.
		bool getVal(std::string &s) const;
		bool getVal(int &i) const;
		bool getVal(bool &b) const;
		bool getVal(float &f) const;
		bool getVal(double &f) const;
		bool getVal(Vec3 &p) const;
		bool getVal(Rgb &c) const;
		bool getVal(Rgba &c) const;
		bool getVal(Matrix4 &m) const;

		// operator= assigns new value, be aware that this may change the parameter type!
		Parameter &operator = (const std::string &s);
		Parameter &operator = (int i);
		Parameter &operator = (bool b);
		Parameter &operator = (float f);
		Parameter &operator = (const Vec3 &p);
		Parameter &operator = (const Rgba &c);
		Parameter &operator = (const Matrix4 &m);

	private:
		Type type_ = None; //!< type of the stored value
		union
		{
			int ival_;
			double fval_;
			bool bval_;
		};
		std::string sval_;
		std::vector<float> vval_;
};

class ParamMap
{
	public:
		ParamMap() = default;
		//! template function to get a value, available types are those of parameter_t::getVal()
		template <class T>
		bool getParam(const std::string &name, T &val) const
		{
			auto i = dicc_.find(name);
			if(i != dicc_.end()) return i->second.getVal(val);
			return false;
		}
		Parameter &operator [](const std::string &key);
		std::string print() const;
		void logContents(Logger &logger) const;

		void clear();
		std::map<std::string, Parameter>::const_iterator begin() const;
		std::map<std::string, Parameter>::const_iterator end() const;

	private:
		std::map<std::string, Parameter> dicc_;
};

END_YAFARAY
#endif // YAFARAY_PARAM_H
