#include <yafray_constants.h>

#include <core_api/ray.h>
#include <core_api/color.h>
#include <core_api/volume.h>
#include <core_api/bound.h>
#include <core_api/surface.h>
#include <core_api/texture.h>
#include <core_api/environment.h>
#include <core_api/params.h>
#include <utilities/mcqmc.h>
#include <utilities/mathOptimizations.h>

BEGIN_YAFRAY

struct RenderState;
struct PSample;

class NoiseVolume : public DensityVolume
{
	public:

		NoiseVolume(Rgb sa, Rgb ss, Rgb le, float gg, float cov, float sharp, float dens,
					Point3 pmin, Point3 pmax, int attgrid_scale, Texture *noise) :
			DensityVolume(sa, ss, le, gg, pmin, pmax, attgrid_scale)
		{
			tex_dist_noise_ = noise;
			cover_ = cov;
			sharpness_ = sharp * sharp;
			density_ = dens;
		}

		virtual float density(Point3 p);

		static VolumeRegion *factory(ParamMap &params, RenderEnvironment &render);

	protected:

		Texture *tex_dist_noise_;
		float cover_;
		float sharpness_;
		float density_;
};

float NoiseVolume::density(Point3 p)
{
	float d = tex_dist_noise_->getColor(p * 0.1f).energy();

	d = 1.0f / (1.0f + fExp__(sharpness_ * (1.0f - cover_ - d)));
	d *= density_;

	return d;
}

VolumeRegion *NoiseVolume::factory(ParamMap &params, RenderEnvironment &render)
{
	float ss = .1f;
	float sa = .1f;
	float le = .0f;
	float g = .0f;
	float cov = 1.0f;
	float sharp = 1.0f;
	float dens = 1.0f;
	float min[] = {0, 0, 0};
	float max[] = {0, 0, 0};
	int att_sc = 1;
	std::string tex_name;

	params.getParam("sigma_s", ss);
	params.getParam("sigma_a", sa);
	params.getParam("l_e", le);
	params.getParam("g", g);
	params.getParam("sharpness", sharp);
	params.getParam("density", dens);
	params.getParam("cover", cov);
	params.getParam("minX", min[0]);
	params.getParam("minY", min[1]);
	params.getParam("minZ", min[2]);
	params.getParam("maxX", max[0]);
	params.getParam("maxY", max[1]);
	params.getParam("maxZ", max[2]);
	params.getParam("attgridScale", att_sc);
	params.getParam("texture", tex_name);

	if(tex_name.empty())
	{
		Y_VERBOSE << "NoiseVolume: Noise texture not set, the volume region won't be created." << YENDL;
		return nullptr;
	}

	Texture *noise = render.getTexture(tex_name);

	if(!noise)
	{
		Y_VERBOSE << "NoiseVolume: Noise texture '" << tex_name << "' couldn't be found, the volume region won't be created." << YENDL;
		return nullptr;
	}

	NoiseVolume *vol = new NoiseVolume(Rgb(sa), Rgb(ss), Rgb(le), g, cov, sharp, dens,
									   Point3(min[0], min[1], min[2]), Point3(max[0], max[1], max[2]), att_sc, noise);
	return vol;
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("NoiseVolume", NoiseVolume::factory);
	}
}

END_YAFRAY
