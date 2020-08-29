#include <core_api/params.h>
#include <core_api/vector3d.h>
#include <core_api/color.h>
#include <core_api/matrix4.h>

BEGIN_YAFRAY

Parameter::Parameter(const std::string &s) : type_(String) { sval_ = s; }
Parameter::Parameter(int i) : type_(Int) { ival_ = i; }
Parameter::Parameter(bool b) : type_(Bool) { bval_ = b; }
Parameter::Parameter(float f) : type_(Float) { fval_ = f; }
Parameter::Parameter(double f) : type_(Float) { fval_ = f; }

Parameter::Parameter(const Point3 &p) : type_(Point)
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

bool Parameter::getVal(Point3 &p) const {
	if(type_ == Point)
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

Parameter &Parameter::operator=(const Point3 &p)
{
	if(type_ != Point) { type_ = Point; sval_.clear(); vval_.resize(3); }
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

Parameter &ParamMap::operator[](const std::string &key) { return dicc_[key]; }
void ParamMap::clear() { dicc_.clear(); }

std::map<std::string, Parameter>::const_iterator ParamMap::begin() const { return dicc_.begin(); }
std::map<std::string, Parameter>::const_iterator ParamMap::end() const { return dicc_.end(); }

END_YAFRAY
