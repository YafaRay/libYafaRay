#include <yafray_config.h>
#include <core_api/environment.h>
#include <core_api/material.h>
#include <core_api/integrator.h>
#include <core_api/background.h>
#include <core_api/light.h>
#include <integrators/integr_utils.h>
#include <yafraycore/photon.h>
#include <utilities/mcqmc.h>
#include <yafraycore/scr_halton.h>
#include <utilities/mathOptimizations.h>
#include <vector>
#include <cmath>
#include <stack>

__BEGIN_YAFRAY

class YAFRAYPLUGIN_EXPORT SingleScatterIntegrator : public volumeIntegrator_t {
	private:
		bool adaptive;
		bool optimize;
		float adaptiveStepSize;
		std::vector<VolumeRegion*> listVR;
		std::vector<light_t*> lights;

	public:
	SingleScatterIntegrator(float sSize, bool adapt, bool opt) {
		adaptive = adapt;
		stepSize = sSize;
		optimize = opt;
		adaptiveStepSize = sSize * 100.0f;

		std::cout << "INFO: ScatterIntegrator, stepSize: " << stepSize << " adaptive: " << adaptive 
			<< " optimize: " << optimize << std::endl;
	}

	virtual bool preprocess() {
		// std::cout << "Preprocessing SingleScatterIntegrator" << std::endl;

		for(unsigned int i=0;i<scene->lights.size();++i) {
			lights.push_back(scene->lights[i]);
		}
		if (scene->getBackground()) {
			light_t *bgl = scene->getBackground()->getLight();
			if (bgl) lights.push_back(bgl);
		}

		listVR = scene->getVolumes();
		
		if (optimize) {
			for (unsigned int i = 0; i < listVR.size(); i++) {
			//std::cout << "using vr" << std::endl;
				VolumeRegion* vr = listVR.at(i);
				bound_t bb = vr->getBB();

				int xSize = vr->attGridX;
				int ySize = vr->attGridY;
				int zSize = vr->attGridZ;

				float xSizeInv = 1.f/(float)xSize;
				float ySizeInv = 1.f/(float)ySize;
				float zSizeInv = 1.f/(float)zSize;

				std::cout << "volume, attGridMaps with size: " << xSize << " " << ySize << " " << xSize << std::endl;
			
				for(std::vector<light_t *>::const_iterator l=lights.begin(); l!=lights.end(); ++l) {
					color_t lcol(0.0);

					float* attenuationGrid = (float*)malloc(xSize * ySize * zSize * sizeof(float));
					vr->attenuationGridMap[(*l)] = attenuationGrid;

					for (int z = 0; z < zSize; ++z) {
						for (int y = 0; y < ySize; ++y) {
							for (int x = 0; x < xSize; ++x) {
								// std::cout << "volume " << x << " " << y << " " << x << std::endl;
								// generate the world position inside the grid
								point3d_t p(bb.longX() * xSizeInv * x + bb.a.x,
											bb.longY() * ySizeInv * y + bb.a.y,
											bb.longZ() * zSizeInv * z + bb.a.z);

								surfacePoint_t sp;
								sp.P = p;
								
								ray_t lightRay;

								lightRay.from = sp.P;

								// handle lights with delta distribution, e.g. point and directional lights
								if( (*l)->diracLight() ) {
									bool ill = (*l)->illuminate(sp, lcol, lightRay);
									lightRay.tmin = 0.0005; // < better add some _smart_ self-bias value...this is bad.
									if (lightRay.tmax < 0.f) lightRay.tmax = 1e10; // infinitely distant light

									// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
									//color_t lightstepTau = vr->tau(lightRay, stepSize, 0.5f /* (*state.prng)() */);

									color_t lightstepTau(0.f);
									if (ill) {
										for (unsigned int j = 0; j < listVR.size(); j++) {
											VolumeRegion* vr2 = listVR.at(j);
											lightstepTau += vr2->tau(lightRay, stepSize, 0.0f);
										}
									}

									float lightTr = fExp(-lightstepTau.energy());
									//float lightTr = exp(-lightstepTau.energy());
									attenuationGrid[x + y * xSize + ySize * xSize * z] = lightTr;

									//std::cout << "gridpos: " << (x + y * xSize + ySize * xSize * z) << " " << lightstepTau.energy() << " " << lightRay.dir << " " << lightRay.tmax << " lightTr: " << lightTr << std::endl;

								}
								else // area light and suchlike
								{
									float lightTr = 0;
									int n = (*l)->nSamples() / 2;
									if (n < 1) n = 1;
									lSample_t ls;
									for(int i=0; i<n; ++i)
									{
										ls.s1 = 0.5f; //(*state.prng)();
										ls.s2 = 0.5f; //(*state.prng)();

										(*l)->illumSample(sp, ls, lightRay);
										lightRay.tmin = 0.0005;
										if (lightRay.tmax < 0.f) lightRay.tmax = 1e10; // infinitely distant light

										// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
										//color_t lightstepTau = vr->tau(lightRay, stepSize, 0.5f);
										color_t lightstepTau(0.f);
										for (unsigned int j = 0; j < listVR.size(); j++) {
											VolumeRegion* vr2 = listVR.at(j);
											lightstepTau += vr2->tau(lightRay, stepSize, 0.0f);
										}
										lightTr += fExp(-lightstepTau.energy());
									}

									attenuationGrid[x + y * xSize + ySize * xSize * z] = lightTr / (float)n;
								}
							}
						}
					}
				}
			}
		}

		return true;
	}
	


