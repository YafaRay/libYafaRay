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

#include "color/color.h"
#include "output/output_memory.h"
#include "color/color_layers.h"
#include "common/param.h"
#include "scene/scene.h"

BEGIN_YAFARAY

UniquePtr_t<ColorOutput> MemoryInputOutput::factory(const ParamMap &params, const Scene &scene)
{
	std::string name;
	int width = 0;
	int height = 0;
	std::string color_space_str;
	float gamma = 1.f;
	bool alpha_premultiply = false;

	params.getParam("name", name);
	params.getParam("width", width);
	params.getParam("height", height);
	params.getParam("color_space", color_space_str);
	params.getParam("gamma", gamma);
	params.getParam("alpha_premultiply", alpha_premultiply);

	const ColorSpace color_space = Rgb::colorSpaceFromName(color_space_str);
	float *image_mem = nullptr; //FIXME image_mem???
	return UniquePtr_t<ColorOutput>(new MemoryInputOutput(width, height, image_mem, name, color_space, gamma, alpha_premultiply));
}

// Depth channel support?
//FIXME: the putpixel functions in MemoryIO are not actually using the Render Passes, always using the Combined pass. Probably no need for this to do anything for now, but in the future it might need to be extended to include all passes
bool MemoryInputOutput::putPixel(int x, int y, const ColorLayer &color_layer)
{
	image_mem_[(x + width_ * y) * 4 + 0] = color_layer.color_.r_;
	image_mem_[(x + width_ * y) * 4 + 1] = color_layer.color_.g_;
	image_mem_[(x + width_ * y) * 4 + 2] = color_layer.color_.b_;
	image_mem_[(x + width_ * y) * 4 + 3] = color_layer.color_.a_;
	return true;
}

void MemoryInputOutput::flush(const RenderControl &render_control) { }

END_YAFARAY

