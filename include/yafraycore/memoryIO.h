#pragma once

#ifndef YAFARAY_MEMORYIO_H
#define YAFARAY_MEMORYIO_H

#include <yafray_constants.h>
#include <core_api/output.h>

BEGIN_YAFRAY

class RenderPasses;

class YAFRAYCORE_EXPORT MemoryInputOutput : public ColorOutput
{
	public:
		MemoryInputOutput(int resx, int resy, float *i_mem);
		virtual bool putPixel(int num_view, int x, int y, const RenderPasses *render_passes, int idx, const Rgba &color, bool alpha = true);
		virtual bool putPixel(int num_view, int x, int y, const RenderPasses *render_passes, const std::vector<Rgba> &col_ext_passes, bool alpha = true);
		void flush(int num_view, const RenderPasses *render_passes);
		virtual void flushArea(int num_view, int x_0, int y_0, int x_1, int y_1, const RenderPasses *render_passes) {}; // no tiled file format used...yet
		virtual ~MemoryInputOutput();
	protected:
		int sizex_, sizey_;
		float *image_mem_;
};


END_YAFRAY

#endif // YAFARAY_MEMORYIO_H