	color_t getInScatter(renderState_t& state, ray_t& stepRay, float currentStep) const {
		color_t inScatter(0.f);
		surfacePoint_t sp;
		sp.P = stepRay.from;

		ray_t lightRay;
		lightRay.from = sp.P;

		for(std::vector<light_t *>::const_iterator l=lights.begin(); l!=lights.end(); ++l) {
			color_t lcol(0.0);

			// handle lights with delta distribution, e.g. point and directional lights
			if( (*l)->diracLight() ) {
				if( (*l)->illuminate(sp, lcol, lightRay) )
				{
					// ...shadowed...
					lightRay.tmin = YAF_SHADOW_BIAS; // < better add some _smart_ self-bias value...this is bad.
					if (lightRay.tmax < 0.f) lightRay.tmax = 1e10; // infinitely distant light
					bool shadowed = scene->isShadowed(state, lightRay);
					if (!shadowed)
					{
						float lightTr = 0.0f;
						// replace lightTr with precalculated attenuation
						if (optimize) {
							//lightTr = vr->attenuation(sp.P, (*l));
							// replaced by
							for (unsigned int i = 0; i < listVR.size(); i++) {
								VolumeRegion* vr = listVR.at(i);
								float t0Tmp = -1, t1Tmp = -1;
								if (vr->intersect(lightRay, t0Tmp, t1Tmp)) {
									lightTr += vr->attenuation(sp.P, (*l)) / (float)listVR.size();
								}
							}
						}
						else {
							// replaced by
							color_t lightstepTau(0.f);
							for (unsigned int i = 0; i < listVR.size(); i++) {
								VolumeRegion* vr = listVR.at(i);
								float t0Tmp = -1, t1Tmp = -1;
								if (listVR.at(i)->intersect(lightRay, t0Tmp, t1Tmp)) {
									//lightstepTau += vr->tau(lightRay, currentStep * 4.f, (*state.prng)());
									//lightstepTau += vr->tau(lightRay, currentStep, (*state.prng)() * currentStep);
									lightstepTau += vr->tau(lightRay, currentStep, 0.f) / (float)listVR.size();
								}
							}
							// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
							lightTr = fExp(-lightstepTau.energy());
						}
						lightTr /= listVR.size();

						//std::cout << "stepSample: " << stepSample << " " << sigma_s << " " << lcol.energy() << " " << lightTr << std::endl;
						inScatter += lightTr * lcol; // * vr->p(lightRay.dir, -ray.dir);
					}
				}
			}
			else // area light and suchlike
			{
				int n = (*l)->nSamples() / 4;
				if (n < 1) n = 1;
				color_t ccol(0.0);
				float lightTr = 0.0f;
				lSample_t ls;

				for(int i=0; i<n; ++i)
				{
					// ...get sample val...
					ls.s1 = (*state.prng)();
					ls.s2 = (*state.prng)();

					if((*l)->illumSample(sp, ls, lightRay))
					{
						// ...shadowed...
						lightRay.tmin = YAF_SHADOW_BIAS; // < better add some _smart_ self-bias value...this is bad.
						if (lightRay.tmax < 0.f) lightRay.tmax = 1e10; // infinitely distant light
						bool shadowed = scene->isShadowed(state, lightRay);
						if(!shadowed) {
							ccol += ls.col / ls.pdf;

							// replace lightTr with precalculated attenuation
							if (optimize) {
								//lightTr = vr->attenuation(sp.P, (*l));
								// replaced by
								for (unsigned int i = 0; i < listVR.size(); i++) {
									VolumeRegion* vr = listVR.at(i);
									float t0Tmp = -1, t1Tmp = -1;
									if (vr->intersect(lightRay, t0Tmp, t1Tmp)) {
										//lightTr *= vr->attenuation(sp.P, (*l));
										lightTr += vr->attenuation(sp.P, (*l)) / (float)listVR.size();
										break;
									}
								}
							}
							else {
								//color_t lightstepTau = vr->tau(lightRay, step * 4.f,(*state.prng)());
								// replaced by
								color_t lightstepTau(0.f);
								for (unsigned int i = 0; i < listVR.size(); i++) {
									VolumeRegion* vr = listVR.at(i);
									float t0Tmp = -1, t1Tmp = -1;
									if (listVR.at(i)->intersect(lightRay, t0Tmp, t1Tmp)) {
										lightstepTau += vr->tau(lightRay, currentStep * 4.f, 0.0f);
									}
								}
								// transmittance from the point p in the volume to the light (i.e. how much light reaches p)
								lightTr += fExp(-lightstepTau.energy()) / (float)listVR.size();
							}

						}
					}
					lightTr /= listVR.size();
				} // end of area light sample loop

				lightTr /= (float)n;

				ccol = ccol /* * vr->p(lightRay.dir, -ray.dir) */ / (float)n;
				inScatter += lightTr * ccol;
				//inScatter += trTmp * lightTr * sigma_s * ccol;
			} // end of area lights loop
		}

		return inScatter;
	}


