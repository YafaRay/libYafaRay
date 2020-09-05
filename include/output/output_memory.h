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

#ifndef YAFARAY_OUTPUT_MEMORY_H
#define YAFARAY_OUTPUT_MEMORY_H

#include "constants.h"
#include "output/output.h"

BEGIN_YAFARAY

class RenderPasses;

class MemoryInputOutput final : public ColorOutput
{
	public:
		MemoryInputOutput(int resx, int resy, float *i_mem);
		virtual bool putPixel(int num_view, int x, int y, const RenderPasses *render_passes, int idx, const Rgba &color, bool alpha = true) override;
		virtual bool putPixel(int num_view, int x, int y, const RenderPasses *render_passes, const std::vector<Rgba> &col_ext_passes, bool alpha = true) override;
		void flush(int num_view, const RenderPasses *render_passes) override;
		virtual void flushArea(int num_view, int x_0, int y_0, int x_1, int y_1, const RenderPasses *render_passes) override {}; // no tiled file format used...yet
		virtual ~MemoryInputOutput() override;

	private:
		int sizex_, sizey_;
		float *image_mem_;
};


END_YAFARAY

#endif // YAFARAY_OUTPUT_MEMORY_H
