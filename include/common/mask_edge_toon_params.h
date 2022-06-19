#pragma once
/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef YAFARAY_MASK_EDGE_TOON_PARAMS_H
#define YAFARAY_MASK_EDGE_TOON_PARAMS_H

#include "common/yafaray_common.h"
#include "color/color.h"

namespace yafaray {

struct MaskParams
{
	unsigned int obj_index_; //!Object Index used for masking in/out in the Mask Render Layers
	unsigned int mat_index_; //!Material Index used for masking in/out in the Mask Render Layers
	bool invert_ = false; //!False=mask in, True=mask out
	bool only_ = false; //!False=rendered image is masked, True=only the mask is shown without rendered image
};

struct EdgeToonParams //Options for Edge detection and Toon Render Layers
{
	int thickness_ = 2; //!Thickness of the edges used in the Object Edge and Toon Render Layers
	float threshold_ = 0.3f; //!Threshold for the edge detection process used in the Object Edge and Toon Render Layers
	float smoothness_ = 0.75f; //!Smoothness (blur) of the edges used in the Object Edge and Toon Render Layers
	Rgb toon_color_ {0.f}; //!Color of the edges used in the Toon Render Layers.
	float toon_pre_smooth_ = 3.f; //!Toon effect: smoothness applied to the original image
	float toon_quantization_ = 0.1f; //!Toon effect: color Quantization applied to the original image
	float toon_post_smooth_ = 3.f; //!Toon effect: smoothness applied after Quantization
	int face_thickness_ = 1; //!Thickness of the edges used in the Faces Edge Render Layers
	float face_threshold_ = 0.01f; //!Threshold for the edge detection process used in the Faces Edge Render Layers
	float face_smoothness_ = 0.5f; //!Smoothness (blur) of the edges used in the Faces Edge Render Layers
};

} //namespace yafaray

#endif //LIBYAFARAY_MASK_EDGE_TOON_PARAMS_H