	// optical thickness, absorption, attenuation, extinction
	virtual colorA_t transmittance(renderState_t &state, ray_t &ray) const {
		colorA_t Tr(1.f);
		//return Tr;
		
		if (listVR.size() == 0) return Tr;
		
		for (unsigned int i = 0; i < listVR.size(); i++) {
			VolumeRegion* vr = listVR.at(i);
			float t0 = -1, t1 = -1;
			if (vr->intersect(ray, t0, t1)) {
				float random = (*state.prng)();
				color_t opticalThickness = listVR.at(i)->tau(ray, stepSize, random);
				Tr *= colorA_t(fExp(-opticalThickness.energy()));
			}
		}
		
		return Tr;
	}
	
	// emission and in-scattering
	virtual colorA_t integrate(renderState_t &state, ray_t &ray) const {
		float t0 = 1e10f, t1 = -1e10f;

		colorA_t result(0.f);
		//return result;
				
		if (listVR.size() == 0) return result;
		
		bool hit = (ray.tmax > 0.f);

		// find min t0 and max t1
		for (unsigned int i = 0; i < listVR.size(); i++) {
			float t0Tmp, t1Tmp;
			VolumeRegion* vr = listVR.at(i);

			if (!vr->intersect(ray, t0Tmp, t1Tmp)) continue;

			if (hit && ray.tmax < t0Tmp) continue;

			if (t0Tmp < 0.f) t0Tmp = 0.f;

			if (hit && ray.tmax < t1Tmp) t1Tmp = ray.tmax;

			if (t1Tmp > t1) t1 = t1Tmp;
			if (t0Tmp < t0) t0 = t0Tmp;
		}

		float dist = t1-t0;
		if (dist < 1e-3f) return result;

		float pos;
		int samples;
		pos = t0 - (*state.prng)() * stepSize; // start position of ray marching
		dist = t1 - pos;
		samples = dist / stepSize + 1;

		std::vector<float> densitySamples;
		std::vector<float> accumDensity;
		int adaptiveResolution = 1;
		
		if (adaptive) {
			adaptiveResolution = adaptiveStepSize / stepSize;

			densitySamples.resize(samples);
			accumDensity.resize(samples);
			//std::vector<ray_t> stepRays(samples);

			accumDensity.at(0) = 0.f;
			for (int i = 0; i < samples; ++i) {
				point3d_t p = ray.from + (stepSize * i + pos) * ray.dir;

				float density = 0;
				for (unsigned int j = 0; j < listVR.size(); j++) {
					VolumeRegion* vr = listVR.at(j);
					density += vr->sigma_t(p, vector3d_t()).energy();
				}

				densitySamples.at(i) = density;
				if (i > 0)
					accumDensity.at(i) = accumDensity.at(i - 1) + density * stepSize;

				//std::cout << i << " " << stepSize * i + pos << " " << p << " " << densitySamples.at(i) << " " << accumDensity.at(i) << std::endl;
			}
		}


		//std::cout << "singleScat, dist: " << (t1 - t0) << " " << t0 << " " << t1 << std::endl;

		float adaptThresh = .01f;
		bool adaptNow = false;
		float currentStep = stepSize;
		int stepLength = 1;
		int stepToStopAdapt = -1;
		color_t trTmp(1.f); // transmissivity during ray marching

		if (adaptive) {
			currentStep = adaptiveStepSize;
			stepLength = adaptiveResolution;
		}

		color_t stepTau(0.f);
		int lookaheadSamples = adaptiveResolution / 10;

		for (int stepSample = 0; stepSample < samples; stepSample += stepLength) {
			if (adaptive) {
				if (!adaptNow) {
					int nextSample = (stepSample + adaptiveResolution > samples - 1) ? samples - 1 : stepSample + adaptiveResolution;
					//std::cout << "stepSample: " << stepSample << " " << accumDensity.at(stepSample) << " " << accumDensity.at(nextSample) << std::endl;
					if (std::fabs(accumDensity.at(stepSample) - accumDensity.at(nextSample)) > adaptThresh) {
						//std::cout << "stepSample: " << stepSample << " starting adaptive" << std::endl;
						adaptNow = true;
						stepLength = 1;
						stepToStopAdapt = stepSample + lookaheadSamples;
						currentStep = stepSize;
					}
				}
			}

			ray_t stepRay(ray.from + (ray.dir * pos), ray.dir, 0, currentStep, 0);
			//std::cout << "*** stepSample: " << stepSample << " stepTau: " << stepTau << " pos: " << pos << " " << stepRay.from << std::endl;

			if (adaptive) {
				//stepTau += densitySamples.at(stepSample) * currentStep;
				stepTau = accumDensity.at(stepSample);
				//std::cout << densitySamples.at(stepSample) * currentStep << std::endl;
			}
			else {
				for (unsigned int j = 0; j < listVR.size(); j++) {
					VolumeRegion* vr = listVR.at(j);
					float t0Tmp = -1, t1Tmp = -1;
					if (vr->intersect(stepRay, t0Tmp, t1Tmp)) {
						//std::cout << vr->sigma_t(stepRay.from, stepRay.dir) * currentStep << std::endl;
						stepTau += vr->sigma_t(stepRay.from, stepRay.dir) * currentStep;
					}
				}
			}

			trTmp = fExp(-stepTau.energy());

			//std::cout << stepSample << " trTmp: " << trTmp << " sr: " << stepRay.from << std::endl;

			if (optimize && trTmp.energy() < 1e-3f) {
			//if (trTmp.energy() < 1e-3f) {
				float random = (*state.prng)();
				//std::cout << "breaking: " << stepSample << " " << accumDensity.at(stepSample) << " " << densitySamples.at(stepSample)
				//	<< " " << stepTau << std::endl;
				if (random < 0.5f) { 
					// result.R = 0; result.G = 1; result.B = 0;
					break;
				}
				trTmp = trTmp / random;
			}

			//std::cout << "emission: " << stepRay.from << std::endl;
			//resultTmp = trTmp * vr->emission(stepRay.from, stepRay.dir);

			float sigma_s = 0.0f;
			for (unsigned int i = 0; i < listVR.size(); i++) {
				VolumeRegion* vr = listVR.at(i);
				float t0Tmp = -1, t1Tmp = -1;
				if (listVR.at(i)->intersect(stepRay, t0Tmp, t1Tmp)) {
					sigma_s += vr->sigma_s(stepRay.from, stepRay.dir).energy();
				}
			}
			
			//std::cout << "stepSample: " << stepSample << " " << sigma_s << std::endl;

			// with a sigma_s close to 0, no light can be scattered -> computation can be skipped

			if (optimize && sigma_s < 1e-3f) {
			//if (sigma_s < 1e-3) {
				float random = (*state.prng)();
				if (random < 0.5f) {
					//std::cout << "throwing away because ss < 1e-3" << std::endl;
					//stepSample += stepLength;
					pos += currentStep;
					continue;
				}
				sigma_s = sigma_s / random;
			}

			result += trTmp * getInScatter(state, stepRay, currentStep) * sigma_s * currentStep;

			//std::cout << "stepSample: " << stepSample << " " << trTmp << " " << stepTau.energy() << " " << result << std::endl;

			if (adaptive) {
				if (adaptNow && stepSample >= stepToStopAdapt) {
					int nextSample = (stepSample + adaptiveResolution > samples - 1) ? samples - 1 : stepSample + adaptiveResolution;
					if (std::fabs(accumDensity.at(stepSample) - accumDensity.at(nextSample)) > adaptThresh) {
						// continue moving slowly ahead until the discontinuity is found
						stepToStopAdapt = stepSample + lookaheadSamples;
						//std::cout << "stepSample: " << stepSample << " continuing adaptive" << std::endl;
					}
					else {
						//std::cout << "stepSample: " << stepSample << " stopping adaptive" << std::endl;
						adaptNow = false;
						stepLength = adaptiveResolution;
						currentStep = adaptiveStepSize;
					}
				}
			}

			pos += currentStep;
		}

		//std::cout << "done" << std::endl;
		//std::cout << "samples: " << samples << " " << trTmp << " " << stepTau.energy() << std::endl;

		/*
		if (trTmp.energy() > 0.95f) {
			result.R = 1; result.G = result.B = 0;
		}
		*/
		result.A = 1.0f; // FIXME: get correct alpha value, does it even matter?
		return result;
	}
	
	static integrator_t* factory(paraMap_t &params, renderEnvironment_t &render)
	{
		bool adapt = false;
		bool opt = false;
		float sSize = 1.f;
		params.getParam("stepSize", sSize);
		params.getParam("adaptive", adapt);
		params.getParam("optimize", opt);
		SingleScatterIntegrator* inte = new SingleScatterIntegrator(sSize, adapt, opt);
		return inte;
	}

	float stepSize;

};

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("SingleScatterIntegrator", SingleScatterIntegrator::factory);
	}

}

__END_YAFRAY
