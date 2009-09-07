
#ifndef Y_VMAP_H
#define Y_VMAP_H

#include <vector>
#include <core_api/color.h>

#if HAVE_EXR
#include <half.h>
#endif

__BEGIN_YAFRAY

//! a vertex map class for triangle meshes
class vmap_t
{
	public:
		enum VMAP_TYPE { VM_UNINITIALIZED=0, VM_HALF, VM_FLOAT };
		vmap_t(): type(VM_UNINITIALIZED), dimensions(0){};
		bool init(int maptype, int dimensions, int size);
		void setVal(int triangle, int vertex, float* vals);
		void pushTriVal(float* vals);
		bool getVal(int triangle, float *vals) const;
		int getDimensions() const { return dimensions; }
	protected:
		
		#if HAVE_EXR
		std::vector<half> hmap;
		#endif
		std::vector<CFLOAT> fmap;
		VMAP_TYPE type;
		int dimensions;
};



__END_YAFRAY

#endif // Y_VMAP_H
