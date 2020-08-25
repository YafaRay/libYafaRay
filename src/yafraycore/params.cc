#include <core_api/params.h>
#include <core_api/vector3d.h>
#include <core_api/color.h>
#include <core_api/matrix4.h>

__BEGIN_YAFRAY

parameter_t::parameter_t(const std::string &s) : type_(String) { sval_ = s; }
parameter_t::parameter_t(int i) : type_(Int) { ival_ = i; }
parameter_t::parameter_t(bool b) : type_(Bool) { bval_ = b; }
parameter_t::parameter_t(float f) : type_(Float) { fval_ = f; }
parameter_t::parameter_t(double f) : type_(Float) { fval_ = f; }

parameter_t::parameter_t(const point3d_t &p) : type_(Point)
{
	vval_.resize(3); vval_.at(0) = p.x, vval_.at(1) = p.y, vval_.at(2) = p.z;
}

parameter_t::parameter_t(const colorA_t &c) : type_(Color)
{
	vval_.resize(4); vval_.at(0) = c.R, vval_.at(1) = c.G, vval_.at(2) = c.B, vval_.at(3) = c.A;
}

parameter_t::parameter_t(const matrix4x4_t &m) : type_(Matrix)
{
	vval_.resize(16);
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j)
			vval_.at(i * 4 + j) = m[i][j];
}

bool parameter_t::getVal(std::string &s) const {if(type_ == String) { s = sval_; return true;} return false;}
bool parameter_t::getVal(int &i) const {if(type_ == Int)  { i = ival_; return true;} return false;}
bool parameter_t::getVal(bool &b) const {if(type_ == Bool)  { b = bval_; return true;} return false;}
bool parameter_t::getVal(float &f) const {if(type_ == Float) { f = (float)fval_; return true;} return false;}
bool parameter_t::getVal(double &f) const {if(type_ == Float) { f = fval_; return true;} return false;}

bool parameter_t::getVal(point3d_t &p) const {
	if(type_ == Point)
	{
		p.x = vval_.at(0), p.y = vval_.at(1), p.z = vval_.at(2);
		return true;
	}
	return false;
}

bool parameter_t::getVal(color_t &c) const {
	if(type_ == Color)
	{
		c.R = vval_.at(0), c.G = vval_.at(1), c.B = vval_.at(2);
		return true;
	}
	return false;
}

bool parameter_t::getVal(colorA_t &c) const {
	if(type_ == Color)
	{
		c.R = vval_.at(0), c.G = vval_.at(1), c.B = vval_.at(2), c.A = vval_.at(3);
		return true;
	}
	return false;
}

bool parameter_t::getVal(matrix4x4_t &m) const
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

parameter_t &parameter_t::operator=(const std::string &s)
{
	if(type_ != String) { type_ = String; vval_.clear(); }
	sval_ = s;
	return *this;
}

parameter_t &parameter_t::operator=(int i)
{
	if(type_ != Int) { type_ = Int; vval_.clear(); sval_.clear(); }
	ival_ = i;
	return *this;
}

parameter_t &parameter_t::operator=(bool b)
{
	if(type_ != Bool) { type_ = Bool; vval_.clear(); sval_.clear(); }
	bval_ = b;
	return *this;
}

parameter_t &parameter_t::operator=(float f)
{
	if(type_ != Float) { type_ = Float; vval_.clear(); sval_.clear(); }
	fval_ = f;
	return *this;
}

parameter_t &parameter_t::operator=(const point3d_t &p)
{
	if(type_ != Point) { type_ = Point; sval_.clear(); vval_.resize(3); }
	vval_.at(0) = p.x, vval_.at(1) = p.y, vval_.at(2) = p.z;
	return *this;
}
parameter_t &parameter_t::operator=(const colorA_t &c)
{
	if(type_ != Color) { type_ = Color; sval_.clear(); vval_.resize(4); }
	vval_.at(0) = c.R, vval_.at(1) = c.G, vval_.at(2) = c.B, vval_.at(3) = c.A;
	return *this;
}

parameter_t &parameter_t::operator=(const matrix4x4_t &m)
{
	if(type_ != Matrix) { type_ = Matrix; sval_.clear(); vval_.resize(16); }
	for(int i = 0; i < 4; ++i)
		for(int j = 0; j < 4; ++j)
			vval_.at(i * 4 + j) = m[i][j];
	return *this;
}

parameter_t &paraMap_t::operator[](const std::string &key) { return dicc_[key]; }
void paraMap_t::clear() { dicc_.clear(); }

std::map<std::string, parameter_t>::const_iterator paraMap_t::begin() const { return dicc_.begin(); }
std::map<std::string, parameter_t>::const_iterator paraMap_t::end() const { return dicc_.end(); }

__END_YAFRAY
