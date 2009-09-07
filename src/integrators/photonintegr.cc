/****************************************************************************
 *      photonintegr.cc: integrator for photon mapping and final gather
 *      This is part of the yafray package
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

//#include <mcqmc.h>
#include <integrators/photonintegr.h>

__BEGIN_YAFRAY

photonIntegrator_t::photonIntegrator_t(int photons, bool transpShad, int shadowDepth, float dsRad):
	trShad(transpShad), finalGather(true), cacheIrrad(false), nPhotons(photons), sDepth(shadowDepth), dsRadius(dsRad)
{
	type = SURFACE;
	rDepth = 6;
	maxBounces = 5;
#if OLD_PMAP > 0
	diffuseMap.setMaxRadius(fSqrt(dsRad)); causticMap.setMaxRadius(fSqrt(dsRad));
#endif
}

struct preGatherData_t
{
	preGatherData_t(photonMap_t *dm, photonMap_t *cm): diffuseMap(dm), causticMap(cm), fetched(0) {}
	photonMap_t *diffuseMap, *causticMap;
	
	std::vector<radData_t> rad_points;
	//std::vector<normal_t> rad_normals;
	//std::vector<point3d_t> rad_points;
	//std::vector<color_t> rad_refl;
	//std::vector<color_t> rad_transm;
	std::vector<photon_t> radianceVec;
	progressBar_t *pbar;
	volatile int fetched;
	yafthreads::mutex_t mutex;
};

class preGatherWorker_t: public yafthreads::thread_t
{
	public:
		preGatherWorker_t(preGatherData_t *dat, PFLOAT rad, int search):
			gdata(dat), dsRadius_2(rad*rad), nSearch(search) {};
		virtual void body();
	protected:
		preGatherData_t *gdata;
		PFLOAT dsRadius_2;
		int nSearch;
};

void preGatherWorker_t::body()
{
	unsigned int start, end, total;
	
	gdata->mutex.lock();
	start = gdata->fetched;
	total = gdata->rad_points.size();
	end = gdata->fetched = std::min(total, start + 32);
	gdata->mutex.unlock();
	
	foundPhoton_t *gathered = new foundPhoton_t[nSearch];
	float diffPaths = (float)gdata->diffuseMap->nPaths();
	while(start < total)
	{
		for(unsigned int n=start; n<end; ++n)
		{
			PFLOAT radius = dsRadius_2; //actually the square radius...
			int nGathered = gdata->diffuseMap->gather(gdata->rad_points[n].pos, gathered, nSearch, radius);
			vector3d_t rnorm = gdata->rad_points[n].normal;
			color_t sum(0.0);
			if(nGathered > 0)
			{
				color_t surfCol = gdata->rad_points[n].refl;
				float scale = 1.f / ( diffPaths * radius * M_PI);
				if(isnan(scale)){ std::cout << "NaN WARNING (scale)" << std::endl; break; }
				for(int i=0; i<nGathered; ++i)
				{
					vector3d_t pdir = gathered[i].photon->direction();
					
					if( rnorm * pdir > 0.f ) sum += surfCol * scale * gathered[i].photon->color();
					else sum += gdata->rad_points[n].transm * scale * gathered[i].photon->color();
				}
			}
			gdata->radianceVec[n] = photon_t(rnorm, gdata->rad_points[n].pos, sum);
		}
		gdata->mutex.lock();
		start = gdata->fetched;
		end = gdata->fetched = std::min(total, start + 32);
		gdata->pbar->update(32);
		gdata->mutex.unlock();
	}
	delete[] gathered;
}

photonIntegrator_t::~photonIntegrator_t()
{
}

inline color_t photonIntegrator_t::estimateOneDirect(renderState_t &state, const surfacePoint_t &sp, vector3d_t wo, const std::vector<light_t *>  &lights, int d1, int n)const
{
	//static bool debug=true;
	color_t lcol(0.0), scol, col(0.0);
	ray_t lightRay;
	float lightPdf;
	bool shadowed;
	const material_t *oneMat = sp.material;
	lightRay.from = sp.P;
	int nLightsI = lights.size();
	if(nLightsI == 0) return color_t(0.f);
	float nLights = float(nLightsI);
	float s1;
	if(d1 > 50)  s1 = (*state.prng)();
	else s1 = scrHalton(d1, n) * nLights;
	int lnum = (int)(s1);
	if(lnum > nLightsI-1) lnum = nLightsI-1;
	const light_t *light = lights[lnum];
	s1 = s1 - (float)lnum; // scrHalton(d1, n); // 
	//BSDF_t oneBSDFs = oneMat->getFlags();
	//oneMat->initBSDF(state, sp, oneBSDFs);
	// handle lights with delta distribution, e.g. point and directional lights
	if( light->diracLight() )
	{
		if( light->illuminate(sp, lcol, lightRay) )
		{
			// ...shadowed...
			lightRay.tmin = YAF_SHADOW_BIAS; // < better add some _smart_ self-bias value...this is bad.
			shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
			if(!shadowed)
			{
				if(trShad) lcol *= scol;
				color_t surfCol = oneMat->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
				col = surfCol * lcol * std::fabs(sp.N*lightRay.dir);
			}
		}
	}
	else // area light and suchlike
	{
		//color_t ccol(0.0);
		// ...get sample val...
		lSample_t ls;
		ls.s1 = s1;
		if(d1 > 49)  ls.s2 = (*state.prng)();
		else ls.s2 = scrHalton(d1+1, n);
		bool canIntersect=light->canIntersect();
		
		if( light->illumSample (sp, ls, lightRay) )
		{
			// ...shadowed...
			lightRay.tmin = YAF_SHADOW_BIAS; // < better add some _smart_ self-bias value...this is bad.
			shadowed = (trShad) ? scene->isShadowed(state, lightRay, sDepth, scol) : scene->isShadowed(state, lightRay);
			if(!shadowed && ls.pdf > 1e-6f)
			{
				if(trShad) ls.col *= scol;
				//color_t surfCol = bsdf->eval(sp, wo, lightRay.dir, userdata);
				color_t surfCol = oneMat->eval(state, sp, wo, lightRay.dir, BSDF_ALL);
				
				if( canIntersect ) // bound samples and compensate by sampling from BSDF later
				{
					float mPdf = oneMat->pdf(state, sp, wo, lightRay.dir, BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE | BSDF_REFLECT | BSDF_TRANSMIT);
					float l2 = ls.pdf * ls.pdf;
					float m2 = mPdf * mPdf + 0.01f;
					float w = l2 / (l2 + m2);
					//test! limit lightPdf...
					//if(lightPdf > 1.f) lightPdf = 1.f;
					col = surfCol * ls.col * std::fabs(sp.N*lightRay.dir) * w / ls.pdf;
				}
				else
				{
					//test! limit lightPdf...
					if(ls.pdf < 0.0001f) ls.pdf = 0.0001f;
					col = surfCol * ls.col * std::fabs(sp.N*lightRay.dir) / ls.pdf;
				}
			}
		}
		if(canIntersect) // sample from BSDF to complete MIS
		{
			//color_t ccol2(0.f);

			ray_t bRay;
			bRay.tmin = YAF_SHADOW_BIAS;
			bRay.from = sp.P;
			sample_t s(ls.s1, ls.s2, BSDF_GLOSSY | BSDF_DIFFUSE | BSDF_DISPERSIVE | BSDF_REFLECT | BSDF_TRANSMIT);
			color_t surfCol = oneMat->sample(state, sp, wo, bRay.dir, s);
			if( s.pdf>1e-6f && light->intersect(bRay, bRay.tmax, lcol, lightPdf) )
			{
				shadowed = (trShad) ? scene->isShadowed(state, bRay, sDepth, scol) : scene->isShadowed(state, bRay);
				if(!shadowed)
				{
					if(trShad) lcol *= scol;
					float lPdf = 1.f/lightPdf;
					float l2 = lPdf * lPdf;
					float m2 = s.pdf * s.pdf + 0.01f;
					float w = m2 / (l2 + m2);
					CFLOAT cos2 = std::fabs(sp.N*bRay.dir);
					col += surfCol * lcol * cos2 * w / s.pdf;
				}
			}
		}
	}
	return col*nLights;
}

bool photonIntegrator_t::render(imageFilm_t *image)
{
	imageFilm = image;
	scene->getAAParameters(AA_samples, AA_passes, AA_inc_samples, AA_threshold);
	std::cout << "rendering "<<AA_passes<<" passes, min " << AA_samples << " samples, " << 
				AA_inc_samples << " per additional pass (max "<<AA_samples + std::max(0,AA_passes-1)*AA_inc_samples<<" total)\n";
	gTimer.addEvent("rendert");
	gTimer.start("rendert");
	imageFilm->init();
	
	this->prepass = false;
	if(cacheIrrad)
	{
		//this->prepass = true;
		//renderPass(1, 0, false);
		//this->prepass = false;
		renderIrradPass();
		imageFilm->init();
	}
	renderPass(AA_samples, 0, false);
	for(int i=1; i<AA_passes; ++i)
	{
		imageFilm->setAAThreshold(AA_threshold);
		imageFilm->nextPass(true);
		renderPass(AA_inc_samples, AA_samples + (i-1)*AA_inc_samples, true);
		int s = scene->getSignals();
		if(s & Y_SIG_ABORT) break;
	}
	gTimer.stop("rendert");
	std::cout << "overall rendertime: "<< gTimer.getTime("rendert")<<"s\n";
//	surfIntegrator->cleanup();
//	imageFilm->flush();
	return true;
}

bool photonIntegrator_t::preprocess()
{
	diffuseMap.clear();
	causticMap.clear();
	background = scene->getBackground();
	lights = scene->lights;
	if(background)
	{
		light_t *bgl = background->getLight();
		if(bgl) lights.push_back(bgl);
	}
	//stats:
	int _nIntersect=0, _nDiffuse=0;
	//end stats:
	ray_t ray;
	float lightNumPdf, lightPdf, s1, s2, s3, s4, s5, s6, s7, sL;
	int numLights = lights.size();
	float fNumLights = (float)numLights;
	float *energies = new float[numLights];
	color_t pcol;
	for(int i=0;i<numLights;++i) energies[i] = lights[i]->totalEnergy().energy();
	lightPowerD = new pdf1D_t(energies, numLights);
	for(int i=0;i<numLights;++i) std::cout << "energy: "<< energies[i] <<" (dirac: "<<lights[i]->diracLight()<<")\n";
	for(int i=0;i<numLights;++i)
	{
		pcol = lights[i]->emitPhoton(.5, .5, .5, .5, ray, lightPdf);
		lightNumPdf = lightPowerD->func[i] * lightPowerD->invIntegral;
		pcol *= fNumLights*lightPdf/lightNumPdf; //remember that lightPdf is the inverse of the pdf, hence *=...
		std::cout << "photon col:"<<pcol<<" lnpdf: "<<lightNumPdf<<"\n";
	}
	delete[] energies;
	//shoot photons
	bool done=false;
	unsigned int curr=0;
	// for radiance map:
	preGatherData_t pgdat(&diffuseMap, &causticMap);
	
	surfacePoint_t sp;
	renderState_t state;
	unsigned char userdata[USER_DATA_SIZE+7];
	state.userdata = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
	while(!done)
	{
		state.chromatic = true;
		state.wavelength = RI_S(curr);
		s1 = RI_vdC(curr);
		s2 = scrHalton(2, curr);
		s3 = scrHalton(3, curr);
		s4 = scrHalton(4, curr);
		//sL = RI_S(curr);
		sL = float(curr) / float(nPhotons);
		int lightNum = lightPowerD->DSample(sL, &lightNumPdf);
		if(lightNum >= numLights){ std::cout << "lightPDF sample error! "<<sL<<"/"<<lightNum<<"\n"; delete lightPowerD; return false; }
		
		pcol = lights[lightNum]->emitPhoton(s1, s2, s3, s4, ray, lightPdf);
		ray.tmin = 0.0005;
		ray.tmax = -1.0;
		pcol *= fNumLights*lightPdf/lightNumPdf; //remember that lightPdf is the inverse of th pdf, hence *=...
		if(pcol.isBlack())
		{
			++curr;
			done = (curr >= nPhotons) ? true : false;
			continue;
		}
		int nBounces=0;
		bool causticPhoton = false;
		bool directPhoton = true;
		while( scene->intersect(ray, sp) )
		{
			++_nIntersect;
			if(isnan(pcol.R) || isnan(pcol.G) || isnan(pcol.B))
			{ std::cout << "NaN WARNING (photon color)" << std::endl; break; }
			vector3d_t wi = -ray.dir, wo;
			const material_t *material = sp.material;
			BSDF_t bsdfs;
			material->initBSDF(state, sp, bsdfs);
			if(bsdfs & (BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_REFLECT))
			{
				++_nDiffuse;
				//deposit photon on surface
				if(causticPhoton)
				{
					photon_t np(wi, sp.P, pcol);
					causticMap.pushPhoton(np);
					causticMap.setNumPaths(curr);
				}
				else
				{
					photon_t np(wi, sp.P, pcol);
					diffuseMap.pushPhoton(np);
					diffuseMap.setNumPaths(curr);
				}
				// create entry for radiance photon:
				// don't forget to choose subset only, face normal forward; geometric vs. smooth normal?
				if(finalGather && ourRandom() < 0.125 )
				{
					vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wi);
					radData_t rd(sp.P, N);
					//pgdat.rad_mats.push_back(sp.material);
					rd.refl = material->getReflectivity(state, sp, BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_REFLECT);
					rd.transm = material->getReflectivity(state, sp, BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_TRANSMIT);
					//if(rad_refl.size() < 10) std::cout << "reflectivity: " << rad_refl.back() <<
					//			"transmissivity: " <<  rad_transm.back() << std::endl;
					pgdat.rad_points.push_back(rd);
				}
			}
			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(nBounces == maxBounces) break;
			// scatter photon
			int d5 = 3*nBounces + 5;
			//int d6 = d5 + 1;

			// scrHalton is not a good choice for dimensions bigger than 50, and in those cases 
			// using random numbers might be a better choice.
			if(d5+2 <= 50)
			{
				s5 = scrHalton(d5, curr);
				s6 = scrHalton(d5+1, curr);
				s7 = scrHalton(d5+2, curr);
			}
			else
			{
				s5 = ourRandom();
				s6 = ourRandom();
				s7 = ourRandom();
			}
			pSample_t sample(s5, s6, s7, BSDF_ALL, pcol);
			//color_t fcol;
			//bool scattered = material->scatterPhoton(sp, wi, wo, s5, s6, BSDF_ALL, bsdfs, fcol);
			bool scattered = material->scatterPhoton(state, sp, wi, wo, sample);
			if(!scattered) break; //photon was absorped.
			//pcol *= fcol;
			pcol = sample.color;
			/* causticPhoton = (sample.sampledFlags & BSDF_SPECULAR) && (!(sample.sampledFlags & BSDF_FILTER))
							&& (directPhoton || causticPhoton); */
			causticPhoton = ((sample.sampledFlags & (BSDF_SPECULAR | BSDF_DISPERSIVE)) && directPhoton) ||
							((sample.sampledFlags & (BSDF_SPECULAR | BSDF_FILTER | BSDF_DISPERSIVE)) && causticPhoton);
			// light through transparent materials can be calculated by direct lighting, so still consider them direct!
			directPhoton = (sample.sampledFlags & BSDF_FILTER) && directPhoton;
			if(state.chromatic && sample.sampledFlags & BSDF_DISPERSIVE)
			{
				state.chromatic=false;
				color_t wl_col;
				wl2rgb(state.wavelength, wl_col);
				pcol *= wl_col;
			}
			ray.from = sp.P;
			ray.dir = wo;
			ray.tmin = 0.0005;
			ray.tmax = -1.0;
			++nBounces;
		}
		++curr;
		done = (curr >= nPhotons) ? true : false;
	}
	delete lightPowerD;
	std::cout << "shot "<<curr<<" photons, "<<_nIntersect<<" hits, "<<_nDiffuse<<" of them on diffuse srf.\n";
	std::cout << "stored caustic photons: "<<causticMap.nPhotons()<<"\n";
	std::cout << "stored diffuse photons: "<<diffuseMap.nPhotons()<<"\n";
	std::cout << "building photon kd-trees...\n";
	if(causticMap.nPhotons() > 0) causticMap.updateTree();
	if(diffuseMap.nPhotons() > 0) diffuseMap.updateTree();
	std::cout << "done!\n";
	if(diffuseMap.nPhotons() < 50)
	{ std::cout<<"too few photons! Stop.\n"; return false; }
	
	gTimer.addEvent("pregather");
	lookupRad = 4*dsRadius*dsRadius;
	if(finalGather) //create radiance map:
	{
		gTimer.start("pregather");
#ifdef USING_THREADS
		// == remove too close radiance points ==//
		kdtree::pointKdTree< radData_t > *rTree = new kdtree::pointKdTree< radData_t >(pgdat.rad_points);
		std::vector< radData_t > cleaned;
		for(unsigned int i=0; i<pgdat.rad_points.size(); ++i)
		{
			if(pgdat.rad_points[i].use)
			{
				cleaned.push_back(pgdat.rad_points[i]);
				eliminatePhoton_t elimProc(pgdat.rad_points[i].normal);
				PFLOAT maxrad = 0.01f*dsRadius; // 10% of diffuse search radius
				rTree->lookup(pgdat.rad_points[i].pos, elimProc, maxrad);
			}
		}
		pgdat.rad_points.swap(cleaned);
		// ================ //
		int nThreads = scene->getNumThreads();
		pgdat.radianceVec.resize(pgdat.rad_points.size());
		pgdat.pbar = new ConsoleProgressBar_t(80);
		pgdat.pbar->init(pgdat.rad_points.size());
		std::vector<preGatherWorker_t *> workers;
		for(int i=0; i<nThreads; ++i) workers.push_back(new preGatherWorker_t(&pgdat, dsRadius, nSearch));
		
		for(int i=0;i<nThreads;++i) workers[i]->run();
		for(int i=0;i<nThreads;++i) workers[i]->wait();
		for(int i=0;i<nThreads;++i) delete workers[i];
		
		radianceMap.swapVector(pgdat.radianceVec);
		pgdat.pbar->done();
		delete pgdat.pbar;
#else
		if(radianceMap.nPhotons() != 0){ std::cout << "Preprocess: [WARNING]: radianceMap not empty!\n"; radianceMap.clear(); }
		std::cout << "creating radiance map..." << std::endl;
		progressBar_t *pbar = new ConsoleProgressBar_t(80);
		pbar->init(pgdat.rad_points.size());
		foundPhoton_t *gathered = (foundPhoton_t *)malloc(nSearch * sizeof(foundPhoton_t));
		PFLOAT dsRadius_2 = dsRadius*dsRadius;
		for(unsigned int n=0; n< pgdat.rad_points.size(); ++n)
		{
			PFLOAT radius = dsRadius_2; //actually the square radius...
			int nGathered = diffuseMap.gather(pgdat.rad_points[n].pos, gathered, nSearch, radius);
			color_t sum(0.0);
			if(nGathered > 0)
			{
				color_t surfCol = pgdat.rad_points[n].refl;
				vector3d_t rnorm = pgdat.rad_points[n].normal;
				float scale = 1.f / ( float(diffuseMap.nPaths()) * radius * M_PI);
				if(isnan(scale)){ std::cout << "NaN WARNING (scale)" << std::endl; break; }
				for(int i=0; i<nGathered; ++i)
				{
					vector3d_t pdir = gathered[i].photon->direction();
					
					if( rnorm * pdir > 0.f ) sum += surfCol * scale * gathered[i].photon->color();
					else sum += pgdat.rad_points[n].transm * scale * gathered[i].photon->color();
				}
			}
			photon_t radP(pgdat.rad_points[n].normal, pgdat.rad_points[n].pos, sum);
			radianceMap.pushPhoton(radP);
			if(n && !(n&7)) pbar->update(8);
		}
		pbar->done();
		delete pbar;
		free(gathered);
#endif
		gTimer.stop("pregather");
		std::cout << gTimer.getTime("pregather") << "sec\nbuilding radiance tree..." << std::endl;
		radianceMap.updateTree();
		std::cout << "done!\n";
	}
	//irradiance cache
	if(cacheIrrad)
	{
		irCache.init(*scene, 1.f);
	}
	//delete lightPowerD;
	return true;
}

