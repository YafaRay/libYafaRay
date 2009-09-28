
#include <core_api/bound.h>
#include <iostream>

__BEGIN_YAFRAY

bound_t::bound_t(const bound_t &r,const bound_t &l)
{
	PFLOAT minx=std::min(r.a.x,l.a.x);
	PFLOAT miny=std::min(r.a.y,l.a.y);
	PFLOAT minz=std::min(r.a.z,l.a.z);
	PFLOAT maxx=std::max(r.g.x,l.g.x);
	PFLOAT maxy=std::max(r.g.y,l.g.y);
	PFLOAT maxz=std::max(r.g.z,l.g.z);
	a.set(minx,miny,minz);
	g.set(maxx,maxy,maxz);
}

GFLOAT bound_t::vol() const
{
	GFLOAT ret=(g.y-a.y)*(g.x-a.x)*(g.z-a.z);
	#ifdef DEBUG
	if( ret<0 ) std::cout<<"warning usorted bounding points\n";
	#endif
	return ret;
}

__END_YAFRAY
