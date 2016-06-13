
#include <core_api/bound.h>
#include <iostream>

__BEGIN_YAFRAY

bound_t::bound_t(const bound_t &r,const bound_t &l)
{
	float minx=std::min(r.a.x,l.a.x);
	float miny=std::min(r.a.y,l.a.y);
	float minz=std::min(r.a.z,l.a.z);
	float maxx=std::max(r.g.x,l.g.x);
	float maxy=std::max(r.g.y,l.g.y);
	float maxz=std::max(r.g.z,l.g.z);
	a.set(minx,miny,minz);
	g.set(maxx,maxy,maxz);
}

float bound_t::vol() const
{
	float ret=(g.y-a.y)*(g.x-a.x)*(g.z-a.z);

	return ret;
}

__END_YAFRAY
