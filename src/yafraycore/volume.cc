#include <core_api/volume.h>
#include <core_api/ray.h>
#include <core_api/color.h>
#include <cmath>

__BEGIN_YAFRAY

color_t DensityVolume::tau(const ray_t &ray, float stepSize, float offset) {
		float t0 = -1, t1 = -1;
		
		// ray doesn't hit the BB
		if (!intersect(ray, t0, t1)) {
			//std::cout << "no hit of BB " << t0 << " " << t1 << std::endl;
			return color_t(0.f);
		}
		
		//std::cout << " ray.from: " << ray.from << " ray.dir: " << ray.dir << " ray.tmax: " << ray.tmax << " t0: " << t0 << " t1: " << t1 << std::endl;
		
		if (ray.tmax < t0 && ! (ray.tmax < 0)) return color_t(0.f);
		
		if (ray.tmax < t1 && ! (ray.tmax < 0)) t1 = ray.tmax;
		
		if (t0 < 0.f) t0 = 0.f;
		
		// distance travelled in the volume
		//float dist = t1 - t0;
		
		//std::cout << "dist " << dist << " t0: " << t0 << " t1: " << t1 << std::endl;
		
		//int N = 10;
		float step = stepSize; // dist / (float)N; // length between two sample points along the ray
		//int N = dist / step;
		//--N;
		//if (N < 1) N = 1;
		float pos = t0 + offset * step;
		color_t tauVal(0.f);

		//std::cout << std::endl << "tau, dist: " << (t1 - t0) << " N: " << N <<  " step: " << step << std::endl;

		color_t tauPrev(0.f);
		//int N = 0;

		bool adaptive = false;

		while (pos < t1) {
			//ray_t stepRay(ray.from + (ray.dir * (pos - step)), ray.dir, 0, step, 0);
			color_t tauTmp = sigma_t(ray.from + (ray.dir * pos), ray.dir);


			if (adaptive) {

				float epsilon = 0.01f;
				
				if (std::fabs(tauTmp.energy() - tauPrev.energy()) > epsilon && step > stepSize/50.f) {
					pos -= step;
					step /= 2.f;
				}
				else {
					//++N;
					tauVal += tauTmp * step;
					//if (step < stepSize && std::fabs(tauTmp.energy() - tauPrev.energy()) < epsilon)
					//	step *= 2.f;

					tauPrev = tauTmp;
				}
			}
			else {
				tauVal += tauTmp * step;
				//++N;
			}

			pos += step;

			/* original
			tauVal += sigma_t(ray.from + (ray.dir * pos), ray.dir);
			pos += step;
			*/
		}
		
		//tauVal *= step;
		//tauVal *= (t1-t0) / (float)N;
		return tauVal;
	}

inline float min(float a, float b) { return (a > b) ? b : a; }
inline float max(float a, float b) { return (a < b) ? b : a; }

inline double cosInter(
		double y1,double y2,
		double mu)
{
	double mu2;

	mu2 = (1.0f - fCos(mu*M_PI)) / 2.0f;
	return y1 * (1.0f - mu2) + y2 * mu2;
}

float VolumeRegion::attenuation(const point3d_t p, light_t *l) {
	if (attenuationGridMap.find(l) == attenuationGridMap.end()) {
		std::cout << "attmap not found" << std::endl;
	}

	float* attenuationGrid = attenuationGridMap[l];

	float x = (p.x - bBox.a.x) / bBox.longX() * attGridX - .5f;
	float y = (p.y - bBox.a.y) / bBox.longY() * attGridY - .5f;
	float z = (p.z - bBox.a.z) / bBox.longZ() * attGridZ - .5f;

	// cell vertices in which p lies
	int x0 = max(0, floor(x));
	int y0 = max(0, floor(y));
	int z0 = max(0, floor(z));

	int x1 = min(attGridX - 1, ceil(x));
	int y1 = min(attGridY - 1, ceil(y));
	int z1 = min(attGridZ - 1, ceil(z));

	// offsets
	float xd = x - x0;
	float yd = y - y0;
	float zd = z - z0;
	
	// trilinear combination
	float i1 = attenuationGrid[x0 + y0 * attGridX + attGridY * attGridX * z0] * (1-zd) + attenuationGrid[x0 + y0 * attGridX + attGridY * attGridX * z1] * zd;
	float i2 = attenuationGrid[x0 + y1 * attGridX + attGridY * attGridX * z0] * (1-zd) + attenuationGrid[x0 + y1 * attGridX + attGridY * attGridX * z1] * zd;
	float j1 = attenuationGrid[x1 + y0 * attGridX + attGridY * attGridX * z0] * (1-zd) + attenuationGrid[x1 + y0 * attGridX + attGridY * attGridX * z1] * zd;
	float j2 = attenuationGrid[x1 + y1 * attGridX + attGridY * attGridX * z0] * (1-zd) + attenuationGrid[x1 + y1 * attGridX + attGridY * attGridX * z1] * zd;
	
	float w1 = i1 * (1 - yd) + i2 * yd;
	float w2 = j1 * (1 - yd) + j2 * yd;
	
	float att = w1 * (1 - xd) + w2 * xd;

	/*
	float i1 = cosInter(attenuationGrid[x0 + y0 * attGridX + attGridY * attGridX * z0], attenuationGrid[x0 + y0 * attGridX + attGridY * attGridX * z1], zd);
	float i2 = cosInter(attenuationGrid[x0 + y1 * attGridX + attGridY * attGridX * z0], attenuationGrid[x0 + y1 * attGridX + attGridY * attGridX * z1], zd);
	float j1 = cosInter(attenuationGrid[x1 + y0 * attGridX + attGridY * attGridX * z0], attenuationGrid[x1 + y0 * attGridX + attGridY * attGridX * z1], zd);
	float j2 = cosInter(attenuationGrid[x1 + y1 * attGridX + attGridY * attGridX * z0], attenuationGrid[x1 + y1 * attGridX + attGridY * attGridX * z1], zd);
	
	float w1 = cosInter(i1, i2, yd);
	float w2 = cosInter(j1, j2, yd);
	
	float att = cosInter(w1, w2, xd);
	*/

	//att = attenuationGrid[x0 + y0 * xSize + ySize * xSize * z0];

	//std::cout << "gridpos: " << (x0 + y0 * xSize + ySize * xSize * z0) << " " << x0 << " " << y0 << " " << z0 << " " << x1 << " " << y1 << " " << z1 << " att: " << att << std::endl;

	return att;
}

__END_YAFRAY

