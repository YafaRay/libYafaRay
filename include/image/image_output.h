#pragma once
/****************************************************************************
 *
 *      image_output.h: Image Output class
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
#ifndef LIBYAFARAY_IMAGE_OUTPUT_H
#define LIBYAFARAY_IMAGE_OUTPUT_H

#include "public_api/yafaray_c_api.h"
#include "image/badge.h"
#include "image/image_layers.h"
#include "common/enum.h"
#include "common/enum_map.h"
#include "param/class_meta.h"

namespace yafaray {

class RenderView;
class ColorLayers;
class Format;
template <typename T> class Items;

class ImageOutput final
{
	private: struct Type;
	public:
		inline static std::string getClassName() { return "ImageOutput"; }
		static Type type() ;
		void setId(size_t id) { id_ = id; }
		[[nodiscard]] size_t getId() const { return id_; }
		static std::pair<std::unique_ptr<ImageOutput>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return Params::meta_.print(excluded_params); }
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const;
		ImageOutput(Logger &logger, ParamResult &param_result, const ParamMap &param_map);
		void flush(const RenderControl &render_control, const Timer &timer);
		void init(const Size2i &size, const ImageLayers *exported_image_layers, const Items<RenderView> *render_views);
		void setRenderView(const RenderView *render_view) { current_render_view_ = render_view; }
		[[nodiscard]] std::string getName() const { return name_; }
		std::string printBadge(const RenderControl &render_control, const Timer &timer) const;
		std::unique_ptr<Image> generateBadgeImage(const RenderControl &render_control, const Timer &timer) const;

	private:
		struct Type : public Enum<Type>
		{
			using Enum::Enum;
			enum : ValueType_t { ImageOutput };
			inline static const EnumMap<ValueType_t> map_{{
					{"ImageOutput", ImageOutput, ""},
				}};
		};
		const struct Params
		{
			PARAM_INIT;
			PARAM_DECL(std::string, image_path_, "", "image_path", "");
			PARAM_ENUM_DECL(ColorSpace, color_space_, ColorSpace::RawManualGamma, "color_space", "");
			PARAM_DECL(float, gamma_, 1.f, "gamma", "");
			PARAM_DECL(bool, alpha_channel_, false, "alpha_channel", "");
			PARAM_DECL(bool, alpha_premultiply_, false, "alpha_premultiply", "");
			PARAM_DECL(bool, multi_layer_, true, "multi_layer", "");
			PARAM_DECL(bool, denoise_enabled_, false, "denoise_enabled", "");
			PARAM_DECL(int, denoise_h_lum_, 3, "denoise_h_lum", "");
			PARAM_DECL(int, denoise_h_col_, 3, "denoise_h_col", "");
			PARAM_DECL(float, denoise_mix_, 0.8f, "denoise_mix", "Mix factor between the de-noised image and the original \"noisy\" image to avoid banding artifacts in images with all noise removed");
			PARAM_DECL(bool, logging_save_txt_, false, "logging_save_txt", "Enable/disable text log file saving with exported images");
			PARAM_DECL(bool, logging_save_html_, false, "logging_save_html", "Enable/disable HTML file saving with exported images");
			PARAM_ENUM_DECL(Badge::Position, badge_position_, Badge::Position::None, "badge_position", "");
			PARAM_DECL(bool, badge_draw_render_settings_, true, "badge_draw_render_settings", "");
			PARAM_DECL(bool, badge_draw_aa_noise_settings_, true, "badge_draw_aa_noise_settings", "");
			PARAM_DECL(std::string, badge_author_, "", "badge_author", "");
			PARAM_DECL(std::string, badge_title_, "", "badge_title", "");
			PARAM_DECL(std::string, badge_contact_, "", "badge_contact", "");
			PARAM_DECL(std::string, badge_comment_, "", "badge_comment", "");
			PARAM_DECL(std::string, badge_icon_path_, "", "badge_icon_path", "");
			PARAM_DECL(std::string, badge_font_path_, "", "badge_font_path", "");
			PARAM_DECL(float, badge_font_size_factor_, 1.f, "badge_font_size_factor", "");

		} params_;
		bool denoiseEnabled() const { return params_.denoise_enabled_; }
		void saveImageFile(const std::string &filename, LayerDef::Type layer_type, Format *format, const RenderControl &render_control, const Timer &timer);
		void saveImageFileMultiChannel(const std::string &filename, Format *format, const RenderControl &render_control, const Timer &timer);

		size_t id_{0};
		std::string name_ = "out";
		ColorSpace color_space_{params_.color_space_};
		float gamma_{params_.gamma_};
		const DenoiseParams denoise_params_{params_.denoise_enabled_, params_.denoise_h_lum_, params_.denoise_h_col_, params_.denoise_mix_};
		const ImageLayers *image_layers_ = nullptr;
		const RenderView *current_render_view_ = nullptr;
		const Items<RenderView> *render_views_ = nullptr;
		Logger &logger_;
		Badge badge_{
			logger_,
			params_.badge_draw_aa_noise_settings_,
			params_.badge_draw_render_settings_,
			params_.badge_font_size_factor_,
			params_.badge_position_,
			params_.badge_title_,
			params_.badge_author_,
			params_.badge_contact_,
			params_.badge_comment_,
			params_.badge_icon_path_,
			params_.badge_font_path_,
		};
};

} //namespace yafaray

#endif // LIBYAFARAY_IMAGE_OUTPUT_H
