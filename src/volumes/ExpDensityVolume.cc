#include <core_api/logging.h>
#include <core_api/ray.h>
#include <core_api/color.h>
#include <core_api/volume.h>
#include <core_api/bound.h>
#include <core_api/volume.h>
#include <core_api/surface.h>
#include <core_api/environment.h>
#include <core_api/params.h>

BEGIN_YAFRAY

struct RenderState;
struct PSample;

class ExpDensityVolume final : public DensityVolume
{
	public:
		static VolumeRegion *factory(ParamMap &params, RenderEnvironment &render);

	private:
		ExpDensityVolume(Rgb sa, Rgb ss, Rgb le, float gg, Point3 pmin, Point3 pmax, int attgrid_scale, float aa, float bb) :
			DensityVolume(sa, ss, le, gg, pmin, pmax, attgrid_scale)
		{
			a_ = aa;
			b_ = bb;
			Y_VERBOSE << "ExpDensityVolume vol: " << s_a_ << " " << s_s_ << " " << l_e_ << " " << a_ << " " << b_ << YENDL;
		}
		virtual float density(Point3 p) override;

		float a_, b_;
};

float ExpDensityVolume::density(Point3 p)
{
	float height = p.z_ - b_box_.a_.z_;
	return a_ * fExp__(-b_ * height);
}

VolumeRegion *ExpDensityVolume::factory(ParamMap &params, RenderEnvironment &render)
{
	float ss = .1f;
	float sa = .1f;
	float le = .0f;
	float g = .0f;
	float a = 1.f;
	float b = 1.f;
	float min[] = {0, 0, 0};
	float max[] = {0, 0, 0};
	int att_sc = 1;

	params.getParam("sigma_s", ss);
	params.getParam("sigma_a", sa);
	params.getParam("l_e", le);
	params.getParam("g", g);
	params.getParam("a", a);
	params.getParam("b", b);
	params.getParam("minX", min[0]);
	params.getParam("minY", min[1]);
	params.getParam("minZ", min[2]);
	params.getParam("maxX", max[0]);
	params.getParam("maxY", max[1]);
	params.getParam("maxZ", max[2]);
	params.getParam("attgridScale", att_sc);

	ExpDensityVolume *vol = new ExpDensityVolume(Rgb(sa), Rgb(ss), Rgb(le), g,
												 Point3(min[0], min[1], min[2]), Point3(max[0], max[1], max[2]), att_sc, a, b);
	return vol;
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT void registerPlugin__(RenderEnvironment &render)
	{
		render.registerFactory("ExpDensityVolume", ExpDensityVolume::factory);
	}
}

END_YAFRAY
