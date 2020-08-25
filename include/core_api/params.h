#pragma once

#ifndef YAFARAY_PARAMS_H
#define YAFARAY_PARAMS_H

#include <yafray_constants.h>
#include <map>
#include <vector>
#include <string>

__BEGIN_YAFRAY

class color_t;
class colorA_t;
class point3d_t;
class matrix4x4_t;

/*! a class that can hold exactly one value of a range types.
*/
class YAFRAYCORE_EXPORT parameter_t
{
	public:
		enum Type : int { None = -1, Int = 1, Bool, Float, String, Point, Color, Matrix };
		//! return the type of the parameter_t
		Type type() const { return type_; }

		parameter_t() = default;
		parameter_t(const std::string &s);
		parameter_t(int i);
		parameter_t(bool b);
		parameter_t(float f);
		parameter_t(double f);
		parameter_t(const point3d_t &p);
		parameter_t(const colorA_t &c);
		parameter_t(const matrix4x4_t &m);

		// return parameter value in reference parameter, return true if
		// the parameter type matches, false otherwise.
		bool getVal(std::string &s) const;
		bool getVal(int &i) const;
		bool getVal(bool &b) const;
		bool getVal(float &f) const;
		bool getVal(double &f) const;
		bool getVal(point3d_t &p) const;
		bool getVal(color_t &c) const;
		bool getVal(colorA_t &c) const;
		bool getVal(matrix4x4_t &m) const;

		// operator= assigns new value, be aware that this may change the parameter type!
		parameter_t &operator = (const std::string &s);
		parameter_t &operator = (int i);
		parameter_t &operator = (bool b);
		parameter_t &operator = (float f);
		parameter_t &operator = (const point3d_t &p);
		parameter_t &operator = (const colorA_t &c);
		parameter_t &operator = (const matrix4x4_t &m);

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

class YAFRAYCORE_EXPORT paraMap_t
{
	public:
		paraMap_t() = default;
		//! template function to get a value, available types are those of parameter_t::getVal()
		template <class T>
		bool getParam(const std::string &name, T &val) const
		{
			auto i = dicc_.find(name);
			if(i != dicc_.end()) return i->second.getVal(val);
			return false;
		}
		parameter_t &operator [](const std::string &key);

		void clear();
		std::map<std::string, parameter_t>::const_iterator begin() const;
		std::map<std::string, parameter_t>::const_iterator end() const;

	private:
		std::map<std::string, parameter_t> dicc_;
};

__END_YAFRAY
#endif // YAFARAY_PARAMS_H
