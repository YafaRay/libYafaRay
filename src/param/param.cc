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

#include "param/param.h"
#include "geometry/vector.h"
#include "color/color.h"
#include "geometry/matrix.h"
#include <sstream>

namespace yafaray {

Parameter::Parameter(const std::string &s) : type_(Type::String) { sval_ = s; }
Parameter::Parameter(std::string &&s) : type_(Type::String) { sval_ = std::move(s); }
Parameter::Parameter(int i) : type_(Type::Int) { ival_ = i; }
Parameter::Parameter(bool b) : type_(Type::Bool) { bval_ = b; }
Parameter::Parameter(float f) : type_(Type::Float) { fval_ = f; }
Parameter::Parameter(double f) : type_(Type::Float) { fval_ = f; }

Parameter::Parameter(const Vec3f &p) : type_(Type::Vector)
{
	vval_.resize(3); vval_.at(0) = p[Axis::X], vval_.at(1) = p[Axis::Y], vval_.at(2) = p[Axis::Z];
}

Parameter::Parameter(const Rgba &c) : type_(Type::Color)
{
	vval_.resize(4); vval_.at(0) = c.r_, vval_.at(1) = c.g_, vval_.at(2) = c.b_, vval_.at(3) = c.a_;
}

Parameter::Parameter(const Matrix4f &m) : type_(Type::Matrix)
{
	vval_.resize(16);
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j)
			vval_.at(i * 4 + j) = m[i][j];
}

bool Parameter::getVal(std::string &s) const {if(type_ == Type::String) { s = sval_; return true;} return false;}
bool Parameter::getVal(int &i) const {if(type_ == Type::Int)  { i = ival_; return true;} return false;}
bool Parameter::getVal(bool &b) const {if(type_ == Type::Bool)  { b = bval_; return true;} return false;}
bool Parameter::getVal(float &f) const {if(type_ == Type::Float) { f = (float)fval_; return true;} return false;}
bool Parameter::getVal(double &f) const {if(type_ == Type::Float) { f = fval_; return true;} return false;}

bool Parameter::getVal(Vec3f &p) const {
	if(type_ == Type::Vector)
	{
		p[Axis::X] = vval_.at(0), p[Axis::Y] = vval_.at(1), p[Axis::Z] = vval_.at(2);
		return true;
	}
	return false;
}

bool Parameter::getVal(Rgb &c) const {
	if(type_ == Type::Color)
	{
		c.r_ = vval_.at(0), c.g_ = vval_.at(1), c.b_ = vval_.at(2);
		return true;
	}
	return false;
}

bool Parameter::getVal(Rgba &c) const {
	if(type_ == Type::Color)
	{
		c.r_ = vval_.at(0), c.g_ = vval_.at(1), c.b_ = vval_.at(2), c.a_ = vval_.at(3);
		return true;
	}
	return false;
}

bool Parameter::getVal(Matrix4f &m) const
{
	if(type_ == Type::Matrix)
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
	if(type_ != Type::String) { type_ = Type::String; vval_.clear(); }
	sval_ = s;
	return *this;
}

Parameter &Parameter::operator=(std::string &&s)
{
	if(type_ != Type::String) { type_ = Type::String; vval_.clear(); }
	sval_ = std::move(s);
	return *this;
}

Parameter &Parameter::operator=(int i)
{
	if(type_ != Type::Int) { type_ = Type::Int; vval_.clear(); sval_.clear(); }
	ival_ = i;
	return *this;
}

Parameter &Parameter::operator=(bool b)
{
	if(type_ != Type::Bool) { type_ = Type::Bool; vval_.clear(); sval_.clear(); }
	bval_ = b;
	return *this;
}

Parameter &Parameter::operator=(float f)
{
	if(type_ != Type::Float) { type_ = Type::Float; vval_.clear(); sval_.clear(); }
	fval_ = f;
	return *this;
}

Parameter &Parameter::operator=(const Vec3f &p)
{
	if(type_ != Type::Vector) { type_ = Type::Vector; sval_.clear(); vval_.resize(3); }
	vval_.at(0) = p[Axis::X], vval_.at(1) = p[Axis::Y], vval_.at(2) = p[Axis::Z];
	return *this;
}

Parameter &Parameter::operator=(const Rgb &c)
{
	if(type_ != Type::Color) { type_ = Type::Color; sval_.clear(); vval_.resize(4); }
	const Rgba col{c};
	vval_.at(0) = col.r_, vval_.at(1) = col.g_, vval_.at(2) = col.b_, vval_.at(3) = col.a_;
	return *this;
}

