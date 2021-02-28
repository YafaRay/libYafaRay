#pragma once
/****************************************************************************
 *      directlight.cc: an integrator for direct lighting only
 *      This is part of the libYafaRay package
 *      Copyright (C) 2006  Mathias Wein
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

#ifndef YAFARAY_INTEGRATOR_DEBUG_H
#define YAFARAY_INTEGRATOR_DEBUG_H

#include "render/render_view.h"
#include "integrator_tiled.h"
#include "render/imagesplitter.h"

BEGIN_YAFARAY

class Light;

class DebugIntegrator final : public TiledIntegrator
{
	public:
		static std::unique_ptr<Integrator> factory(ParamMap &params, const Scene &scene);

	private:
		enum SurfaceProperties {N = 1, DPdU = 2, DPdV = 3, Nu = 4, Nv = 5, DSdU = 6, DSdV = 7};
		DebugIntegrator(SurfaceProperties dt);
		virtual std::string getShortName() const override { return "DBG"; }
		virtual std::string getName() const override { return "DebugIntegrator"; }
		virtual bool preprocess(const RenderControl &render_control, const RenderView *render_view) override;
		virtual Rgba integrate(RenderData &render_data, const DiffRay &ray, int additional_depth, ColorLayers *color_layers, const RenderView *render_view) const override;

		std::vector<const Light *> lights_;
		SurfaceProperties debug_type_;
		bool show_pn_;
};

END_YAFARAY

#endif // YAFARAY_INTEGRATOR_DEBUG_H