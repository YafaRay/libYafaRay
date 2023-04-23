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

#ifndef LIBYAFARAY_INTEGRATOR_BIDIRECTIONAL_H
#define LIBYAFARAY_INTEGRATOR_BIDIRECTIONAL_H


#include "integrator_tiled.h"
#include "color/color.h"
#include "render/renderer.h"
#include <map>
#include <atomic>

namespace yafaray {

template <typename T, size_t N> class Vec;
typedef Vec<float, 3> Vec3f;
class Light;
class Pdf1D;

class BidirectionalIntegrator final : public TiledIntegrator
{
		using ThisClassType_t = BidirectionalIntegrator; using ParentClassType_t = TiledIntegrator;

	public:
		inline static std::string getClassName() { return "BidirectionalIntegrator"; }
		static std::pair<std::unique_ptr<SurfaceIntegrator>, ParamResult> factory(Logger &logger, const std::string &name, const ParamMap &param_map);
		static std::string printMeta(const std::vector<std::string> &excluded_params) { return class_meta::print<Params>(excluded_params); }
		BidirectionalIntegrator(Logger &logger, ParamResult &param_result, const std::string &name, const ParamMap &param_map);
		[[nodiscard]] ParamMap getAsParamMap(bool only_non_default) const override;

	private:
		[[nodiscard]] Type type() const override { return Type::Bidirectional; }
		const struct Params
		{
			Params(ParamResult &param_result, const ParamMap &param_map);
			static std::map<std::string, const ParamMeta *> getParamMetaMap();
			PARAM_DECL(bool, transparent_shadows_, false, "transpShad", "Use transparent shadows");
			PARAM_DECL(int, shadow_depth_, 4, "shadowDepth", "Shadow depth for transparent shadows");
			PARAM_DECL(bool, ao_, false, "do_AO", "Use ambient occlusion");
			PARAM_DECL(int, ao_samples_, 32, "AO_samples", "Ambient occlusion samples");
			PARAM_DECL(float, ao_distance_, 1.f, "AO_distance", "Ambient occlusion distance");
			PARAM_DECL(Rgb, ao_color_, Rgb{1.f}, "AO_color", "Ambient occlusion color");
			PARAM_DECL(bool, transparent_background_, false, "bg_transp", "Render background as transparent");
			PARAM_DECL(bool, transparent_background_refraction_, false, "bg_transp_refract", "Render refractions of background as transparent");
		} params_;
		bool render(RenderControl &render_control, ImageFilm &image_film, unsigned int object_index_highest, unsigned int material_index_highest) override;
		static constexpr inline int max_path_length_ = 32;
		static constexpr inline int max_path_eval_length_ = 2 * max_path_length_ + 1;
		static constexpr inline int min_path_length_ = 3;
		struct PathData;
		struct PathVertex;
		struct PathEvalVertex;
		bool preprocess(RenderControl &render_control, const Scene &scene) override;
		void cleanup(ImageFilm &image_film) const override;
		std::pair<Rgb, float> integrate(ImageFilm &image_film, Ray &ray, RandomGenerator &random_generator, std::vector<int> &correlative_sample_number, ColorLayers *color_layers, int thread_id, int ray_level, bool chromatic_enabled, float wavelength, int additional_depth, const RayDivision &ray_division, const PixelSamplingData &pixel_sampling_data, unsigned int object_index_highest, unsigned int material_index_highest, float aa_light_sample_multiplier, float aa_indirect_sample_multiplier) override;
		int createPath(RandomGenerator &random_generator, const Accelerator &accelerator, bool chromatic_enabled, float wavelength, const Ray &start, std::vector<PathVertex> &path, int max_len, const Camera *camera) const;
		Rgb evalPath(const Accelerator &accelerator, int s, int t, const PathData &pd, const Camera *camera) const;
		Rgb evalLPath(const Accelerator &accelerator, int t, const PathData &pd, const Ray &l_ray, const Rgb &lcol, const Camera *camera) const;
		Rgb evalPathE(const Accelerator &accelerator, int s, const PathData &pd, const Camera *camera) const;
		std::tuple<bool, Ray, Rgb> connectLPath(PathData &pd, RandomGenerator &random_generator, bool chromatic_enabled, float wavelength, int t) const;
		bool connectPathE(PathData &pd, int s, const Camera *camera) const;
		float pathWeight0T(PathData &pd, int t) const;
		static void clearPath(std::vector<PathEvalVertex> &p, int s, int t);
		static void checkPath(std::vector<PathEvalVertex> &p, int s, int t);
		static void copyLightSubpath(PathData &pd, int s, int t);
		static void copyEyeSubpath(PathData &pd, int s, int t);
		static bool connectPaths(PathData &pd, int s, int t);
		static float pathWeight(int s, int t, const PathData &pd);

		//mutable std::vector<pathVertex_t> lightPath, eyePath;
		//mutable int nPaths;
		//mutable pathData_t pathData;
		mutable std::atomic<int> n_paths_ {0};
		std::unique_ptr<Pdf1D> light_power_d_;
		float f_num_lights_;
		std::map <const Light *, float> inv_light_power_d_;
};

} //namespace yafaray

#endif // LIBYAFARAY_INTEGRATOR_BIDIRECTIONAL_H
