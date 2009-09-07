
#include <yafraycore/vmap.h>

__BEGIN_YAFRAY

bool vmap_t::init(int maptype, int dim, int size)
{
	if(maptype != VM_HALF && maptype != VM_FLOAT) return false;
	type = (VMAP_TYPE) maptype;
	dimensions = dim;
	if(type == VM_HALF)
	{
#if HAVE_EXR
		hmap.resize(size*dimensions);
		return true;
#else
		type = VM_FLOAT;
		fmap.resize(size*dimensions);
		return true;
#endif
	}
	else if(type == VM_FLOAT)
	{
		fmap.resize(size*dimensions);
		return true;
	}
	return false;
}

void vmap_t::setVal(int triangle, int vertex, float* vals)
{
	int index = (3*triangle + vertex)*dimensions;
	switch(type)
	{
		case VM_HALF:
#if HAVE_EXR
			for(int i=0; i<dimensions; ++i) hmap[index+i] = (half) vals[i];
#endif
			break;
			
		case VM_FLOAT:
			for(int i=0; i<dimensions; ++i) fmap[index+i] = vals[i];
			break;
		default: break;
	}
}

void vmap_t::pushTriVal(float* vals)
{
	int n = 3*dimensions;
	switch(type)
	{
		case VM_HALF:
#if HAVE_EXR
			for(int i=0; i<n; ++i) hmap.push_back((half) vals[i]);
#endif
			break;
			
		case VM_FLOAT:
			for(int i=0; i<n; ++i) fmap.push_back(vals[i]);
			break;
		default: break;
	}
}

/* void vmap_t::setVal(int triangle, float* vals)
{
	int index = 3*triangle*dimensions, n = 3*dimensions;
	switch(type)
	{
		case VM_HALF:
			for(int i=0; i<n; ++i) hmap[index+i] = (half) vals[i];
			break;
			
		case VM_FLOAT:
			for(int i=0; i<n; ++i) fmap[index+i] = vals[i];
			break;
		default: break;
	}
} */

/*! get the vmap values in (d1,d2,..dn),(d1,d2...dn),(d1,d2,..dn) order
	\param vals must point to an array of required size (3*dimensions) */
bool vmap_t::getVal(int triangle, float *vals) const
{
	int n = 3*dimensions;
	int index = n*triangle;
	switch(type)
	{
		case VM_HALF:
#if HAVE_EXR
			for(int i=0; i<n; ++i) vals[i] = hmap[index+i];
#endif
			break;
			
		case VM_FLOAT:
			for(int i=0; i<n; ++i) vals[i] = fmap[index+i];
			break;
		default: break;
	}
	return true;
}

__END_YAFRAY