// final gathering: this is basically a full path tracer only that it uses the radiance map only
// at the path end. I.e. paths longer than 1 are only generated to overcome lack of local radiance detail.
// precondition: initBSDF of current spot has been called!
color_t photonIntegrator_t::finalGathering(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo) const
{
	color_t pathCol(0.0);
	void *first_udat = state.userdata;
	unsigned char userdata[USER_DATA_SIZE+7];
	void *n_udat = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
	
	int nSampl = std::max(1, nPaths/state.rayDivision);
	for(int i=0; i<nSampl; ++i)
	{
		color_t throughput( 1.0 );
		PFLOAT length=0;
		surfacePoint_t hit=sp;
		vector3d_t pwo = wo;
		ray_t pRay;
		BSDF_t matBSDFs;
		bool did_hit;
		const material_t *p_mat = sp.material;
		unsigned int offs = nPaths * state.pixelSample + state.samplingOffs + i; // some redundancy here...
		color_t lcol, scol;
		// "zero'th" FG bounce:
		float s1 = RI_vdC(offs);
		float s2 = scrHalton(2, offs);
		if(state.rayDivision > 1)
		{
			s1 = addMod1(s1, state.dc1);
			s2 = addMod1(s2, state.dc2);
		}
		//sample_t s(s1, s2, BSDF_ALL&~BSDF_SPECULAR); // specular done via recursive raytracing
		sample_t s(s1, s2, BSDF_DIFFUSE|BSDF_REFLECT|BSDF_TRANSMIT); // specular/glossy done via recursive raytracing
		scol = p_mat->sample(state, hit, pwo, pRay.dir, s);
		if(s.pdf > 1.0e-6f) scol *= (std::fabs(pRay.dir*sp.N)/s.pdf);
		else continue;
		//scol = p_mat->sample(state, hit, pwo, pRay.dir, s1, s2); //ya no pdf yet...assume lambertian
		if(scol.isBlack()) continue;
		pRay.tmin = 0.0005;
		pRay.tmax = -1.0;
		pRay.from = hit.P;
		throughput = scol;
		
		if( !(did_hit = scene->intersect(pRay, hit)) ) //hit background
		{
			if(background && use_bg) pathCol += throughput * (*background)(pRay, state, true);
			continue;
		}
		p_mat = hit.material;
		length = pRay.tmax;
		state.userdata = n_udat;
		matBSDFs = p_mat->getFlags();
		bool has_spec = matBSDFs & BSDF_SPECULAR;
		bool caustic = false;
		bool close = length < gatherDist;
		bool do_bounce = close || has_spec;
		// further bounces construct a path just as with path tracing:
		for(int depth=0; depth<gatherBounces && do_bounce; ++depth)
		{
			pwo = -pRay.dir;
			p_mat->initBSDF(state, hit, matBSDFs);
			//lcol = estimateOneDirect(state, scene, hit, pwo, scene->lights, trShad, sDepth, 4*depth+5, offs);
			if(matBSDFs & (BSDF_DIFFUSE | BSDF_GLOSSY))
			{
				if(close)
				{
					lcol = estimateOneDirect(state, hit, pwo, lights, 4*depth+5, offs);
					//lcol += estimatePhotons(state, hit, causticMap, pwo, nCausSearch, dsRadius);
					pathCol += lcol*throughput;
				}
				else if(caustic)
				{
					vector3d_t sf = hit.N;//FACE_FORWARD(hit.Ng, hit.N, pwo);
					const photon_t *nearest = radianceMap.findNearest(hit.P, sf, lookupRad);
					if(nearest) pathCol += throughput * nearest->color();
				}
			}
			
			s1 = scrHalton(4*depth+3, offs); //ourRandom();//
			s2 = scrHalton(4*depth+4, offs); //ourRandom();//;
			if(state.rayDivision > 1)
			{
				s1 = addMod1(s1, state.dc1);
				s2 = addMod1(s2, state.dc2);
			}
			sample_t sb(s1, s2, (close) ? BSDF_ALL : BSDF_ALL_SPECULAR | BSDF_FILTER);
			//scol = p_mat->sample(state, hit, pwo, pRay.dir, s1, s2); //ya no pdf yet...assume lambertian
			//if(scol.isBlack()) break;
			scol = p_mat->sample(state, hit, pwo, pRay.dir, sb);
			if( sb.pdf > 1.0e-6f) scol *= (std::fabs(pRay.dir*hit.N)/sb.pdf);
			else { did_hit=false; break; }
			pRay.tmin = 0.0005;
			pRay.tmax = -1.0;
			pRay.from = hit.P;
			throughput *= scol;
			did_hit = scene->intersect(pRay, hit);
			if(!did_hit) //hit background
			{
				if(background && use_bg) pathCol += throughput * (*background)(pRay, state, true);
				break;
//				std::cout <<"!";
			}
			p_mat = hit.material;
			length += pRay.tmax;
			caustic = (caustic || !depth) && (sb.sampledFlags & (BSDF_SPECULAR | BSDF_FILTER));
			close =  length < gatherDist;
			do_bounce = caustic || close;
		}
		if(did_hit)
		{
			matBSDFs = p_mat->getFlags();
			if(matBSDFs & (BSDF_DIFFUSE | BSDF_GLOSSY))
			{
				vector3d_t sf = FACE_FORWARD(hit.Ng, hit.N, -pRay.dir);//hit.N;
				const photon_t *nearest = radianceMap.findNearest(hit.P, sf, lookupRad);
				if(nearest) pathCol += throughput * nearest->color();
			}
		}
		state.userdata = first_udat;
	}
	return pathCol / (CFLOAT)nSampl;
}

