/****************************************************************************
 *   This is part of the libYafaRay package
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "public_api/yafaray_c_api.h"
#include "render/render_control.h"

yafaray_RenderControl *yafaray_createRenderControl()
{
	auto render_control{new yafaray::RenderControl()};
	return reinterpret_cast<yafaray_RenderControl *>(render_control);
}

void yafaray_destroyRenderControl(yafaray_RenderControl *render_control)
{
	delete reinterpret_cast<yafaray::RenderControl *>(render_control);
}

void yafaray_cancelRendering(yafaray_RenderControl *render_control)
{
	if(!render_control) return;
	reinterpret_cast<yafaray::RenderControl *>(render_control)->setCanceled();
}
