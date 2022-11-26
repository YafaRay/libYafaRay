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
 *
 */

#ifndef YAFARAY_RENDER_CALLBACKS_H
#define YAFARAY_RENDER_CALLBACKS_H

struct RenderCallbacks final
{
	yafaray_RenderNotifyViewCallback notify_view_ = nullptr;
	void *notify_view_data_ = nullptr;
	yafaray_RenderNotifyLayerCallback notify_layer_ = nullptr;
	void *notify_layer_data_ = nullptr;
	yafaray_RenderPutPixelCallback put_pixel_ = nullptr;
	void *put_pixel_data_ = nullptr;
	yafaray_RenderHighlightPixelCallback highlight_pixel_ = nullptr;
	void *highlight_pixel_data_ = nullptr;
	yafaray_RenderFlushAreaCallback flush_area_ = nullptr;
	void *flush_area_data_ = nullptr;
	yafaray_RenderFlushCallback flush_ = nullptr;
	void *flush_data_ = nullptr;
	yafaray_RenderHighlightAreaCallback highlight_area_ = nullptr;
	void *highlight_area_data_ = nullptr;
};

#endif //LIBYAFARAY_RENDER_CALLBACKS_H