Parameter &Parameter::operator=(const Rgba &c)
{
	if(type_ != Type::Color) { type_ = Type::Color; sval_.clear(); vval_.resize(4); }
	vval_.at(0) = c.r_, vval_.at(1) = c.g_, vval_.at(2) = c.b_, vval_.at(3) = c.a_;
	return *this;
}

Parameter &Parameter::operator=(const Matrix4f &m)
{
	if(type_ != Type::Matrix) { type_ = Type::Matrix; sval_.clear(); vval_.resize(16); }
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j)
			vval_.at(i * 4 + j) = m[i][j];
	return *this;
}

std::string Parameter::print() const
{
	if(type_ == Type::Int)
	{
		int value;
		getVal(value);
		return std::to_string(value);
	}
	else if(type_ == Type::Bool)
	{
		bool value;
		getVal(value);
		return value == true ? "true" : "false";
	}
	else if(type_ == Type::Float)
	{
		double value;
		getVal(value);
		return std::to_string(value);
	}
	else if(type_ == Type::String)
	{
		std::string value;
		getVal(value);
		return value;
	}
	else if(type_ == Type::Vector)
	{
		Point3f value;
		getVal(value);
		return "(x:" + std::to_string(value[Axis::X]) + ", y:" + std::to_string(value[Axis::X]) + ", z:" + std::to_string(value[Axis::Z]) + ")";
	}
	else if(type_ == Type::Color)
	{
		Rgba value;
		getVal(value);
		return "(r:" + std::to_string(value.r_) + ", g:" + std::to_string(value.g_) + ", b:" + std::to_string(value.b_) + ", a:" + std::to_string(value.a_) + ")";
	}
	else if(type_ == Type::Matrix)
	{
		Matrix4f value;
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
		case Type::Int: return "Int";
		case Type::Bool: return "Bool";
		case Type::Float: return "Float";
		case Type::String: return "String";
		case Type::Vector: return "Vector";
		case Type::Color: return "Color";
		case Type::Matrix: return "Matrix";
		default: return "None/Unknown";
	}
}

bool Parameter::operator==(const Parameter &parameter) const
{
	return (
			fval_ == parameter.fval_ &&
			sval_ == parameter.sval_ &&
			vval_ == parameter.vval_ &&
			type_ == parameter.type_
	);
}

bool Parameter::operator!=(const Parameter &parameter) const
{
	return (
			fval_ != parameter.fval_ ||
			sval_ != parameter.sval_ ||
			vval_ != parameter.vval_ ||
			type_ != parameter.type_
	);
}

std::string ParamMap::print() const
{
	std::string result;
	for(const auto &[param_name, param] : items_)
	{
		result += "'" + param_name + "' (" + param.printType() + ") = '" + param.print() + "'\n";
	}
	return result;
}

std::string ParamMap::logContents() const
{
	std::stringstream ss;
	for(const auto &[param_name, param] : items_)
	{
		ss << "'" << param_name << "' (" << param.printType() << ") = '" << param.print() << "'" << std::endl;
	}
	return ss.str();
}

void ParamMap::setInputColorSpace(const std::string &color_space_string, float gamma_val)
{
	input_color_space_.initFromString(color_space_string);
	input_gamma_ = gamma_val;
}

bool ParamMap::operator==(const ParamMap &param_map) const
{
	return (
			items_ == param_map.items_ &&
			input_gamma_ == param_map.input_gamma_ &&
			input_color_space_ == param_map.input_color_space_
	);
}

bool ParamMap::operator!=(const ParamMap &param_map) const
{
	return (
			items_ != param_map.items_ ||
			input_gamma_ != param_map.input_gamma_ ||
			input_color_space_ != param_map.input_color_space_
	);
}

