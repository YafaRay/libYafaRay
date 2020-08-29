#pragma once

#ifndef YAFARAY_PARAMS_H
#define YAFARAY_PARAMS_H

#include <yafray_constants.h>
#include <map>
#include <vector>
#include <string>

BEGIN_YAFRAY

class Rgb;
class Rgba;
class Point3;
class Matrix4;

/*! a class that can hold exactly one value of a range types.
*/
class YAFRAYCORE_EXPORT Parameter
{
	public:
		enum Type : int { None = -1, Int = 1, Bool, Float, String, Point, Color, Matrix };
		//! return the type of the parameter_t
		Type type() const { return type_; }

		Parameter() = default;
		Parameter(const std::string &s);
		Parameter(int i);
		Parameter(bool b);
		Parameter(float f);
		Parameter(double f);
		Parameter(const Point3 &p);
		Parameter(const Rgba &c);
		Parameter(const Matrix4 &m);

		// return parameter value in reference parameter, return true if
		// the parameter type matches, false otherwise.
		bool getVal(std::string &s) const;
		bool getVal(int &i) const;
		bool getVal(bool &b) const;
		bool getVal(float &f) const;
		bool getVal(double &f) const;
		bool getVal(Point3 &p) const;
		bool getVal(Rgb &c) const;
		bool getVal(Rgba &c) const;
		bool getVal(Matrix4 &m) const;

		// operator= assigns new value, be aware that this may change the parameter type!
		Parameter &operator = (const std::string &s);
		Parameter &operator = (int i);
		Parameter &operator = (bool b);
		Parameter &operator = (float f);
		Parameter &operator = (const Point3 &p);
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

class YAFRAYCORE_EXPORT ParamMap
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

		void clear();
		std::map<std::string, Parameter>::const_iterator begin() const;
		std::map<std::string, Parameter>::const_iterator end() const;

	private:
		std::map<std::string, Parameter> dicc_;
};

END_YAFRAY
#endif // YAFARAY_PARAMS_H
