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

#include <core_api/color.h>
#include <utilities/buffer.h>
#include <core_api/output.h>
#include <yafraycore/memoryIO.h>
#include <cstdlib>

BEGIN_YAFRAY


MemoryInputOutput::MemoryInputOutput (int resx, int resy, float *i_mem)
{
	sizex_ = resx;
	sizey_ = resy;
	image_mem_ = i_mem; // iMem must be a valid pointer to memory of the size: sizex * sizey * 4 * sizeof(float)
}

// Depth channel support?

//FIXME: the putpixel functions in MemoryIO are not actually using the Render Passes, always using the Combined pass. Probably no need for this to do anything for now, but in the future it might need to be extended to include all passes
bool MemoryInputOutput::putPixel(int num_view, int x, int y, const RenderPasses *render_passes, int idx, const Rgba &color, bool alpha)
{
	image_mem_[(x + sizex_ * y) * 4 + 0] = color.r_;
	image_mem_[(x + sizex_ * y) * 4 + 0] = color.g_;
	image_mem_[(x + sizex_ * y) * 4 + 0] = color.b_;
	if(!alpha) image_mem_[(x + sizex_ * y) * 4 + 3] = 1.f;

	return true;
}

//FIXME: the putpixel functions in MemoryIO are not actually using the Render Passes, always using the Combined pass. Probably no need for this to do anything for now, but in the future it might need to be extended to include all passes
bool MemoryInputOutput::putPixel(int num_view, int x, int y, const RenderPasses *render_passes, const std::vector<Rgba> &col_ext_passes, bool alpha)
{
	image_mem_[(x + sizex_ * y) * 4 + 0] = col_ext_passes.at(0).r_;
	image_mem_[(x + sizex_ * y) * 4 + 0] = col_ext_passes.at(0).g_;
	image_mem_[(x + sizex_ * y) * 4 + 0] = col_ext_passes.at(0).b_;
	if(!alpha) image_mem_[(x + sizex_ * y) * 4 + 3] = 1.f;

	return true;
}

void MemoryInputOutput::flush(int num_view, const RenderPasses *render_passes) { }

MemoryInputOutput::~MemoryInputOutput() { }


END_YAFRAY