void writeMatrix(const std::string &matrix_name, const Matrix4f &m, std::stringstream &ss, yafaray_ContainerExportType container_export_type) noexcept
{
	if(container_export_type == YAFARAY_CONTAINER_EXPORT_C || container_export_type == YAFARAY_CONTAINER_EXPORT_PYTHON)
	{
		ss <<
		   m[0][0] << ", " << m[0][1] << ", " << m[0][2] << ", " << m[0][3] << ", " <<
		   m[1][0] << ", " << m[1][1] << ", " << m[1][2] << ", " << m[1][3] << ", " <<
		   m[2][0] << ", " << m[2][1] << ", " << m[2][2] << ", " << m[2][3] << ", " <<
		   m[3][0] << ", " << m[3][1] << ", " << m[3][2] << ", " << m[3][3];
	}
	else if(container_export_type == YAFARAY_CONTAINER_EXPORT_XML)
	{
		ss << "<" << matrix_name << " m00=\"" << m[0][0] << "\" m01=\"" << m[0][1] << "\" m02=\"" << m[0][2] << "\" m03=\"" << m[0][3] << "\""
			 << " m10=\"" << m[1][0] << "\" m11=\"" << m[1][1] << "\" m12=\"" << m[1][2] << "\" m13=\"" << m[1][3] << "\""
			 << " m20=\"" << m[2][0] << "\" m21=\"" << m[2][1] << "\" m22=\"" << m[2][2] << "\" m23=\"" << m[2][3] << "\""
			 << " m30=\"" << m[3][0] << "\" m31=\"" << m[3][1] << "\" m32=\"" << m[3][2] << "\" m33=\"" << m[3][3] << "\"/>";
	}
}

