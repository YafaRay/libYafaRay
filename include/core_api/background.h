#pragma once

#ifndef Y_BACKGROUND_H
#define Y_BACKGROUND_H

#include <yafray_constants.h>

__BEGIN_YAFRAY

struct renderState_t;
class light_t;
class color_t;
class ray_t;

class YAFRAYCORE_EXPORT background_t
{
	public:
		//! get the background color for a given ray
		virtual color_t operator() (const ray_t &ray, renderState_t &state, bool from_postprocessed=false) const=0;
		virtual color_t eval(const ray_t &ray, bool from_postprocessed=false) const=0;
		/*! get the light source representing background lighting.
			\return the light source that reproduces background lighting, or nullptr if background
					shall only be sampled from BSDFs
		*/
		virtual bool hasIBL() { return false; }
		virtual bool shootsCaustic() { return false; }
		virtual ~background_t() {};
};

__END_YAFRAY

#endif // Y_BACKGROUND_H