void photonIntegrator_t::sampleIrrad(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, irradSample_t &ir) const
{
	ir.Rmin = -1.0;
	ir.P = sp.P;
	ir.N = sp.N; //!TODO: use unbumped normal!
	
	void *first_udat = state.userdata;
	unsigned char userdata[USER_DATA_SIZE+7];
	void *n_udat = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
	vector3d_t wi_0;
	
	int nSampl = nPaths;
	for(int i=0; i<nSampl; ++i)
	{
		color_t throughput( 1.0 );
		color_t pathCol(0.0);
		PFLOAT length=0;
		surfacePoint_t hit=sp;
		vector3d_t pwo; // = wo;
		ray_t pRay;
		BSDF_t matBSDFs;
		bool did_hit;
		//const material_t *p_mat = sp.material;
		unsigned int offs = nPaths * state.pixelSample + state.samplingOffs + i; // some redundancy here...
		color_t lcol, scol;
		// "zero'th" FG bounce:
		float s1 = RI_vdC(offs);
		float s2 = scrHalton(2, offs);
		//sample_t s(s1, s2, BSDF_DIFFUSE|BSDF_REFLECT|BSDF_TRANSMIT); // specular/glossy done via recursive raytracing
		//scol = p_mat->sample(state, hit, pwo, pRay.dir, s);
		//if(s.pdf > 1.0e-6f) scol *= (std::std::fabs(pRay.dir*sp.N)/s.pdf);
		//else continue;
		vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
		wi_0 = SampleCosHemisphere(N, sp.NU, sp.NV, s1, s2);
		pRay.dir = wi_0;
		
		//if(scol.isBlack()) continue;
		pRay.tmin = 0.0005;
		pRay.tmax = -1.0;
		pRay.from = hit.P;
		//throughput = scol;
		
		if( (did_hit = scene->intersect(pRay, hit)) )
		{
			if(ir.Rmin < 0.f) ir.Rmin = pRay.tmax;
			else ir.Rmin = std::min(ir.Rmin, pRay.tmax);
		}
		else //hit background
		{
			if(background && use_bg)
			{
				pathCol = throughput * (*background)(pRay, state, true);
				ir.col += pathCol;
				ir.w_r += pathCol.R * wi_0;
				ir.w_g += pathCol.G * wi_0;
				ir.w_b += pathCol.B * wi_0;
			}
			continue;
		}
		const material_t *p_mat = hit.material;
		length = pRay.tmax;
		state.userdata = n_udat;
		matBSDFs = p_mat->getFlags();
		bool has_spec = matBSDFs & BSDF_SPECULAR;
		bool caustic = false;
		bool close = length < gatherDist;
		bool do_bounce = close || has_spec;
		// further bounces construct a path just as with path tracing:
		for(int depth=0; depth<gatherBounces && do_bounce; ++depth)
		{
			pwo = -pRay.dir;
			p_mat->initBSDF(state, hit, matBSDFs);
			//lcol = estimateOneDirect(state, scene, hit, pwo, scene->lights, trShad, sDepth, 4*depth+5, offs);
			if(matBSDFs & (BSDF_DIFFUSE | BSDF_GLOSSY))
			{
				if(close)
				{
					lcol = estimateOneDirect(state, hit, pwo, lights, 4*depth+5, offs);
					//lcol += estimatePhotons(state, hit, causticMap, pwo, nCausSearch, dsRadius);
					pathCol += lcol*throughput;
				}
				else if(caustic)
				{
					vector3d_t sf = FACE_FORWARD(hit.Ng, hit.N, pwo);//hit.N;
					const photon_t *nearest = radianceMap.findNearest(hit.P, sf, lookupRad);
					if(nearest) pathCol += throughput * nearest->color();
				}
			}
			
			s1 = scrHalton(4*depth+3, offs); //ourRandom();//
			s2 = scrHalton(4*depth+4, offs); //ourRandom();//;
			sample_t sb(s1, s2, (close) ? BSDF_ALL : BSDF_ALL_SPECULAR | BSDF_FILTER);
			//scol = p_mat->sample(state, hit, pwo, pRay.dir, s1, s2); //ya no pdf yet...assume lambertian
			//if(scol.isBlack()) break;
			scol = p_mat->sample(state, hit, pwo, pRay.dir, sb);
			if( sb.pdf > 1.0e-6f) scol *= (std::fabs(pRay.dir*hit.N)/sb.pdf);
			else { did_hit=false; break; }
			pRay.tmin = 0.0005;
			pRay.tmax = -1.0;
			pRay.from = hit.P;
			throughput *= scol;
			did_hit = scene->intersect(pRay, hit);
			if(!did_hit) //hit background
			{
				if(background && use_bg) pathCol += throughput * (*background)(pRay, state, true);
				break;
//				std::cout <<"!";
			}
			p_mat = hit.material;
			length += pRay.tmax;
			caustic = (caustic || !depth) && (sb.sampledFlags & (BSDF_SPECULAR | BSDF_FILTER));
			close =  length < gatherDist;
			do_bounce = caustic || close;
		}
		if(did_hit)
		{
			matBSDFs = p_mat->getFlags();
			if(matBSDFs & (BSDF_DIFFUSE | BSDF_GLOSSY))
			{
				vector3d_t sf = FACE_FORWARD(hit.Ng, hit.N, -pRay.dir);//hit.N;
				const photon_t *nearest = radianceMap.findNearest(hit.P, sf, lookupRad);
				if(nearest) pathCol += throughput * nearest->color();
			}
		}
		ir.col += pathCol;
		ir.w_r += pathCol.R * wi_0;
		ir.w_g += pathCol.G * wi_0;
		ir.w_b += pathCol.B * wi_0;
		state.userdata = first_udat;
	}
	ir.col *= 1.f / (CFLOAT)nSampl;
	ir.w_r.normalize();
	ir.w_g.normalize();
	ir.w_b.normalize();
}

