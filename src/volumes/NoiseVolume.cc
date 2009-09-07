#include <yafray_config.h>

#include <core_api/ray.h>
#include <core_api/color.h>
#include <core_api/volume.h>
#include <core_api/bound.h>
#include <core_api/surface.h>
#include <core_api/texture.h>
#include <core_api/environment.h>
//#include <textures/noise.h>
//#include <textures/basictex.h>
#include <utilities/mcqmc.h>
#include <utilities/mathOptimizations.h>

__BEGIN_YAFRAY

class renderState_t;
class pSample_t;

class NoiseVolume : public DensityVolume {
	public:
	
		NoiseVolume(color_t sa, color_t ss, color_t le, float gg, float cov, float sharp, float dens,
				point3d_t pmin, point3d_t pmax, int attgridScale, texture_t* noise) :
			DensityVolume(sa, ss, le, gg, pmin, pmax, attgridScale) {
			texDistNoise = noise;
			cover = cov;
			sharpness = sharp * sharp;
			density = dens;
			std::cout << "NoiseVolume vol: " << s_a << " " << s_s << " " << l_e << " " 
				<< sharp << " " << cov << " " << dens << " " << attgridScale << std::endl;
		}
		
		virtual float Density(point3d_t p);
				
		static VolumeRegion* factory(paraMap_t &params, renderEnvironment_t &render);
	
	protected:

		texture_t* texDistNoise;
		float cover;
		float sharpness;
		float density;
};

float NoiseVolume::Density(const point3d_t p) {
	float d = texDistNoise->getColor(p * 0.1f).energy();

	d = 1.0f / (1.0f + fExp(sharpness * (1.0f - cover - d)));
	d *= density;
	
	return d;
}

VolumeRegion* NoiseVolume::factory(paraMap_t &params,renderEnvironment_t &render)
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
	int attSc = 1;
	
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
	params.getParam("attgridScale", attSc);
	
	texture_t* noise = render.getTexture("TEmytex");
	
	if (noise == 0) std::cout << "mytex not found" << std::endl;
	
	NoiseVolume *vol = new NoiseVolume(color_t(sa), color_t(ss), color_t(le), g, cov, sharp, dens,
						point3d_t(min[0], min[1], min[2]), point3d_t(max[0], max[1], max[2]), attSc, noise);
	return vol;
}

extern "C"
{	
	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("NoiseVolume", NoiseVolume::factory);
	}
}

__END_YAFRAY
