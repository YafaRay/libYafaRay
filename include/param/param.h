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

#ifndef LIBYAFARAY_PARAM_H
#define LIBYAFARAY_PARAM_H

#include "common/collection.h"
#include "common/result_flags.h"
#include "param/param_meta.h"
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

		bool operator==(const Parameter &parameter) const;
		bool operator!=(const Parameter &parameter) const;

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
		bool operator==(const ParamMap &param_map) const;
		bool operator!=(const ParamMap &param_map) const;
		//! template function to get a value, available types are those of parameter_t::getVal()
		template <typename T>
		ResultFlags getParam(const std::string &name, T &val) const
		{
			auto i{find(name)};
			if(i) return i->getVal(val) ? YAFARAY_RESULT_OK : YAFARAY_RESULT_ERROR_WRONG_PARAM_TYPE;
			else return YAFARAY_RESULT_WARNING_PARAM_NOT_SET;
		}
		template <typename T>
		ResultFlags getParam(const ParamMeta &param_meta, T &val) const
		{
			if constexpr (std::is_same_v<T, Rgb> || std::is_same_v<T, Rgba>)
			{
				Rgba col;
				const auto result{getParam(param_meta.name(), col)};
				col.colorSpaceFromLinearRgb(input_color_space_, input_gamma_);
				val = static_cast<T>(col);
				return result;
			}
			else return getParam(param_meta.name(), val);
		}
		template <typename T>
		ResultFlags getEnumParam(const std::string &name, T &val) const
		{
			std::string val_str;
			const ResultFlags result{getParam(name, val_str)};
			val.initFromString(val_str);
			return result;
		}
		template <typename T>
		ResultFlags getEnumParam(const ParamMeta &param_meta, T &val) const
		{
			return getEnumParam(param_meta.name(), val);
		}
		template <typename T>
		void setParam(const std::string param_name, const T &val)
		{
			if constexpr (std::is_same_v<T, Rgb> || std::is_same_v<T, Rgba>)
			{
				T col{val};
				col.linearRgbFromColorSpace(input_color_space_, input_gamma_);
				items_[param_name] = col;
			}
			else items_[param_name] = val;
		}
		template <typename T>
		void setParam(const ParamMeta &param_meta, const T &val)
		{
			setParam(param_meta.name(), val);
		}
		std::string print() const;
		std::string logContents() const;
		void setInputColorSpace(const std::string &color_space_string, float gamma_val);

	private:
		float input_gamma_{1.f};
		ColorSpace input_color_space_{ColorSpace::RawManualGamma};
};

} //namespace yafaray
#endif // LIBYAFARAY_PARAM_H