colorA_t photonIntegrator_t::integrate(renderState_t &state, diffRay_t &ray) const
{
	static int _nMax=0;
	static int calls=0;
	++calls;
	color_t col(0.0);
	CFLOAT alpha=0.0;
	surfacePoint_t sp;
	
	void *o_udat = state.userdata;
	bool oldIncludeLights = state.includeLights;
	if(scene->intersect(ray, sp))
	{
		unsigned char userdata[USER_DATA_SIZE+7];
		state.userdata = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
		if(state.raylevel == 0)
		{
			state.chromatic = true;
			state.includeLights = true;
		}
		BSDF_t bsdfs;
		vector3d_t N_nobump = sp.N;
		vector3d_t wo = -ray.dir;
		const material_t *material = sp.material;
		material->initBSDF(state, sp, bsdfs);
		col += material->emit(state, sp, wo);
		state.includeLights = false;
		spDifferentials_t spDiff(sp, ray);
		/* if(prepass)
		{
			if(ray.hasDifferentials)
			{
				spDifferentials_t spDiff(sp, ray);
				PFLOAT A_pix = spDiff.projectedPixelArea();
				if(calls < 10) std::cout << "A_pix: " << A_pix << std::endl;
				if( ! irCache.enoughSamples(sp, A_pix) )
				{
					irradSample_t irSample;
					sampleIrrad(state, sp, -ray.dir, irSample);
					irSample.Apix = A_pix;
					irCache.insert(irSample);
					if(calls < 10) std::cout << "Rmin: " << irSample.Rmin << std::endl;
					col += color_t(1.f, 0.2f, 0.f);
				}
			}
			return col;
		} */
		if(cacheIrrad)
		{
			if(ray.hasDifferentials)
			{
				PFLOAT A_pix = spDiff.projectedPixelArea();
				if(calls < 10) std::cout << "A_pix: " << A_pix << std::endl;
				irradSample_t irr;
				irr.N = N_nobump;
				//bool do_debug = (state.pixelNumber == 245223 || state.pixelNumber == 246023 || state.pixelNumber == 246823);
				//if(do_debug) std::cout << "\nCosine:" << sp.N*wo;
				std::swap(sp.N, N_nobump);
				if( irCache.gatherSamples(sp, A_pix, irr/* , do_debug */) )
				{
					//restore sp.N
					std::swap(sp.N, N_nobump);
					color_t cos_Nnb_w(1.f/std::max(0.05f, CFLOAT(N_nobump*irr.w_r)),
									  1.f/std::max(0.05f, CFLOAT(N_nobump*irr.w_g)),
									  1.f/std::max(0.05f, CFLOAT(N_nobump*irr.w_b)) );
					color_t cos_N_w(	std::max(0.05f, CFLOAT(sp.N*irr.w_r)),
										std::max(0.05f, CFLOAT(sp.N*irr.w_g)),
										std::max(0.05f, CFLOAT(sp.N*irr.w_b)) );
					if(calls < 10) std::cout << "irr.col: " << irr.col << std::endl;
					color_t mcol;
					mcol.R = (material->eval(state, sp, wo, irr.w_r, BSDF_DIFFUSE|BSDF_REFLECT|BSDF_TRANSMIT)).R;
					mcol.G = (material->eval(state, sp, wo, irr.w_r, BSDF_DIFFUSE|BSDF_REFLECT|BSDF_TRANSMIT)).G;
					mcol.B = (material->eval(state, sp, wo, irr.w_r, BSDF_DIFFUSE|BSDF_REFLECT|BSDF_TRANSMIT)).B;
					col += mcol * irr.col * cos_N_w * cos_Nnb_w;
				}
				// we're forced to do final gathering unless we can write to cache while rendering...
				else
				{
					//restore sp.N
					std::swap(sp.N, N_nobump);
					col += finalGathering(state, sp, wo);
				}
				/* else
				{
					irradSample_t irSample;
					sampleIrrad(state, sp, wo, irSample);
					irSample.Apix = A_pix;
					irCache.insert(irSample);
					if(calls < 10) std::cout << "Rmin: " << irSample.Rmin << std::endl;
					col += color_t(1.f, 0.f, 0.f);
				} */
			}
			else col += finalGathering(state, sp, wo);
			if( bsdfs & (BSDF_DIFFUSE | BSDF_GLOSSY) )
				col += estimateDirect_PH(state, sp, lights, scene, wo, trShad, sDepth);
		}
		else if(finalGather)
		{
			if(showMap)
			{
				vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
				const photon_t *nearest = radianceMap.findNearest(sp.P, N, lookupRad);
				if(nearest) col += nearest->color();
			}
			else
			{
				if( bsdfs & (BSDF_DIFFUSE | BSDF_GLOSSY) )
					col += estimateDirect_PH(state, sp, lights, scene, wo, trShad, sDepth);
				if(isnan(col.R) || isnan(col.G) || isnan(col.B)) std::cout << "NaN WARNING! (photonintegr, estimateDirect)\n";
				if( bsdfs & BSDF_DIFFUSE )
					col += finalGathering(state, sp, wo);
				if(isnan(col.R) || isnan(col.G) || isnan(col.B)) std::cout << "NaN WARNING! (photonintegr, finalGathering)\n";
			}
		}
		else
		{
	#if OLD_PMAP > 0
			std::vector<foundPhoton_t> gathered;
			PFLOAT radius = fSqrt(dsRadius); //actually the square radius...
	#else		
			foundPhoton_t *gathered = (foundPhoton_t *)alloca(nSearch * sizeof(foundPhoton_t));
			PFLOAT radius = dsRadius; //actually the square radius...
	#endif
			int nGathered=0;
			
	#if OLD_PMAP > 0
			if(diffuseMap.nPhotons() > 0) diffuseMap.gather(sp.P, sp.N, gathered, nSearch, radius);
			nGathered = gathered.size();
	#else
			if(diffuseMap.nPhotons() > 0) nGathered = diffuseMap.gather(sp.P, gathered, nSearch, radius);
	#endif
			color_t sum(0.0);
			if(nGathered > 0)
			{
	#if OLD_PMAP > 0
				radius = gathered.front().dis;
				radius *= radius;
	#endif			
				if(nGathered > _nMax)
				{
					_nMax = nGathered;
					std::cout << "maximum Photons: "<<_nMax<<", radius: "<<radius<<"\n";
					if(_nMax == 10) for(int j=0; j<nGathered; ++j) std::cout<<"col:"<<gathered[j].photon->color()<<"\n";
				}

				float scale = 1.f / ( float(diffuseMap.nPaths()) * radius * M_PI);
				for(int i=0; i<nGathered; ++i)
				{
					vector3d_t pdir = gathered[i].photon->direction();
					color_t surfCol = material->eval(state, sp, wo, pdir, BSDF_DIFFUSE);
					col += surfCol * scale * gathered[i].photon->color();// * std::std::fabs(sp.N*pdir); //< wrong!?
				}
	//			vector3d_t pdir = gathered[0].photon->direction();
	//			col += CFLOAT(nGathered) * scale * material->eval(state, sp, wo, pdir, BSDF_DIFFUSE)* gathered[0].photon->color();
			}
		}
		// add caustics
		if(bsdfs & (BSDF_DIFFUSE))// | BSDF_GLOSSY))
		{
			col += estimatePhotons(state, sp, causticMap, wo, nCausSearch, dsRadius);
		}
		state.raylevel++;
		if(state.raylevel <= rDepth)
		{
			// dispersive effects with recursive raytracing:
			if( (bsdfs & BSDF_DISPERSIVE) && state.chromatic )
			{
				state.includeLights = true; //debatable...
				int dsam = 8;
				int oldDivision = state.rayDivision;
				int oldOffset = state.rayOffset;
				float old_dc1 = state.dc1, old_dc2 = state.dc2;
				if(state.rayDivision > 1) dsam = std::max(1, dsam/oldDivision);
				state.rayDivision *= dsam;
				int branch = state.rayDivision*oldOffset;
				//int offs = gsam * state.pixelSample + state.samplingOffs;
				float d_1 = 1.f/(float)dsam;
				float ss1 = RI_S(state.pixelSample + state.samplingOffs);
				color_t dcol(0.f);
				vector3d_t wi;
				for(int ns=0; ns<dsam; ++ns)
				{
					state.wavelength = (ns + ss1)*d_1;
					state.dc1 = scrHalton(2*state.raylevel+1, branch + state.samplingOffs);
					state.dc2 = scrHalton(2*state.raylevel+2, branch + state.samplingOffs);
					if(oldDivision > 1)	state.wavelength = addMod1(state.wavelength, old_dc1);
					state.rayOffset = branch;
					++branch;
					sample_t s(0.5f, 0.5f, BSDF_REFLECT|BSDF_TRANSMIT|BSDF_DISPERSIVE);
					color_t mcol = material->sample(state, sp, wo, wi, s);
					if(s.pdf > 1.0e-6f && (s.sampledFlags & BSDF_DISPERSIVE))
					{
						mcol *= std::fabs(wi*sp.N)/s.pdf;
						state.chromatic = false;
						color_t wl_col;
						wl2rgb(state.wavelength, wl_col);
						diffRay_t refRay(sp.P, wi, 0.0005);
						dcol += (color_t)integrate(state, refRay) * mcol * wl_col;
						state.chromatic = true;
					}
				}
				col += dcol * d_1;
				state.rayDivision = oldDivision;
				state.rayOffset = oldOffset;
				state.dc1 = old_dc1; state.dc2 = old_dc2;
			}
			// glossy reflection with recursive raytracing:
			if( bsdfs & BSDF_GLOSSY )
			{
				state.includeLights = false;
				int gsam = 8;
				int oldDivision = state.rayDivision;
				int oldOffset = state.rayOffset;
				float old_dc1 = state.dc1, old_dc2 = state.dc2;
				if(state.rayDivision > 1) gsam = std::max(1, gsam/oldDivision);
				state.rayDivision *= gsam;
				int branch = state.rayDivision*oldOffset;
				int offs = gsam * state.pixelSample + state.samplingOffs;
				float d_1 = 1.f/(float)gsam;
				color_t gcol(0.f);
				vector3d_t wi;
				for(int ns=0; ns<gsam; ++ns)
				{
					state.dc1 = scrHalton(2*state.raylevel+1, branch + state.samplingOffs);
					state.dc2 = scrHalton(2*state.raylevel+2, branch + state.samplingOffs);
					state.rayOffset = branch;
					++branch;
					float s1 = RI_vdC(offs + ns);
					float s2 = scrHalton(2, offs + ns);
					if(oldDivision > 1) // create generalized halton sequence
					{
						s1 = addMod1(s1, old_dc1);
						s2 = addMod1(s2, old_dc2);
					}
					sample_t s(s1, s2, BSDF_REFLECT|BSDF_TRANSMIT|BSDF_GLOSSY);
					color_t mcol = material->sample(state, sp, wo, wi, s);
					if(s.pdf > 1.0e-6f && (s.sampledFlags & BSDF_GLOSSY))
					{
						mcol *= std::fabs(wi*sp.N)/s.pdf;
						diffRay_t refRay(sp.P, wi, 0.0005);
						gcol += (color_t)integrate(state, refRay) * mcol;
					}
				}
				col += gcol * d_1;
				//restore renderstate
				state.rayDivision = oldDivision;
				state.rayOffset = oldOffset;
				state.dc1 = old_dc1; state.dc2 = old_dc2;
			}
			
			//...perfect specular reflection/refraction with recursive raytracing...
			if(bsdfs & (BSDF_SPECULAR | BSDF_FILTER))
			{
				state.includeLights = true;
				bool reflect=false, refract=false;
				vector3d_t dir[2];
				color_t rcol[2], vcol;
				material->getSpecular(state, sp, wo, reflect, refract, &dir[0], &rcol[0]);
				if(reflect)
				{
					diffRay_t refRay(sp.P, dir[0], 0.0005);
					spDiff.reflectedRay(ray, refRay);
					color_t integ = color_t(integrate(state, refRay) );
					if((bsdfs&BSDF_VOLUMETRIC) && material->volumeTransmittance(state, sp, refRay, vcol))
					{	integ *= vcol;	}
					col += color_t(integ) * rcol[0];
				}
				if(refract)
				{
					diffRay_t refRay(sp.P, dir[1], 0.0005);
					spDiff.refractedRay(ray, refRay, material->getMatIOR());
					colorA_t integ = integrate(state, refRay);
					if((bsdfs&BSDF_VOLUMETRIC) && material->volumeTransmittance(state, sp, refRay, vcol))
					{	integ *= vcol;	}
					col += color_t(integ) * rcol[1];
					alpha = integ.A;
				}
			}
			
		}
		--state.raylevel;
		
		CFLOAT m_alpha = material->getAlpha(state, sp, wo);
		alpha = m_alpha + (1.f-m_alpha)*alpha;
	}
	else //nothing hit, return background
	{
		if(background)
		{
			col += (*background)(ray, state, false);
		}
	}
	state.userdata = o_udat;
	state.includeLights = oldIncludeLights;
	return colorA_t(col, alpha);
}