std::string ParamMap::exportMap(size_t indent_level, yafaray_ContainerExportType container_export_type, bool only_export_non_default_parameters, const std::map<std::string, const ParamMeta *> &param_meta_map, const std::vector<std::string> &excluded_params_meta) const
{
	std::stringstream ss;
	for(const auto &[param_name, param] : items_)
	{
		const Parameter::Type type = param.type();
		if(container_export_type == YAFARAY_CONTAINER_EXPORT_C)
		{
			if(type == Parameter::Type::Int)
			{
				int i = 0;
				param.getVal(i);
				ss << "yafaray_setParamMapInt(yi, \"" << param_name << "\", " << i << ");";
			}
			else if(type == Parameter::Type::Bool)
			{
				bool b = false;
				param.getVal(b);
				ss << "yafaray_setParamMapBool(yi, \"" << param_name << "\", " << (b ? "true" : "false") << ");";
			}
			else if(type == Parameter::Type::Float)
			{
				double f = 0.0;
				param.getVal(f);
				ss << "yafaray_setParamMapFloat(yi, \"" << param_name << "\", " << f << ");";
			}
			else if(type == Parameter::Type::String)
			{
				std::string s;
				param.getVal(s);
				ss << "yafaray_setParamMapString(yi, \"" << param_name << "\", \"" << s << "\");";
			}
			else if(type == Parameter::Type::Vector)
			{
				Point3f p{{0.f, 0.f, 0.f}};
				param.getVal(p);
				ss << "yafaray_setParamMapVector(yi, \"" << param_name << "\", " << p[Axis::X] << ", " << p[Axis::Y] << ", " << p[Axis::Z] << ");";
			}
			else if(type == Parameter::Type::Color)
			{
				Rgba c(0.f);
				param.getVal(c);
				c.colorSpaceFromLinearRgb(input_color_space_, input_gamma_);    //Color values are encoded to the desired color space before saving them to the XML file
				ss << "yafaray_setParamMapColor(yi, \"" << param_name << "\", " << c.r_ << ", " << c.g_ << ", " << c.b_ << ", " << c.a_ << ");";
			}
			else if(type == Parameter::Type::Matrix)
			{
				Matrix4f m;
				param.getVal(m);
				ss << "yafaray_setParamMapMatrix(yi, ";
				writeMatrix(param_name, m, ss, container_export_type);
				ss << ", false);";
			}
		}
		else if(container_export_type == YAFARAY_CONTAINER_EXPORT_PYTHON)
		{
			ss << std::string(indent_level, '\t');
			if(type == Parameter::Type::Int)
			{
				int i = 0;
				param.getVal(i);
				ss << "yafaray_setParamMapInt(yi, \"" << param_name << "\", " << i << ");";
			}
			else if(type == Parameter::Type::Bool)
			{
				bool b = false;
				param.getVal(b);
				ss << "yafaray_setParamMapBool(yi, \"" << param_name << "\", " << (b ? "true" : "false") << ");";
			}
			else if(type == Parameter::Type::Float)
			{
				double f = 0.0;
				param.getVal(f);
				ss << "yafaray_setParamMapFloat(yi, \"" << param_name << "\", " << f << ");";
			}
			else if(type == Parameter::Type::String)
			{
				std::string s;
				param.getVal(s);
				ss << "yafaray_setParamMapString(yi, \"" << param_name << "\", \"" << s << "\");";
			}
			else if(type == Parameter::Type::Vector)
			{
				Point3f p{{0.f, 0.f, 0.f}};
				param.getVal(p);
				ss << "yafaray_setParamMapVector(yi, \"" << param_name << "\", " << p[Axis::X] << ", " << p[Axis::Y] << ", " << p[Axis::Z] << ");";
			}
			else if(type == Parameter::Type::Color)
			{
				Rgba c(0.f);
				param.getVal(c);
				c.colorSpaceFromLinearRgb(input_color_space_, input_gamma_);    //Color values are encoded to the desired color space before saving them to the XML file
				ss << "yafaray_setParamMapColor(yi, \"" << param_name << "\", " << c.r_ << ", " << c.g_ << ", " << c.b_ << ", " << c.a_ << ");";
			}
			else if(type == Parameter::Type::Matrix)
			{
				Matrix4f m;
				param.getVal(m);
				ss << "yafaray_setParamMapMatrix(yi, ";
				writeMatrix(param_name, m, ss, container_export_type);
				ss << ", false);";
			}
		}
		else if(container_export_type == YAFARAY_CONTAINER_EXPORT_XML)
		{
			ss << std::string(indent_level, '\t');
			if(type == Parameter::Type::Int)
			{
				int i = 0;
				param.getVal(i);
				ss << "<" << param_name << " ival=\"" << i << "\"/>";
			}
			else if(type == Parameter::Type::Bool)
			{
				bool b = false;
				param.getVal(b);
				ss << "<" << param_name << " bval=\"" << b << "\"/>";
			}
			else if(type == Parameter::Type::Float)
			{
				double f = 0.0;
				param.getVal(f);
				ss << "<" << param_name << " fval=\"" << f << "\"/>";
			}
			else if(type == Parameter::Type::String)
			{
				std::string s;
				param.getVal(s);
				ss << "<" << param_name << " sval=\"" << s << "\"/>";
			}
			else if(type == Parameter::Type::Vector)
			{
				Point3f p{{0.f, 0.f, 0.f}};
				param.getVal(p);
				ss << "<" << param_name << " x=\"" << p[Axis::X] << "\" y=\"" << p[Axis::Y] << "\" z=\"" << p[Axis::Z] << "\"/>";
			}
			else if(type == Parameter::Type::Color)
			{
				Rgba c(0.f);
				param.getVal(c);
				c.colorSpaceFromLinearRgb(input_color_space_, input_gamma_);    //Color values are encoded to the desired color space before saving them to the XML file
				ss << "<" << param_name << " r=\"" << c.r_ << "\" g=\"" << c.g_ << "\" b=\"" << c.b_ << "\" a=\"" << c.a_ << "\"/>";
			}
			else if(type == Parameter::Type::Matrix)
			{
				Matrix4f m;
				param.getVal(m);
				writeMatrix(param_name, m, ss, container_export_type);
			}
		}
		bool skip_param_meta = false;
		for(const auto &excluded_param_meta: excluded_params_meta)
		{
			if(param_name == excluded_param_meta)
			{
				skip_param_meta = true;
				break;
			}
		}
		if(!skip_param_meta)
		{
			const auto param_meta{findPointerInMap(param_meta_map, param_name)};
			if(param_meta)
			{
				if(!param_meta->desc().empty())
				{
					if(container_export_type == YAFARAY_CONTAINER_EXPORT_C)
					{
						ss << " /* " << param_meta->desc() << " */";
					}
					else if(container_export_type == YAFARAY_CONTAINER_EXPORT_PYTHON)
					{
						ss << " #" << param_meta->desc();
					}
					else if(container_export_type == YAFARAY_CONTAINER_EXPORT_XML)
					{
						ss << " <!-- " << param_meta->desc() << " -->";
					}
				}
			}
			else
			{
				constexpr std::string_view error_unknown{"This parameter is *unknown* and probably not currently used by libYafaRay"};
				if(container_export_type == YAFARAY_CONTAINER_EXPORT_C)
				{
					ss << " /* " << error_unknown << " */";
				}
				else if(container_export_type == YAFARAY_CONTAINER_EXPORT_PYTHON)
				{
					ss << " #" << error_unknown;
				}
				else if(container_export_type == YAFARAY_CONTAINER_EXPORT_XML)
				{
					ss << " <!-- " << error_unknown << " -->";
				}
			}
		}
		ss << std::endl;
	}
	return ss.str();
}

} //namespace yafaray
