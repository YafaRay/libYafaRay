
#include <core_api/bound.h>
#include <iostream>

BEGIN_YAFRAY

Bound::Bound(const Bound &r, const Bound &l)
{
	float minx = std::min(r.a_.x_, l.a_.x_);
	float miny = std::min(r.a_.y_, l.a_.y_);
	float minz = std::min(r.a_.z_, l.a_.z_);
	float maxx = std::max(r.g_.x_, l.g_.x_);
	float maxy = std::max(r.g_.y_, l.g_.y_);
	float maxz = std::max(r.g_.z_, l.g_.z_);
	a_.set(minx, miny, minz);
	g_.set(maxx, maxy, maxz);
}

float Bound::vol() const
{
	float ret = (g_.y_ - a_.y_) * (g_.x_ - a_.x_) * (g_.z_ - a_.z_);

	return ret;
}

END_YAFRAY