integrator_t* photonIntegrator_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	bool transpShad=false;
	bool finalGather=true;
	bool use_bg=true;
	bool show_map=false;
	bool cache_irrad=false;
	int shadowDepth=5;
	int raydepth=5;
	int numPhotons=100000;
	int search = 50;
	int caustic_mix = 50;
	int bounces = 5;
	int fgPaths = 32;
	int fgBounces = 2;
	float dsRad=0.1;
	float gatherDist=0.2;
	
	params.getParam("transpShad", transpShad);
	params.getParam("shadowDepth", shadowDepth);
	params.getParam("raydepth", raydepth);
	params.getParam("photons", numPhotons);
	params.getParam("diffuseRadius", dsRad);
	params.getParam("search", search);
	caustic_mix = search;
	params.getParam("caustic_mix", caustic_mix);
	params.getParam("bounces", bounces);
	params.getParam("use_background", use_bg);
	params.getParam("finalGather", finalGather);
	params.getParam("fg_samples", fgPaths);
	params.getParam("fg_bounces", fgBounces);
	gatherDist = /* 2.f* */dsRad;
	params.getParam("fg_min_pathlen", gatherDist);
	params.getParam("show_map", show_map);
	params.getParam("irradiance_cache", cache_irrad);
	
	photonIntegrator_t* ite = new photonIntegrator_t(numPhotons, transpShad, shadowDepth, dsRad);
	ite->rDepth = raydepth;
	ite->nSearch = search;
	ite->nCausSearch = caustic_mix;
	ite->finalGather = finalGather;
	ite->maxBounces = bounces;
	ite->nPaths = fgPaths;
	ite->gatherBounces = fgBounces;
	ite->use_bg = use_bg;
	ite->showMap = show_map;
	ite->gatherDist = gatherDist;
	ite->cacheIrrad = cache_irrad;
	return ite;
}

extern "C"
{

	YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
	{
		render.registerFactory("photonmapping", photonIntegrator_t::factory);
	}

}

__END_YAFRAY
