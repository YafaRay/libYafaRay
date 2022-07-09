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

#include "common/collection.h"
#include <map>
#include <vector>
#include <string>

namespace yafaray {

class Rgb;
class Rgba;
template <typename T, size_t N> class Vec;
typedef Vec<float, 3> Vec3f;
template <typename T, size_t N> class SquareMatrix;
typedef SquareMatrix<float, 4> Matrix4f;
class Logger;

/*! a class that can hold exactly one value of a range types.
*/
class Parameter
{
	public:
		enum class Type : unsigned char { None, Int, Bool, Float, String, Vector, Color, Matrix };
		//! return the type of the parameter_t
		Type type() const { return type_; }
		std::string print() const;
		std::string printType() const;

		Parameter() = default;
		explicit Parameter(const std::string &s);
		explicit Parameter(std::string &&s);
		explicit Parameter(int i);
		explicit Parameter(bool b);
		explicit Parameter(float f);
		explicit Parameter(double f);
		explicit Parameter(const Vec3f &p);
		explicit Parameter(const Rgba &c);
		explicit Parameter(const Matrix4f &m);

		// return parameter value in reference parameter, return true if
		// the parameter type matches, false otherwise.
		bool getVal(std::string &s) const;
		bool getVal(int &i) const;
		bool getVal(bool &b) const;
		bool getVal(float &f) const;
		bool getVal(double &f) const;
		bool getVal(Vec3f &p) const;
		bool getVal(Rgb &c) const;
		bool getVal(Rgba &c) const;
		bool getVal(Matrix4f &m) const;

		// operator= assigns new value, be aware that this may change the parameter type!
		Parameter &operator = (const std::string &s);
		Parameter &operator = (std::string &&s);
		Parameter &operator = (int i);
		Parameter &operator = (bool b);
		Parameter &operator = (float f);
		Parameter &operator = (const Vec3f &p);
		Parameter &operator = (const Rgb &c);
		Parameter &operator = (const Rgba &c);
		Parameter &operator = (const Matrix4f &m);

	private:
		union
		{
			int ival_;
			double fval_;
			bool bval_;
		};
		std::string sval_;
		std::vector<float> vval_;
		Type type_ = Type::None; //!< type of the stored value
};

class ParamMap : public Collection<std::string, Parameter>
{
	public:
		//! template function to get a value, available types are those of parameter_t::getVal()
		template <typename T>
		bool getParam(const std::string &name, T &val) const
		{
			auto i{find(name)};
			if(i) return i->getVal(val);
			else return false;
		}
		std::string print() const;
		void logContents(Logger &logger) const;
};

} //namespace yafaray
#endif // YAFARAY_PARAM_H
