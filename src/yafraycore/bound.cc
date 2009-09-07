
#include <core_api/bound.h>
#include <iostream>

__BEGIN_YAFRAY

#define MIN(a,b) ( ((a)<(b)) ? (a) : (b))
#define MAX(a,b) ( ((a)>(b)) ? (a) : (b))


bound_t::bound_t(const bound_t &r,const bound_t &l)
{
	PFLOAT minx=MIN(r.a.x,l.a.x);
	PFLOAT miny=MIN(r.a.y,l.a.y);
	PFLOAT minz=MIN(r.a.z,l.a.z);
	PFLOAT maxx=MAX(r.g.x,l.g.x);
	PFLOAT maxy=MAX(r.g.y,l.g.y);
	PFLOAT maxz=MAX(r.g.z,l.g.z);
	a.set(minx,miny,minz);
	g.set(maxx,maxy,maxz);
}

GFLOAT bound_t::vol() const
{
	GFLOAT ret=(g.y-a.y)*(g.x-a.x)*(g.z-a.z);
	if( ret<0 ) std::cout<<"warning usorted bounding points\n";
	return ret;
}

bool bound_t::cross(const point3d_t &from,const vector3d_t &ray,PFLOAT &where,PFLOAT dist)const
{
		const point3d_t &a0=a,&a1=g;
		vector3d_t p;
		p=from-a0;

    PFLOAT lmin=-1, lmax=-1,tmp1,tmp2;
		if(ray.x!=0.0)
		{
	    tmp1 =               -p.x/ray.x;
	    tmp2 = ((a1.x-a0.x)-p.x)/ray.x;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    lmin = tmp1;
	    lmax = tmp2;	// si  < 0, podemos devolver false
			if((lmax<0) || (lmin>dist)) return false;
		}
		if(ray.y!=0.0)
		{
	    tmp1 = -p.y/ray.y;
	    tmp2 = ((a1.y-a0.y)-p.y)/ray.y;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    if (tmp1 > lmin)
	        lmin = tmp1;
	    //if (tmp2 < lmax)
	    if ((tmp2 < lmax) || (lmax<0))
	        lmax = tmp2;	// si  < 0, podemos devolver false
			if((lmax<0) || (lmin>dist)) return false;
		}
		if(ray.z!=0.0)
		{
	    tmp1 = -p.z/ray.z;
	    tmp2 = ((a1.z-a0.z)-p.z)/ray.z;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    if (tmp1 > lmin)
	        lmin = tmp1;
	    if ((tmp2 < lmax) || (lmax<0))
	        lmax = tmp2;	
		} 
		if((lmin <= lmax) && (lmax >= 0) && (lmin<=dist))
		{
			where=(lmin>0) ? lmin : 0;
			return true;
		}
		else return false;
}

bool bound_t::cross(const point3d_t &from,const vector3d_t &ray,PFLOAT &enter,PFLOAT &leave,PFLOAT dist)const
{
		const point3d_t &a0=a,&a1=g;
		vector3d_t p;
		p=from-a0;

    PFLOAT lmin=-1, lmax=-1,tmp1,tmp2;
		if(ray.x!=0.0)
		{
	    tmp1 =               -p.x/ray.x;
	    tmp2 = ((a1.x-a0.x)-p.x)/ray.x;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    lmin = tmp1;
	    lmax = tmp2;	// si  < 0, podemos devolver false
			if((lmax<0) || (lmin>dist)) return false;
		}
		if(ray.y!=0.0)
		{
	    tmp1 = -p.y/ray.y;
	    tmp2 = ((a1.y-a0.y)-p.y)/ray.y;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    if (tmp1 > lmin)
	        lmin = tmp1;
	    //if (tmp2 < lmax)
	    if ((tmp2 < lmax) || (lmax<0))
	        lmax = tmp2;	// si  < 0, podemos devolver false
			if((lmax<0) || (lmin>dist)) return false;
		}
		if(ray.z!=0.0)
		{
	    tmp1 = -p.z/ray.z;
	    tmp2 = ((a1.z-a0.z)-p.z)/ray.z;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    if (tmp1 > lmin)
	        lmin = tmp1;
	    if ((tmp2 < lmax) || (lmax<0))
	        lmax = tmp2;	
		} 
		if((lmin <= lmax) && (lmax >= 0) && (lmin<=dist))
		{
			enter=/*(lmin>0) ? */lmin/* : 0*/;
			leave=lmax;
			return true;
		}
		else return false;
}

__END_YAFRAY
