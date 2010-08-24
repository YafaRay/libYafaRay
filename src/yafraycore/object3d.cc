#include <yafraycore/meshtypes.h>
#include <cstdlib>

__BEGIN_YAFRAY

triangleObject_t::triangleObject_t(int ntris, bool hasUV, bool hasOrco):
	has_orco(hasOrco), has_uv(hasUV), has_vcol(false), is_smooth(false)
{
	triangles.reserve(ntris);
	if(hasUV)
	{
		uv_offsets.reserve(ntris);
	}
}

int triangleObject_t::getPrimitives(const triangle_t **prims)
{
	for(unsigned int i=0; i < triangles.size(); ++i)
	{
		prims[i] = &(triangles[i]);
	}
	return triangles.size();
}

triangle_t* triangleObject_t::addTriangle(const triangle_t &t)
{
	triangles.push_back(t);
	return &(triangles.back());
}

void triangleObject_t::setContext(std::vector<point3d_t>::iterator p, std::vector<normal_t>::iterator n)
{
	points = p;
	normals = n;
}

void triangleObject_t::finish()
{
	for(std::vector<triangle_t>::iterator i=triangles.begin(); i!= triangles.end(); ++i)
	{
		(*i).recNormal();
	}
}

/*===================
	meshObject_t methods
=====================================*/

meshObject_t::meshObject_t(int ntris, bool hasUV, bool hasOrco):
	has_orco(hasOrco), has_uv(hasUV), has_vcol(false), is_smooth(false), light(0)
{
	//triangles.reserve(ntris);
	if(hasUV)
	{
		uv_offsets.reserve(ntris);
	}
}

int meshObject_t::getPrimitives(const primitive_t **prims) const
{
	int n=0;
	for(unsigned int i=0; i < triangles.size(); ++i, ++n)
	{
		prims[n] = &(triangles[i]);
	}
	for(unsigned int i=0; i < s_triangles.size(); ++i, ++n)
	{
		prims[n] = &(s_triangles[i]);
	}
	return n;
}

primitive_t* meshObject_t::addTriangle(const vTriangle_t &t)
{
	triangles.push_back(t);
	return &(triangles.back());
}

primitive_t* meshObject_t::addBsTriangle(const bsTriangle_t &t)
{
	s_triangles.push_back(t);
	return &(triangles.back());
}

void meshObject_t::setContext(std::vector<point3d_t>::iterator p, std::vector<normal_t>::iterator n)
{
	points = p;
	normals = n;
}

void meshObject_t::finish()
{
	for(std::vector<vTriangle_t>::iterator i=triangles.begin(); i!= triangles.end(); ++i)
	{
		(*i).recNormal();
	}
}

__END_YAFRAY
