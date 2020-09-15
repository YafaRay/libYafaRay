#pragma once
/****************************************************************************
 *
 *      output.h: Output base class
 *      This is part of the libYafaRay package
 *      Copyright (C) 2002  Alejandro Conty Estévez
 *      Modifyed by Rodrigo Placencia Vazquez (2009)
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
#ifndef YAFARAY_OUTPUT_H
#define YAFARAY_OUTPUT_H

#include "constants.h"
#include <vector>
#include <string>

BEGIN_YAFARAY

class PassesSettings;
class Rgba;

/*! Base class for rendering output containers */

class ColorOutput
{
	public:
		virtual ~ColorOutput() = default;
		virtual void initTilesPasses(int total_views, int num_ext_passes) {};
		virtual bool putPixel(int num_view, int x, int y, int ext_pass, const Rgba &color, bool alpha = true) = 0;
		virtual bool putPixel(int num_view, int x, int y, const std::vector<Rgba> &colors, bool alpha = true) = 0;
		virtual void flush(int num_view) = 0;
		virtual void flushArea(int num_view, int x_0, int y_0, int x_1, int y_1) = 0;
		virtual void highlightArea(int num_view, int x_0, int y_0, int x_1, int y_1) {};
		virtual bool isImageOutput() const { return false; }
		virtual bool isPreview() const { return false; }
		virtual std::string getDenoiseParams() const { return ""; }
		void setPassesSettings(const PassesSettings *passes_settings) { passes_settings_ = passes_settings; }

	protected:
		const PassesSettings *passes_settings_ = nullptr;
};

END_YAFARAY

#endif // YAFARAY_OUTPUT_H
