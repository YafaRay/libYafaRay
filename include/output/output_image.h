#pragma once
/****************************************************************************
 *
 *      imageOutput.h: generic color output based on imageHandlers
 *      This is part of the libYafaRay package
 *      Copyright (C) 2010 Rodrigo Placencia Vazquez
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
 *
 */

#ifndef YAFARAY_OUTPUT_IMAGE_H
#define YAFARAY_OUTPUT_IMAGE_H

#include "output/output.h"
#include "imagehandler/imagehandler.h"

BEGIN_YAFARAY

class RenderPasses;

class LIBYAFARAY_EXPORT ImageOutput final : public ColorOutput
{
	public:
		ImageOutput() = default;
		ImageOutput(ImageHandler *handle, const std::string &name, int bx, int by);

	private:
		virtual bool putPixel(int num_view, int x, int y, const RenderPasses *render_passes, int idx, const Rgba &color, bool alpha = true) override;
		virtual bool putPixel(int num_view, int x, int y, const RenderPasses *render_passes, const std::vector<Rgba> &col_ext_passes, bool alpha = true) override;
		virtual void flush(int num_view, const RenderPasses *render_passes) override;
		virtual void flushArea(int num_view, int x_0, int y_0, int x_1, int y_1, const RenderPasses *render_passes) override {} // not used by images... yet
		virtual bool isImageOutput() override { return true; }
		virtual std::string getDenoiseParams() const override
		{
			if(image_) return image_->getDenoiseParams();
			else return "";
		}
		void saveImageFile(std::string filename, int idx);
		void saveImageFileMultiChannel(std::string filename, const RenderPasses *render_passes);

		ImageHandler *image_ = nullptr;
		std::string fname_;
		float b_x_;
		float b_y_;
};

END_YAFARAY

#endif // YAFARAY_OUTPUT_IMAGE_H

