#pragma once

#ifndef YAFARAY_BACKGROUND_H
#define YAFARAY_BACKGROUND_H

#include <yafray_constants.h>

BEGIN_YAFRAY

struct RenderState;
class Light;
class Rgb;
class Ray;

class YAFRAYCORE_EXPORT Background
{
	public:
		//! get the background color for a given ray
		virtual Rgb operator()(const Ray &ray, RenderState &state, bool from_postprocessed = false) const = 0;
		virtual Rgb eval(const Ray &ray, bool from_postprocessed = false) const = 0;
		/*! get the light source representing background lighting.
			\return the light source that reproduces background lighting, or nullptr if background
					shall only be sampled from BSDFs
		*/
		virtual bool hasIbl() const { return false; }
		virtual bool shootsCaustic() const { return false; }
		virtual ~Background() = default;
};

END_YAFRAY

#endif // YAFARAY_BACKGROUND_H
