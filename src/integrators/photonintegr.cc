/****************************************************************************
 *      photonintegr.cc: integrator for photon mapping and final gather
 *      This is part of the yafaray package
 *      Copyright (C) 2006  Mathias Wein (Lynx)
 *		Copyright (C) 2009  Rodrigo Placencia (DarkTide)
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

#include <integrators/photonintegr.h>
#include <utilities/mcqmc.h>
#include <yafraycore/scr_halton.h>

#include <sstream>
#include <iomanip>

__BEGIN_YAFRAY

struct preGatherData_t
{
	preGatherData_t(photonMap_t *dm): diffuseMap(dm), fetched(0) {}
	photonMap_t *diffuseMap;
	
	std::vector<radData_t> rad_points;
	std::vector<photon_t> radianceVec;
	progressBar_t *pbar;
	volatile int fetched;
	yafthreads::mutex_t mutex;
};

class preGatherWorker_t: public yafthreads::thread_t
{
	public:
		preGatherWorker_t(preGatherData_t *dat, float dsRad, int search):
			gdata(dat), dsRadius_2(dsRad*dsRad), nSearch(search) {};
		virtual void body();
	protected:
		preGatherData_t *gdata;
		float dsRadius_2;
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

	float radius = 0.f;
	float iScale = 1.f / ((float)gdata->diffuseMap->nPaths() * M_PI);
	float scale = 0.f;
	
	while(start < total)
	{
		for(unsigned int n=start; n<end; ++n)
		{
			radius = dsRadius_2;//actually the square radius...
			int nGathered = gdata->diffuseMap->gather(gdata->rad_points[n].pos, gathered, nSearch, radius);
			
			vector3d_t rnorm = gdata->rad_points[n].normal;
			
			color_t sum(0.0);
			
			if(nGathered > 0)
			{
				scale = iScale / radius;
				
				for(int i=0; i<nGathered; ++i)
				{
					vector3d_t pdir = gathered[i].photon->direction();
					
					if( rnorm * pdir > 0.f ) sum += gdata->rad_points[n].refl * scale * gathered[i].photon->color();
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

photonIntegrator_t::photonIntegrator_t(unsigned int dPhotons, unsigned int cPhotons, bool transpShad, int shadowDepth, float dsRad, float cRad)
{
	bEnableCaustics = true;
	bEnableDiffuse = true;
	type = SURFACE;
	trShad = transpShad;
	finalGather = true;
	nDiffusePhotons = dPhotons;
	nCausPhotons = cPhotons;
	sDepth = shadowDepth;
	dsRadius = dsRad;
	causRadius = cRad;
	rDepth = 6;
	maxBounces = 5;
	integratorName = "PhotonMap";
	integratorShortName = "PM";
}

photonIntegrator_t::~photonIntegrator_t()
{
	// Empty
}

class causticWorker_t: public yafthreads::thread_t
{
	public:
		causticWorker_t(photonMap_t * causticmap, int thread_id, const scene_t *sc, unsigned int ncausphotons, const pdf1D_t *lightpowerd, int numclights, const std::string &integratorname, const std::vector<light_t *> &tmp_lights, int causdepth, progressBar_t *ppb, int pbstep, unsigned int &totalphotonsshot, int maxbounces):
			causticMap(causticmap), threadID(thread_id), scene(sc), nCausPhotons(ncausphotons), lightPowerD(lightpowerd), numCLights(numclights), integratorName(integratorname), tmplights(tmp_lights), causDepth(causdepth), pb(ppb), pbStep(pbstep), totalPhotonsShot(totalphotonsshot), maxBounces(maxbounces) {};
		virtual void body();
	protected:
		photonMap_t * causticMap;
		int threadID;
		const scene_t *scene;
		unsigned int nCausPhotons;
		const pdf1D_t *lightPowerD;
		int numCLights;
		const std::string &integratorName;
		const std::vector<light_t *> &tmplights;
		int causDepth;
		std::vector<photon_t> localCausticPhotons;
		progressBar_t *pb;
		int pbStep;
		unsigned int &totalPhotonsShot;
		int maxBounces;
};

void causticWorker_t::body()
{
	ray_t ray;
	float lightNumPdf, lightPdf, s1, s2, s3, s4, s5, s6, s7, sL;
	color_t pcol;

	//shoot photons
	bool done=false;
	unsigned int curr=0;
	
	surfacePoint_t sp;
	renderState_t state;
	unsigned char userdata[USER_DATA_SIZE+7];
	state.userdata = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
	state.cam = scene->getCamera();

	float fNumLights = (float)numCLights;
	unsigned int nCausPhotons_thread = 1 + ( (nCausPhotons - 1) / scene->getNumThreadsPhotons() );

	localCausticPhotons.clear();
	localCausticPhotons.reserve(nCausPhotons_thread);

	float invCaustPhotons = 1.f / (float)nCausPhotons;
	
	while(!done)
	{
		if(scene->getSignals() & Y_SIG_ABORT) { return; }

		//causticMap->mutex.lock();
		//Y_DEBUG << "Thread while First=" << this << " curr=" << curr << " curr_thread=" << curr_thread << " done=" << done << yendl;
		//causticMap->mutex.unlock();

		unsigned int haltoncurr = curr + nCausPhotons_thread * threadID;

		state.chromatic = true;
		state.wavelength = scrHalton(5,haltoncurr);

		s1 = RI_vdC(haltoncurr);
		s2 = scrHalton(2, haltoncurr);
		s3 = scrHalton(3, haltoncurr);
		s4 = scrHalton(4, haltoncurr);

		sL = float(haltoncurr) * invCaustPhotons;
		int lightNum = lightPowerD->DSample(sL, &lightNumPdf);
		
		if(lightNum >= numCLights)
		{
			causticMap->mutex.lock();
			Y_ERROR << integratorName << ": lightPDF sample error! " << sL << "/" << lightNum << yendl;
			causticMap->mutex.unlock();
			return;
		}

		pcol = tmplights[lightNum]->emitPhoton(s1, s2, s3, s4, ray, lightPdf);
		ray.tmin = scene->rayMinDist;
		ray.tmax = -1.0;
		pcol *= fNumLights*lightPdf/lightNumPdf; //remember that lightPdf is the inverse of th pdf, hence *=...
		if(pcol.isBlack())
		{
			++curr;
			done = (curr >= nCausPhotons_thread);
			continue;
		}
		int nBounces=0;
		bool causticPhoton = false;
		bool directPhoton = true;
		const material_t *material = nullptr;
		BSDF_t bsdfs;

		while( scene->intersect(ray, sp) )
		{
			if(std::isnan(pcol.R) || std::isnan(pcol.G) || std::isnan(pcol.B))
			{
				causticMap->mutex.lock();
				Y_WARNING << integratorName << ": NaN  on photon color for light" << lightNum + 1 << "." << yendl;
				causticMap->mutex.unlock();
				continue;
			}
			
			color_t transm(1.f);
			color_t vcol(0.f);
			const volumeHandler_t* vol = nullptr;
			
			if(material)
			{
				if((bsdfs&BSDF_VOLUMETRIC) && (vol=material->getVolumeHandler(sp.Ng * -ray.dir < 0)))
				{
					if(vol->transmittance(state, ray, vcol)) transm = vcol;
				}
			}
			
			vector3d_t wi = -ray.dir, wo;
			material = sp.material;
			material->initBSDF(state, sp, bsdfs);

			if(bsdfs & BSDF_DIFFUSE)
			{
				if(causticPhoton)
				{
					photon_t np(wi, sp.P, pcol);
					localCausticPhotons.push_back(np);
				}
			}
			
			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(nBounces == maxBounces) break;
			// scatter photon
			int d5 = 3*nBounces + 5;

			s5 = scrHalton(d5, haltoncurr);
			s6 = scrHalton(d5+1, haltoncurr);
			s7 = scrHalton(d5+2, haltoncurr);

			pSample_t sample(s5, s6, s7, BSDF_ALL, pcol, transm);

			bool scattered = material->scatterPhoton(state, sp, wi, wo, sample);
			if(!scattered) break; //photon was absorped.

			pcol = sample.color;

			causticPhoton = ((sample.sampledFlags & (BSDF_GLOSSY | BSDF_SPECULAR | BSDF_DISPERSIVE)) && directPhoton) ||
							((sample.sampledFlags & (BSDF_GLOSSY | BSDF_SPECULAR | BSDF_FILTER | BSDF_DISPERSIVE)) && causticPhoton);
			directPhoton = (sample.sampledFlags & BSDF_FILTER) && directPhoton;
			
			if(state.chromatic && (sample.sampledFlags & BSDF_DISPERSIVE))
			{
				state.chromatic=false;
				color_t wl_col;
				wl2rgb(state.wavelength, wl_col);
				pcol *= wl_col;
			}
			
			ray.from = sp.P;
			ray.dir = wo;
			ray.tmin = scene->rayMinDist;
			ray.tmax = -1.0;
			++nBounces;
		}
		++curr;
		if(curr % pbStep == 0)
		{
			pb->mutex.lock();
			pb->update();
			pb->mutex.unlock();
		}
		done = (curr >= nCausPhotons_thread);
	}
	causticMap->mutex.lock();
	causticMap->appendVector(localCausticPhotons, curr);
	totalPhotonsShot += curr;
	causticMap->mutex.unlock();
}


class diffuseWorker_t: public yafthreads::thread_t
{
	public:
		diffuseWorker_t(photonMap_t * diffusemap, int thread_id, const scene_t *sc, unsigned int ndiffusephotons, const pdf1D_t *lightpowerd, int numdlights, const std::string &integratorname, const std::vector<light_t *> &tmp_lights, progressBar_t *ppb, int pbstep, unsigned int &totalphotonsshot, int maxbounces, bool finalgather, preGatherData_t &ppgdat):
			diffuseMap(diffusemap), threadID(thread_id), scene(sc), nDiffusePhotons(ndiffusephotons), lightPowerD(lightpowerd), numDLights(numdlights), integratorName(integratorname), tmplights(tmp_lights), pb(ppb), pbStep(pbstep), totalPhotonsShot(totalphotonsshot), maxBounces(maxbounces), finalGather(finalgather), pgdat(ppgdat) {};
		virtual void body();
	protected:
		photonMap_t * diffuseMap;
		int threadID;
		const scene_t *scene;
		unsigned int nDiffusePhotons;
		const pdf1D_t *lightPowerD;
		int numDLights;
		const std::string &integratorName;
		const std::vector<light_t *> &tmplights;
		std::vector<photon_t> localDiffusePhotons;
		std::vector<radData_t> localRadPoints;
		progressBar_t *pb;
		int pbStep;
		unsigned int &totalPhotonsShot;
		int maxBounces;
		bool finalGather;
		preGatherData_t &pgdat;
};

void diffuseWorker_t::body()
{
	ray_t ray;
	float lightNumPdf, lightPdf, s1, s2, s3, s4, s5, s6, s7, sL;
	color_t pcol;

	//shoot photons
	bool done=false;
	unsigned int curr=0;
	
	surfacePoint_t sp;
	renderState_t state;
	unsigned char userdata[USER_DATA_SIZE+7];
	state.userdata = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
	state.cam = scene->getCamera();
	
	float fNumLights = (float)numDLights;

	unsigned int nDiffusePhotons_thread = 1 + ( (nDiffusePhotons - 1) / scene->getNumThreadsPhotons() );

	localDiffusePhotons.clear();
	localDiffusePhotons.reserve(nDiffusePhotons_thread);
	
	localRadPoints.clear();
	
	float invDiffPhotons = 1.f / (float)nDiffusePhotons;
	
	while(!done)
	{
		if(scene->getSignals() & Y_SIG_ABORT) { return; }
		
		unsigned int haltoncurr = curr + nDiffusePhotons_thread * threadID;

		s1 = RI_vdC(haltoncurr);
		s2 = scrHalton(2, haltoncurr);
		s3 = scrHalton(3, haltoncurr);
		s4 = scrHalton(4, haltoncurr);

		sL = float(haltoncurr) * invDiffPhotons;
		int lightNum = lightPowerD->DSample(sL, &lightNumPdf);
		if(lightNum >= numDLights)
		{
			diffuseMap->mutex.lock();
			Y_ERROR << integratorName << ": lightPDF sample error! " << sL << "/" << lightNum << yendl;
			diffuseMap->mutex.unlock();
			return;
		}

		pcol = tmplights[lightNum]->emitPhoton(s1, s2, s3, s4, ray, lightPdf);
		ray.tmin = scene->rayMinDist;
		ray.tmax = -1.0;
		pcol *= fNumLights*lightPdf/lightNumPdf; //remember that lightPdf is the inverse of th pdf, hence *=...
		
		if(pcol.isBlack())
		{
			++curr;
			done = (curr >= nDiffusePhotons_thread);
			continue;
		}

		int nBounces=0;
		bool causticPhoton = false;
		bool directPhoton = true;
		const material_t *material = nullptr;
		BSDF_t bsdfs;

		while( scene->intersect(ray, sp) )
		{
			if(std::isnan(pcol.R) || std::isnan(pcol.G) || std::isnan(pcol.B))
			{
				diffuseMap->mutex.lock();
				Y_WARNING << integratorName << ": NaN  on photon color for light" << lightNum + 1 << "." << yendl;
				diffuseMap->mutex.unlock();
				continue;
			}
			
			color_t transm(1.f);
			color_t vcol(0.f);
			const volumeHandler_t* vol = nullptr;
			
			if(material)
			{
				if((bsdfs&BSDF_VOLUMETRIC) && (vol=material->getVolumeHandler(sp.Ng * -ray.dir < 0)))
				{
					if(vol->transmittance(state, ray, vcol)) transm = vcol;
				}
			}
			
			vector3d_t wi = -ray.dir, wo;
			material = sp.material;
			material->initBSDF(state, sp, bsdfs);
			
			if(bsdfs & (BSDF_DIFFUSE))
			{
				//deposit photon on surface
				if(!causticPhoton)
				{
					photon_t np(wi, sp.P, pcol);
					localDiffusePhotons.push_back(np);
				}
				// create entry for radiance photon:
				// don't forget to choose subset only, face normal forward; geometric vs. smooth normal?
				if(finalGather && ourRandom() < 0.125 && !causticPhoton )
				{
					vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wi);
					radData_t rd(sp.P, N);
					rd.refl = material->getReflectivity(state, sp, BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_REFLECT);
					rd.transm = material->getReflectivity(state, sp, BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_TRANSMIT);
					localRadPoints.push_back(rd);
				}
			}
			// need to break in the middle otherwise we scatter the photon and then discard it => redundant
			if(nBounces == maxBounces) break;
			// scatter photon
			int d5 = 3*nBounces + 5;

			s5 = scrHalton(d5, haltoncurr);
			s6 = scrHalton(d5+1, haltoncurr);
			s7 = scrHalton(d5+2, haltoncurr);
			
			pSample_t sample(s5, s6, s7, BSDF_ALL, pcol, transm);

			bool scattered = material->scatterPhoton(state, sp, wi, wo, sample);
			if(!scattered) break; //photon was absorped.

			pcol = sample.color;

			causticPhoton = ((sample.sampledFlags & (BSDF_GLOSSY | BSDF_SPECULAR | BSDF_DISPERSIVE)) && directPhoton) ||
							((sample.sampledFlags & (BSDF_GLOSSY | BSDF_SPECULAR | BSDF_FILTER | BSDF_DISPERSIVE)) && causticPhoton);
			directPhoton = (sample.sampledFlags & BSDF_FILTER) && directPhoton;

			ray.from = sp.P;
			ray.dir = wo;
			ray.tmin = scene->rayMinDist;
			ray.tmax = -1.0;
			++nBounces;
		}
		++curr;
		if(curr % pbStep == 0)
		{
			pb->mutex.lock();
			pb->update();
			pb->mutex.unlock();
		}
		done = (curr >= nDiffusePhotons_thread);
	}
	diffuseMap->mutex.lock();
	diffuseMap->appendVector(localDiffusePhotons, curr);
	totalPhotonsShot += curr;
	diffuseMap->mutex.unlock();
	
	pgdat.mutex.lock();
	pgdat.rad_points.insert(std::end(pgdat.rad_points), std::begin(localRadPoints), std::end(localRadPoints));
	pgdat.mutex.unlock();
}

class photonMapKdTreeWorker_t: public yafthreads::thread_t
{
	public:
		photonMapKdTreeWorker_t(photonMap_t * photonmap):
			photonMap(photonmap) {};
		virtual void body();
	protected:
		photonMap_t * photonMap;
};

void photonMapKdTreeWorker_t::body()
{
	photonMap->updateTree();
}

bool photonIntegrator_t::preprocess()
{
	lookupRad = 4*dsRadius*dsRadius;
		
	std::stringstream set;
	gTimer.addEvent("prepass");
	gTimer.start("prepass");

	Y_INFO << integratorName << ": Starting preprocess..." << yendl;

	set << "Photon Mapping  ";

	if(trShad)
	{
		set << "ShadowDepth=" << sDepth << "  ";
	}
	set << "RayDepth=" << rDepth << "  ";

	background = scene->getBackground();
	lights = scene->lights;
	std::vector<light_t*> tmplights;

	if(caustics_enabled())
	{
		set << "\nCaustic photons=" << nCausPhotons << " search=" << nCausSearch <<" radius=" << causRadius << " depth=" << causDepth << "  ";
	}

	if(diffuse_enabled())
	{
		set << "\nDiffuse photons=" << nDiffusePhotons << " search=" << nDiffuseSearch <<" radius=" << dsRadius << "  ";
	}

	if(finalGather)
	{
		set << " FG paths=" << nPaths << " bounces=" << gatherBounces << "  ";
	}
		
	if(photonMapProcessing == PHOTONS_LOAD)
	{
		if(caustics_enabled())
		{
			std::string filename = boost::filesystem::temp_directory_path().string();
			filename += "/yafaray_photonMap_caustics.tmp";
			Y_INFO << integratorName << ": Loading caustics photon map from: " << filename << ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << yendl;
			if(photonMapLoad(session.causticMap, filename)) Y_VERBOSE << integratorName << ": Caustics map loaded." << yendl;
			else photonMapProcessing = PHOTONS_GENERATE_AND_SAVE;
		}

		if(diffuse_enabled())
		{
			std::string filename = boost::filesystem::temp_directory_path().string();
			filename += "/yafaray_photonMap_diffuse.tmp";
			Y_INFO << integratorName << ": Loading diffuse photon map from: " << filename << ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!"  << yendl;
			if(photonMapLoad(session.diffuseMap, filename)) Y_VERBOSE << integratorName << ": Diffuse map loaded." << yendl;
			else photonMapProcessing = PHOTONS_GENERATE_AND_SAVE;
		}

		if(diffuse_enabled() && finalGather)
		{
			std::string filename = boost::filesystem::temp_directory_path().string();
			filename += "/yafaray_photonMap_fg_radiance.tmp";
			Y_INFO << integratorName << ": Loading FG radiance photon map from: " << filename << ". If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!"  << yendl;
			if(photonMapLoad(session.radianceMap, filename)) Y_VERBOSE << integratorName << ": FG radiance map loaded." << yendl;
			else photonMapProcessing = PHOTONS_GENERATE_AND_SAVE;
		}
		
		if(photonMapProcessing == PHOTONS_GENERATE_AND_SAVE)
		{
			Y_WARNING << integratorName << ": photon map loading failed, changing to Generate and Save mode." << yendl;
		}
	}

	if(photonMapProcessing == PHOTONS_REUSE)
	{
		if(caustics_enabled())
		{
			Y_INFO << integratorName << ": Reusing caustics photon map from memory. If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << yendl;
			if(session.causticMap->nPhotons() == 0)
			{
				photonMapProcessing = PHOTONS_GENERATE_ONLY;
			}
		}

		if(diffuse_enabled())
		{
			Y_INFO << integratorName << ": Reusing diffuse photon map from memory. If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << yendl;
			if(session.diffuseMap->nPhotons() == 0)
			{
				photonMapProcessing = PHOTONS_GENERATE_ONLY;
			}
		}

		if(diffuse_enabled() && finalGather)
		{
			Y_INFO << integratorName << ": Reusing FG radiance photon map from memory. If it does not match the scene you could have crashes and/or incorrect renders, USE WITH CARE!" << yendl;
			if(session.diffuseMap->nPhotons() == 0)
			{
				photonMapProcessing = PHOTONS_GENERATE_ONLY;
			}
		}
		
		if(photonMapProcessing == PHOTONS_GENERATE_ONLY)
		{
			Y_WARNING << integratorName << ": One of the photon maps in memory was empty, they cannot be reused: changing to Generate mode." << yendl;
		}
	}

	if(photonMapProcessing == PHOTONS_LOAD)
	{
		set << " (loading photon maps from file)";
	}
	else if(photonMapProcessing == PHOTONS_REUSE)
	{
		set << " (reusing photon maps from memory)";
	}
	else if(photonMapProcessing == PHOTONS_GENERATE_AND_SAVE) set << " (saving photon maps to file)";

	if(photonMapProcessing == PHOTONS_LOAD || photonMapProcessing == PHOTONS_REUSE)
	{
		gTimer.stop("prepass");
		Y_INFO << integratorName << ": Photonmap building time: " << std::fixed << std::setprecision(1) << gTimer.getTime("prepass") << "s" << yendl;

		set << " [" << std::fixed << std::setprecision(1) << gTimer.getTime("prepass") << "s" << "]";

		yafLog.appendRenderSettings(set.str());
		
		for (std::string line; std::getline(set, line, '\n');) Y_VERBOSE << line << yendl;
		
		return true;
	}

	session.diffuseMap->clear();
	session.causticMap->clear();

	ray_t ray;
	float lightNumPdf, lightPdf;
	int numCLights = 0;
	int numDLights = 0;
	float fNumLights = 0.f;
	float *energies = nullptr;
	color_t pcol;

	//shoot photons
	unsigned int curr=0;
	// for radiance map:
	preGatherData_t pgdat(session.diffuseMap);
	
	surfacePoint_t sp;
	renderState_t state;
	unsigned char userdata[USER_DATA_SIZE+7];
	state.userdata = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
	state.cam = scene->getCamera();
	progressBar_t *pb;
	int pbStep;
	if(intpb) pb = intpb;
	else pb = new ConsoleProgressBar_t(80);

	tmplights.clear();

	for(int i=0;i<(int)lights.size();++i)
	{
		if(lights[i]->shootsDiffuseP())
		{
			numDLights++;
			tmplights.push_back(lights[i]);
		}
	}
	
	if(numDLights == 0)
	{
		Y_WARNING << integratorName << ": No lights found that can shoot diffuse photons, disabling Diffuse photon processing" << yendl;
		enableDiffuse(false);
	}
	
	if( diffuse_enabled() )
	{
		fNumLights = (float)numDLights;
		energies = new float[numDLights];

		for(int i=0;i<numDLights;++i) energies[i] = tmplights[i]->totalEnergy().energy();

		lightPowerD = new pdf1D_t(energies, numDLights);
		
		Y_VERBOSE << integratorName << ": Light(s) photon color testing for diffuse map:" << yendl;
		for(int i=0;i<numDLights;++i)
		{
			pcol = tmplights[i]->emitPhoton(.5, .5, .5, .5, ray, lightPdf);
			lightNumPdf = lightPowerD->func[i] * lightPowerD->invIntegral;
			pcol *= fNumLights*lightPdf/lightNumPdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			Y_VERBOSE << integratorName << ": Light [" << i+1 << "] Photon col:" << pcol << " | lnpdf: " << lightNumPdf << yendl;
		}
		
		delete[] energies;
		
		//shoot photons
		curr=0;
		
		Y_INFO << integratorName << ": Building diffuse photon map..." << yendl;
		
		pb->init(128);
		pbStep = std::max(1U, nDiffusePhotons / 128);
		pb->setTag("Building diffuse photon map...");
		//Pregather diffuse photons

#ifdef USING_THREADS
		int nThreads = scene->getNumThreadsPhotons();

		nDiffusePhotons = (nDiffusePhotons / nThreads) * nThreads; //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads)
		
		Y_PARAMS << integratorName << ": Shooting "<<nDiffusePhotons<<" photons across " << nThreads << " threads (" << (nDiffusePhotons / nThreads) << " photons/thread)"<< yendl;
		
		if(nThreads > 1)
		{
			std::vector<diffuseWorker_t *> workers;
			for(int i=0; i<nThreads; ++i) workers.push_back(new diffuseWorker_t(session.diffuseMap, i, scene, nDiffusePhotons, lightPowerD, numDLights, integratorName, tmplights, pb, pbStep, curr, maxBounces, finalGather, pgdat));
			for(int i=0;i<nThreads;++i) workers[i]->run();
			for(int i=0;i<nThreads;++i)	workers[i]->wait();
			for(int i=0;i<nThreads;++i)	delete workers[i];
		}
		else
#endif
		{
		bool done=false;

		float invDiffPhotons = 1.f / (float)nDiffusePhotons;
		float s1, s2, s3, s4, s5, s6, s7, sL;
		
		while(!done)
		{
			if(scene->getSignals() & Y_SIG_ABORT) {  pb->done(); if(!intpb) delete pb; return false; }

			s1 = RI_vdC(curr);
			s2 = scrHalton(2, curr);
			s3 = scrHalton(3, curr);
			s4 = scrHalton(4, curr);

			sL = float(curr) * invDiffPhotons;
			int lightNum = lightPowerD->DSample(sL, &lightNumPdf);
			if(lightNum >= numDLights)
			{
				Y_ERROR << integratorName << ": lightPDF sample error! " << sL << "/" << lightNum << "... stopping now." << yendl;
				delete lightPowerD;
				return false;
			}

			pcol = tmplights[lightNum]->emitPhoton(s1, s2, s3, s4, ray, lightPdf);
			ray.tmin = scene->rayMinDist;
			ray.tmax = -1.0;
			pcol *= fNumLights*lightPdf/lightNumPdf; //remember that lightPdf is the inverse of th pdf, hence *=...
			
			if(pcol.isBlack())
			{
				++curr;
				done = (curr >= nDiffusePhotons);
				continue;
			}

			int nBounces=0;
			bool causticPhoton = false;
			bool directPhoton = true;
			const material_t *material = nullptr;
			BSDF_t bsdfs;

			while( scene->intersect(ray, sp) )
			{
				if(std::isnan(pcol.R) || std::isnan(pcol.G) || std::isnan(pcol.B))
				{
					Y_WARNING << integratorName << ": NaN  on photon color for light" << lightNum + 1 << "." << yendl;
					continue;
				}
				
				color_t transm(1.f);
				color_t vcol(0.f);
				const volumeHandler_t* vol = nullptr;
				
				if(material)
				{
					if((bsdfs&BSDF_VOLUMETRIC) && (vol=material->getVolumeHandler(sp.Ng * -ray.dir < 0)))
					{
						if(vol->transmittance(state, ray, vcol)) transm = vcol;
					}
				}
				
				vector3d_t wi = -ray.dir, wo;
				material = sp.material;
				material->initBSDF(state, sp, bsdfs);
				
				if(bsdfs & (BSDF_DIFFUSE))
				{
					//deposit photon on surface
					if(!causticPhoton)
					{
						photon_t np(wi, sp.P, pcol);
						session.diffuseMap->pushPhoton(np);
						session.diffuseMap->setNumPaths(curr);
					}
					// create entry for radiance photon:
					// don't forget to choose subset only, face normal forward; geometric vs. smooth normal?
					if(finalGather && ourRandom() < 0.125 && !causticPhoton )
					{
						vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wi);
						radData_t rd(sp.P, N);
						rd.refl = material->getReflectivity(state, sp, BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_REFLECT);
						rd.transm = material->getReflectivity(state, sp, BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_TRANSMIT);
						pgdat.rad_points.push_back(rd);
					}
				}
				// need to break in the middle otherwise we scatter the photon and then discard it => redundant
				if(nBounces == maxBounces) break;
				// scatter photon
				int d5 = 3*nBounces + 5;

				s5 = scrHalton(d5, curr);
				s6 = scrHalton(d5+1, curr);
				s7 = scrHalton(d5+2, curr);
				
				pSample_t sample(s5, s6, s7, BSDF_ALL, pcol, transm);

				bool scattered = material->scatterPhoton(state, sp, wi, wo, sample);
				if(!scattered) break; //photon was absorped.

				pcol = sample.color;

				causticPhoton = ((sample.sampledFlags & (BSDF_GLOSSY | BSDF_SPECULAR | BSDF_DISPERSIVE)) && directPhoton) ||
								((sample.sampledFlags & (BSDF_GLOSSY | BSDF_SPECULAR | BSDF_FILTER | BSDF_DISPERSIVE)) && causticPhoton);
				directPhoton = (sample.sampledFlags & BSDF_FILTER) && directPhoton;

				ray.from = sp.P;
				ray.dir = wo;
				ray.tmin = scene->rayMinDist;
				ray.tmax = -1.0;
				++nBounces;
			}
			++curr;
			if(curr % pbStep == 0) pb->update();
			done = (curr >= nDiffusePhotons);
		}
		}

		pb->done();
		pb->setTag("Diffuse photon map built.");
		Y_VERBOSE << integratorName << ": Diffuse photon map built." << yendl;
		Y_INFO << integratorName << ": Shot "<<curr<<" photons from " << numDLights << " light(s)" << yendl;

		delete lightPowerD;

		tmplights.clear();
		
		if(session.diffuseMap->nPhotons() < 50)
		{
			Y_ERROR << integratorName << ": Too few diffuse photons, stopping now." << yendl;
			return false;
		}

		Y_VERBOSE << integratorName << ": Stored diffuse photons: " << session.diffuseMap->nPhotons() << yendl;
	}
	else
	{
		Y_INFO << integratorName << ": Diffuse photon mapping disabled, skipping..." << yendl;
	}
	
#ifdef USING_THREADS
	photonMapKdTreeWorker_t * diffuseMapBuildKdTree = nullptr;

	if( diffuse_enabled() && session.diffuseMap->nPhotons() > 0 && scene->getNumThreadsPhotons() >= 2)
	{
		Y_INFO << integratorName << ": Building diffuse photons kd-tree:" << yendl;
		pb->setTag("Building diffuse photons kd-tree...");

		diffuseMapBuildKdTree = new photonMapKdTreeWorker_t(session.diffuseMap);

		diffuseMapBuildKdTree->run();
	}
	else
#endif
	if( diffuse_enabled() && session.diffuseMap->nPhotons() > 0)
	{
		Y_INFO << integratorName << ": Building diffuse photons kd-tree:" << yendl;
		pb->setTag("Building diffuse photons kd-tree...");
		session.diffuseMap->updateTree();
		Y_VERBOSE << integratorName << ": Done." << yendl;
	}

	for(int i=0;i<(int)lights.size();++i)
	{
		if(lights[i]->shootsCausticP())
		{
			numCLights++;
			tmplights.push_back(lights[i]);
		}
	}

	if(numCLights == 0)
	{
		Y_WARNING << integratorName << ": No lights found that can shoot caustic photons, disabling Caustic photon processing" << yendl;
		enableCaustics(false);
	}

	if( caustics_enabled() )
	{
		curr=0;

		fNumLights = (float)numCLights;
		energies = new float[numCLights];

		for(int i=0;i<numCLights;++i) energies[i] = tmplights[i]->totalEnergy().energy();

		lightPowerD = new pdf1D_t(energies, numCLights);
		
		Y_VERBOSE << integratorName << ": Light(s) photon color testing for caustics map:" << yendl;
		for(int i=0;i<numCLights;++i)
		{
			pcol = tmplights[i]->emitPhoton(.5, .5, .5, .5, ray, lightPdf);
			lightNumPdf = lightPowerD->func[i] * lightPowerD->invIntegral;
			pcol *= fNumLights*lightPdf/lightNumPdf; //remember that lightPdf is the inverse of the pdf, hence *=...
			Y_VERBOSE << integratorName << ": Light [" << i+1 << "] Photon col:" << pcol << " | lnpdf: " << lightNumPdf << yendl;
		}
		
		delete[] energies;

		Y_INFO << integratorName << ": Building caustics photon map..." << yendl;
		pb->init(128);
		pbStep = std::max(1U, nCausPhotons / 128);
		pb->setTag("Building caustics photon map...");
		//Pregather caustic photons


#ifdef USING_THREADS
		int nThreads = scene->getNumThreadsPhotons();

		nCausPhotons = (nCausPhotons / nThreads) * nThreads; //rounding the number of diffuse photons so it's a number divisible by the number of threads (distribute uniformly among the threads)
		
		Y_PARAMS << integratorName << ": Shooting "<<nCausPhotons<<" photons across " << nThreads << " threads (" << (nCausPhotons / nThreads) << " photons/thread)"<< yendl;

		if(nThreads > 1)
		{
			std::vector<causticWorker_t *> workers;
			for(int i=0; i<nThreads; ++i) workers.push_back(new causticWorker_t(session.causticMap, i, scene, nCausPhotons, lightPowerD, numCLights, integratorName, tmplights, causDepth, pb, pbStep, curr, maxBounces));
			for(int i=0;i<nThreads;++i) workers[i]->run();
			for(int i=0;i<nThreads;++i)	workers[i]->wait();
			for(int i=0;i<nThreads;++i)	delete workers[i];
		}
		else		
#endif
		{
		bool done=false;
		
		float invCaustPhotons = 1.f / (float)nCausPhotons;
		float s1, s2, s3, s4, s5, s6, s7, sL;
		
		while(!done)
		{
			if(scene->getSignals() & Y_SIG_ABORT) { pb->done(); if(!intpb) delete pb; return false; }
			state.chromatic = true;
			state.wavelength = scrHalton(5,curr);

			s1 = RI_vdC(curr);
			s2 = scrHalton(2, curr);
			s3 = scrHalton(3, curr);
			s4 = scrHalton(4, curr);

			sL = float(curr) * invCaustPhotons;
			int lightNum = lightPowerD->DSample(sL, &lightNumPdf);
			
			if(lightNum >= numCLights)
			{
				Y_ERROR << integratorName << ": lightPDF sample error! "<<sL<<"/"<<lightNum<<"... stopping now." << yendl;
				delete lightPowerD;
				return false;
			}

			pcol = tmplights[lightNum]->emitPhoton(s1, s2, s3, s4, ray, lightPdf);
			ray.tmin = scene->rayMinDist;
			ray.tmax = -1.0;
			pcol *= fNumLights*lightPdf/lightNumPdf; //remember that lightPdf is the inverse of th pdf, hence *=...
			if(pcol.isBlack())
			{
				++curr;
				done = (curr >= nCausPhotons);
				continue;
			}
			int nBounces=0;
			bool causticPhoton = false;
			bool directPhoton = true;
			const material_t *material = nullptr;
			BSDF_t bsdfs;

			while( scene->intersect(ray, sp) )
			{
				if(std::isnan(pcol.R) || std::isnan(pcol.G) || std::isnan(pcol.B))
				{
					Y_WARNING << integratorName << ": NaN  on photon color for light" << lightNum + 1 << "." << yendl;
					continue;
				}
				
				color_t transm(1.f);
				color_t vcol(0.f);
				const volumeHandler_t* vol = nullptr;
				
				if(material)
				{
					if((bsdfs&BSDF_VOLUMETRIC) && (vol=material->getVolumeHandler(sp.Ng * -ray.dir < 0)))
					{
						if(vol->transmittance(state, ray, vcol)) transm = vcol;
					}
				}
				
				vector3d_t wi = -ray.dir, wo;
				material = sp.material;
				material->initBSDF(state, sp, bsdfs);

				if(bsdfs & BSDF_DIFFUSE)
				{
					if(causticPhoton)
					{
						photon_t np(wi, sp.P, pcol);
						session.causticMap->pushPhoton(np);
						session.causticMap->setNumPaths(curr);
					}
				}
				
				// need to break in the middle otherwise we scatter the photon and then discard it => redundant
				if(nBounces == maxBounces) break;
				// scatter photon
				int d5 = 3*nBounces + 5;

				s5 = scrHalton(d5, curr);
				s6 = scrHalton(d5+1, curr);
				s7 = scrHalton(d5+2, curr);

				pSample_t sample(s5, s6, s7, BSDF_ALL, pcol, transm);

				bool scattered = material->scatterPhoton(state, sp, wi, wo, sample);
				if(!scattered) break; //photon was absorped.

				pcol = sample.color;

				causticPhoton = ((sample.sampledFlags & (BSDF_GLOSSY | BSDF_SPECULAR | BSDF_DISPERSIVE)) && directPhoton) ||
								((sample.sampledFlags & (BSDF_GLOSSY | BSDF_SPECULAR | BSDF_FILTER | BSDF_DISPERSIVE)) && causticPhoton);
				directPhoton = (sample.sampledFlags & BSDF_FILTER) && directPhoton;
				
				if(state.chromatic && (sample.sampledFlags & BSDF_DISPERSIVE))
				{
					state.chromatic=false;
					color_t wl_col;
					wl2rgb(state.wavelength, wl_col);
					pcol *= wl_col;
				}
				
				ray.from = sp.P;
				ray.dir = wo;
				ray.tmin = scene->rayMinDist;
				ray.tmax = -1.0;
				++nBounces;
			}
			++curr;
			if(curr % pbStep == 0) pb->update();
			done = (curr >= nCausPhotons);
		}
		}

		pb->done();
		pb->setTag("Caustics photon map built.");
		delete lightPowerD;
		
		Y_INFO << integratorName << ": Shot "<<curr<<" caustic photons from " << numCLights <<" light(s)." << yendl;
		Y_VERBOSE << integratorName << ": Stored caustic photons: " << session.causticMap->nPhotons() << yendl;
	}
	else
	{
		Y_INFO << integratorName << ": Caustics photon mapping disabled, skipping..." << yendl;
	}
		
	tmplights.clear();

	if(!intpb) delete pb;


#ifdef USING_THREADS
	photonMapKdTreeWorker_t * causticMapBuildKdTree = nullptr;
	
	if(caustics_enabled() && session.causticMap->nPhotons() > 0 && scene->getNumThreadsPhotons() >= 2)
	{
		Y_INFO << integratorName << ": Building caustic photons kd-tree:" << yendl;
		pb->setTag("Building caustic photons kd-tree...");

		causticMapBuildKdTree = new photonMapKdTreeWorker_t(session.causticMap);

		causticMapBuildKdTree->run();
	}
	else
#endif
	{
		if( caustics_enabled() && session.causticMap->nPhotons() > 0)
		{
			Y_INFO << integratorName << ": Building caustic photons kd-tree:" << yendl;
			pb->setTag("Building caustic photons kd-tree...");
			session.causticMap->updateTree();
			Y_VERBOSE << integratorName << ": Done." << yendl;
		}
	}

#ifdef USING_THREADS
	if( diffuse_enabled() && session.diffuseMap->nPhotons() > 0 && scene->getNumThreadsPhotons() >= 2)
	{
		diffuseMapBuildKdTree->wait();

		delete diffuseMapBuildKdTree;

		Y_VERBOSE << integratorName << ": Diffuse photon map: done." << yendl;
	}
#endif

	if(diffuse_enabled() && finalGather) //create radiance map:
	{
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
		if(intpb) pgdat.pbar = intpb;
		else pgdat.pbar = new ConsoleProgressBar_t(80);
		pgdat.pbar->init(pgdat.rad_points.size());
		pgdat.pbar->setTag("Pregathering radiance data for final gathering...");
		std::vector<preGatherWorker_t *> workers;
		for(int i=0; i<nThreads; ++i) workers.push_back(new preGatherWorker_t(&pgdat, dsRadius, nDiffuseSearch));
		
		for(int i=0;i<nThreads;++i) workers[i]->run();
		for(int i=0;i<nThreads;++i)	workers[i]->wait();
		for(int i=0;i<nThreads;++i)	delete workers[i];
		
		session.radianceMap->swapVector(pgdat.radianceVec);
		pgdat.pbar->done();
		pgdat.pbar->setTag("Pregathering radiance data done...");
		if(!intpb) delete pgdat.pbar;
#else
		if(session.radianceMap->nPhotons() != 0)
		{
			Y_WARNING << integratorName << ": radianceMap not empty!" << yendl;
			session.radianceMap->clear();
		}
		
		Y_INFO << integratorName << ": Creating radiance map..." << yendl;
		progressBar_t *pbar;
		if(intpb) pbar = intpb;
		else pbar = new ConsoleProgressBar_t(80);
		pbar->init(pgdat.rad_points.size());
		foundPhoton_t *gathered = (foundPhoton_t *)malloc(nDifuseSearch * sizeof(foundPhoton_t));
		PFLOAT dsRadius_2 = dsRadius*dsRadius;
		for(unsigned int n=0; n< pgdat.rad_points.size(); ++n)
		{
			PFLOAT radius = dsRadius_2; //actually the square radius...
			int nGathered = session.diffuseMap->gather(pgdat.rad_points[n].pos, gathered, nDifuseSearch, radius);
			color_t sum(0.0);
			if(nGathered > 0)
			{
				color_t surfCol = pgdat.rad_points[n].refl;
				vector3d_t rnorm = pgdat.rad_points[n].normal;
				float scale = 1.f / ( float(session.diffuseMap->nPaths()) * radius * M_PI);
				
				if(std::isnan(scale))
				{
					Y_WARNING << integratorName << ": NaN on (scale)" << yendl;
					break;
				}
				
				for(int i=0; i<nGathered; ++i)
				{
					vector3d_t pdir = gathered[i].photon->direction();
					
					if( rnorm * pdir > 0.f ) sum += surfCol * scale * gathered[i].photon->color();
					else sum += pgdat.rad_points[n].transm * scale * gathered[i].photon->color();
				}
			}
			photon_t radP(pgdat.rad_points[n].normal, pgdat.rad_points[n].pos, sum);
			session.radianceMap->pushPhoton(radP);
			if(n && !(n&7)) pbar->update(8);
		}
		pbar->done();
		if(!pbar) delete pbar;
		free(gathered);
#endif
		Y_VERBOSE << integratorName << ": Radiance tree built... Updating the tree..." << yendl;
		session.radianceMap->updateTree();
		Y_VERBOSE << integratorName << ": Done." << yendl;
	}

#ifdef USING_THREADS
	if(caustics_enabled() && session.causticMap->nPhotons() > 0 && scene->getNumThreadsPhotons() >= 2)
	{
		causticMapBuildKdTree->wait();

		delete causticMapBuildKdTree;

		Y_VERBOSE << integratorName << ": Caustic photon map: done." << yendl;
	}
#endif

	if(photonMapProcessing == PHOTONS_GENERATE_AND_SAVE)
	{
		if( diffuse_enabled() && session.diffuseMap->nPhotons() > 0)
		{
			std::string filename = boost::filesystem::temp_directory_path().string();
			filename += "/yafaray_photonMap_diffuse.tmp";
			Y_INFO << integratorName << ": Saving diffuse photon map to: " << filename << yendl;
			if(photonMapSave(session.diffuseMap, filename)) Y_VERBOSE << integratorName << ": Diffuse map saved." << yendl;
		}

		if( caustics_enabled() && session.causticMap->nPhotons() > 0)
		{
			std::string filename = boost::filesystem::temp_directory_path().string();
			filename += "/yafaray_photonMap_caustics.tmp";
			Y_INFO << integratorName << ": Saving caustics photon map to: " << filename << yendl;
			if(photonMapSave(session.causticMap, filename)) Y_VERBOSE << integratorName << ": Caustics map saved." << yendl;
		}

		if(diffuse_enabled() && finalGather && session.radianceMap->nPhotons() > 0)
		{
			std::string filename = boost::filesystem::temp_directory_path().string();
			filename += "/yafaray_photonMap_fg_radiance.tmp";
			Y_INFO << integratorName << ": Saving FG radiance photon map to: " << filename << yendl;
			if(photonMapSave(session.radianceMap, filename)) Y_VERBOSE << integratorName << ": FG radiance map saved." << yendl;
		}
	}

	gTimer.stop("prepass");
	Y_INFO << integratorName << ": Photonmap building time: " << std::fixed << std::setprecision(1) << gTimer.getTime("prepass") << "s" << " (" << scene->getNumThreadsPhotons() << " thread(s))" << yendl;

	set << "| photon maps: " << std::fixed << std::setprecision(1) << gTimer.getTime("prepass") << "s" << " [" << scene->getNumThreadsPhotons() << " thread(s)]";

	yafLog.appendRenderSettings(set.str());
	
	for (std::string line; std::getline(set, line, '\n');) Y_VERBOSE << line << yendl;

	return true;
}

// final gathering: this is basically a full path tracer only that it uses the radiance map only
// at the path end. I.e. paths longer than 1 are only generated to overcome lack of local radiance detail.
// precondition: initBSDF of current spot has been called!
color_t photonIntegrator_t::finalGathering(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &wo, colorPasses_t &colorPasses) const
{
	color_t pathCol(0.0);
	void *first_udat = state.userdata;
	unsigned char userdata[USER_DATA_SIZE+7];
	void *n_udat = (void *)( &userdata[7] - ( ((size_t)&userdata[7])&7 ) ); // pad userdata to 8 bytes
	const volumeHandler_t *vol;
	color_t vcol(0.f);
	float W = 0.f;

	colorPasses_t tmpColorPasses(scene->getRenderPasses());
	
	int nSampl = (int) ceilf(std::max(1, nPaths/state.rayDivision)*AA_indirect_sample_multiplier);
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

		sample_t s(s1, s2, BSDF_DIFFUSE|BSDF_REFLECT|BSDF_TRANSMIT); // glossy/dispersion/specular done via recursive raytracing
		scol = p_mat->sample(state, hit, pwo, pRay.dir, s, W);

		scol *= W;
		if(scol.isBlack()) continue;

		pRay.tmin = scene->rayMinDist;
		pRay.tmax = -1.0;
		pRay.from = hit.P;
		throughput = scol;
		
		if( !(did_hit = scene->intersect(pRay, hit)) ) continue; //hit background
		
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
			int d4 = 4*depth;
			pwo = -pRay.dir;
			p_mat->initBSDF(state, hit, matBSDFs);
			
			if((matBSDFs & BSDF_VOLUMETRIC) && (vol=p_mat->getVolumeHandler(hit.N * pwo < 0)))
			{
				if(vol->transmittance(state, pRay, vcol)) throughput *= vcol;
			}
	
			if(matBSDFs & (BSDF_DIFFUSE))
			{
				if(close)
				{
					lcol = estimateOneDirectLight(state, hit, pwo, offs, tmpColorPasses);
				}
				else if(caustic)
				{
					vector3d_t sf = FACE_FORWARD(hit.Ng, hit.N, pwo);
					const photon_t *nearest = session.radianceMap->findNearest(hit.P, sf, lookupRad);
					if(nearest) lcol = nearest->color();
				}
				
				if(close || caustic)
				{
					if(matBSDFs & BSDF_EMIT) lcol += p_mat->emit(state, hit, pwo);
					pathCol += lcol*throughput;
				}
			}
			
			s1 = scrHalton(d4+3, offs);
			s2 = scrHalton(d4+4, offs);

			if(state.rayDivision > 1)
			{
				s1 = addMod1(s1, state.dc1);
				s2 = addMod1(s2, state.dc2);
			}
			
			sample_t sb(s1, s2, (close) ? BSDF_ALL : BSDF_ALL_SPECULAR | BSDF_FILTER);
			scol = p_mat->sample(state, hit, pwo, pRay.dir, sb, W);
			
			if( sb.pdf <= 1.0e-6f)
			{
				did_hit=false;
				break;
			}

			scol *= W;

			pRay.tmin = scene->rayMinDist;
			pRay.tmax = -1.0;
			pRay.from = hit.P;
			throughput *= scol;
			did_hit = scene->intersect(pRay, hit);
			
			if(!did_hit) //hit background
			{
				 if(caustic && background && background->hasIBL() && background->shootsCaustic())
				 {
					pathCol += throughput * (*background)(pRay, state);
				 }
				 break;
			}
			
			p_mat = hit.material;
			length += pRay.tmax;
			caustic = (caustic || !depth) && (sb.sampledFlags & (BSDF_SPECULAR | BSDF_FILTER));
			close =  length < gatherDist;
			do_bounce = caustic || close;
		}
		
		if(did_hit)
		{
			p_mat->initBSDF(state, hit, matBSDFs);
			if(matBSDFs & (BSDF_DIFFUSE | BSDF_GLOSSY))
			{
				vector3d_t sf = FACE_FORWARD(hit.Ng, hit.N, -pRay.dir);
				const photon_t *nearest = session.radianceMap->findNearest(hit.P, sf, lookupRad);
				if(nearest) lcol = nearest->color();
				if(matBSDFs & BSDF_EMIT) lcol += p_mat->emit(state, hit, -pRay.dir);
				pathCol += lcol * throughput;
			}
		}
		state.userdata = first_udat;
	}
	return pathCol / (float)nSampl;
}

colorA_t photonIntegrator_t::integrate(renderState_t &state, diffRay_t &ray, colorPasses_t &colorPasses, int additionalDepth /*=0*/) const
{
	static int _nMax=0;
	static int calls=0;
	++calls;
	color_t col(0.0);
	CFLOAT alpha;
	surfacePoint_t sp;
	
	void *o_udat = state.userdata;
	bool oldIncludeLights = state.includeLights;
	
	if(transpBackground) alpha=0.0;
	else alpha=1.0;
	
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
		int additionalDepth = 0;
		
		vector3d_t N_nobump = sp.N;
		vector3d_t wo = -ray.dir;
		const material_t *material = sp.material;
		material->initBSDF(state, sp, bsdfs);

		if(additionalDepth < material->getAdditionalDepth()) additionalDepth = material->getAdditionalDepth();
		
		col += colorPasses.probe_add(PASS_INT_EMIT, material->emit(state, sp, wo), state.raylevel == 0);
		
		state.includeLights = false;
		
		if(diffuse_enabled() && finalGather)
		{
			if(showMap)
			{
				vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
				const photon_t *nearest = session.radianceMap->findNearest(sp.P, N, lookupRad);
				if(nearest) col += nearest->color();
			}
			else
			{
				if(state.raylevel == 0 && colorPasses.enabled(PASS_INT_RADIANCE))
				{
					vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
					const photon_t *nearest = session.radianceMap->findNearest(sp.P, N, lookupRad);
					if(nearest) colorPasses(PASS_INT_RADIANCE) = nearest->color();
				}
				
				// contribution of light emitting surfaces
				if(bsdfs & BSDF_EMIT) col += colorPasses.probe_add(PASS_INT_EMIT, material->emit(state, sp, wo), state.raylevel == 0);
				
				if(bsdfs & BSDF_DIFFUSE)
				{
					col += estimateAllDirectLight(state, sp, wo, colorPasses);;
					
					if(AA_clamp_indirect>0.f)
					{
						color_t tmpCol = finalGathering(state, sp, wo, colorPasses);
						tmpCol.clampProportionalRGB(AA_clamp_indirect);
						col += colorPasses.probe_set(PASS_INT_DIFFUSE_INDIRECT, tmpCol, state.raylevel == 0);
					}
					else col += colorPasses.probe_set(PASS_INT_DIFFUSE_INDIRECT, finalGathering(state, sp, wo, colorPasses), state.raylevel == 0);
				}
			}
		}
		else
		{
			if(diffuse_enabled() && showMap)
			{
				vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
				const photon_t *nearest = session.diffuseMap->findNearest(sp.P, N, dsRadius);
				if(nearest) col += nearest->color();
			}
			else
			{
				if(diffuse_enabled() && state.raylevel == 0 && colorPasses.enabled(PASS_INT_RADIANCE))
				{
					vector3d_t N = FACE_FORWARD(sp.Ng, sp.N, wo);
					const photon_t *nearest = session.radianceMap->findNearest(sp.P, N, lookupRad);
					if(nearest) colorPasses(PASS_INT_RADIANCE) = nearest->color();
				}

				if(bsdfs & BSDF_EMIT) col += colorPasses.probe_add(PASS_INT_EMIT, material->emit(state, sp, wo), state.raylevel == 0);
				
				if(bsdfs & BSDF_DIFFUSE)
				{
					col += estimateAllDirectLight(state, sp, wo, colorPasses);
				}
				
				foundPhoton_t *gathered = (foundPhoton_t *)alloca(nDiffuseSearch * sizeof(foundPhoton_t));
				PFLOAT radius = dsRadius; //actually the square radius...

				int nGathered=0;
				
				if(diffuse_enabled() && session.diffuseMap->nPhotons() > 0) nGathered = session.diffuseMap->gather(sp.P, gathered, nDiffuseSearch, radius);
				color_t sum(0.0);
				if(diffuse_enabled() && nGathered > 0)
				{
					if(nGathered > _nMax) _nMax = nGathered;

					float scale = 1.f / ( (float)session.diffuseMap->nPaths() * radius * M_PI);
					for(int i=0; i<nGathered; ++i)
					{
						vector3d_t pdir = gathered[i].photon->direction();
						color_t surfCol = material->eval(state, sp, wo, pdir, BSDF_DIFFUSE);

						col += colorPasses.probe_add(PASS_INT_DIFFUSE_INDIRECT, surfCol * scale * gathered[i].photon->color(), state.raylevel == 0);
					}
				}
			}
		}
		
		// add caustics
		if(caustics_enabled() && bsdfs & BSDF_DIFFUSE)
		{
			if(AA_clamp_indirect>0.f)
			{
				color_t tmpCol = estimateCausticPhotons(state, sp, wo);
				tmpCol.clampProportionalRGB(AA_clamp_indirect);
				col += colorPasses.probe_set(PASS_INT_INDIRECT, tmpCol, state.raylevel == 0);
			}
			else col += colorPasses.probe_set(PASS_INT_INDIRECT, estimateCausticPhotons(state, sp, wo), state.raylevel == 0);
		}
		
		recursiveRaytrace(state, ray, bsdfs, sp, wo, col, alpha, colorPasses, additionalDepth);

		if(colorPasses.size() > 1 && state.raylevel == 0)
		{
			generateCommonRenderPasses(colorPasses, state, sp);
			
			if(colorPasses.enabled(PASS_INT_AO))
			{
				colorPasses(PASS_INT_AO) = sampleAmbientOcclusionPass(state, sp, wo);
			}

			if(colorPasses.enabled(PASS_INT_AO_CLAY))
			{
				colorPasses(PASS_INT_AO_CLAY) = sampleAmbientOcclusionPassClay(state, sp, wo);
			}
		}
		
		if(transpRefractedBackground)
		{
			CFLOAT m_alpha = material->getAlpha(state, sp, wo);
			alpha = m_alpha + (1.f-m_alpha)*alpha;
		}
		else alpha = 1.0;
	}
	else //nothing hit, return background
	{
		if(background && !transpRefractedBackground)
		{
			col += colorPasses.probe_set(PASS_INT_ENV, (*background)(ray, state, false), state.raylevel == 0);
		}
	}
	
	state.userdata = o_udat;
	state.includeLights = oldIncludeLights;
	
	color_t colVolTransmittance = scene->volIntegrator->transmittance(state, ray);
	color_t colVolIntegration = scene->volIntegrator->integrate(state, ray, colorPasses);

	if(transpBackground) alpha = std::max(alpha, 1.f-colVolTransmittance.R);

	colorPasses.probe_set(PASS_INT_VOLUME_TRANSMITTANCE, colVolTransmittance);
	colorPasses.probe_set(PASS_INT_VOLUME_INTEGRATION, colVolIntegration);
		
	col = (col * colVolTransmittance) + colVolIntegration;
	
	return colorA_t(col, alpha);
}

integrator_t* photonIntegrator_t::factory(paraMap_t &params, renderEnvironment_t &render)
{
	bool transpShad=false;
	bool finalGather=true;
	bool show_map=false;
	int shadowDepth=5;
	int raydepth=5;
	int numPhotons = 100000;
	int numCPhotons = 500000;
	int search = 50;
	int caustic_mix = 50;
	int bounces = 5;
	int fgPaths = 32;
	int fgBounces = 2;
	float dsRad=0.1;
	float cRad=0.01;
	float gatherDist=0.2;
	bool do_AO=false;
	int AO_samples = 32;
	double AO_dist = 1.0;
	color_t AO_col(1.f);
	bool bg_transp = false;
	bool bg_transp_refract = false;
	bool enable_caustics = true;
	bool enable_diffuse = true;
	std::string photon_maps_processing_str = "generate";
	
	params.getParam("enable_caustics", enable_caustics);
	params.getParam("enable_diffuse", enable_diffuse);
	
	params.getParam("transpShad", transpShad);
	params.getParam("shadowDepth", shadowDepth);
	params.getParam("raydepth", raydepth);
	params.getParam("photons", numPhotons);
	params.getParam("cPhotons", numCPhotons);
	params.getParam("diffuseRadius", dsRad);
	params.getParam("causticRadius", cRad);
	params.getParam("search", search);
	caustic_mix = search;
	params.getParam("caustic_mix", caustic_mix);
	params.getParam("bounces", bounces);
	params.getParam("finalGather", finalGather);
	params.getParam("fg_samples", fgPaths);
	params.getParam("fg_bounces", fgBounces);
	gatherDist = dsRad;
	params.getParam("fg_min_pathlen", gatherDist);
	params.getParam("show_map", show_map);
	params.getParam("bg_transp", bg_transp);
	params.getParam("bg_transp_refract", bg_transp_refract);
	params.getParam("do_AO", do_AO);
	params.getParam("AO_samples", AO_samples);
	params.getParam("AO_distance", AO_dist);
	params.getParam("AO_color", AO_col);
	params.getParam("photon_maps_processing", photon_maps_processing_str);
	
	photonIntegrator_t* ite = new photonIntegrator_t(numPhotons, numCPhotons, transpShad, shadowDepth, dsRad, cRad);
	
	ite->bEnableCaustics = enable_caustics;
	ite->bEnableDiffuse = enable_diffuse;
	
	ite->rDepth = raydepth;
	ite->nDiffuseSearch = search;
	ite->nCausSearch = caustic_mix;
	ite->finalGather = finalGather;
	ite->maxBounces = bounces;
	ite->causDepth = bounces;
	ite->nPaths = fgPaths;
	ite->gatherBounces = fgBounces;
	ite->showMap = show_map;
	ite->gatherDist = gatherDist;
	// Background settings
	ite->transpBackground = bg_transp;
	ite->transpRefractedBackground = bg_transp_refract;
	// AO settings
    ite->useAmbientOcclusion = do_AO;
	ite->aoSamples = AO_samples;
	ite->aoDist = AO_dist;
	ite->aoCol = AO_col;

	if(photon_maps_processing_str == "generate-save") ite->photonMapProcessing = PHOTONS_GENERATE_AND_SAVE;
	else if(photon_maps_processing_str == "load") ite->photonMapProcessing = PHOTONS_LOAD;
	else if(photon_maps_processing_str == "reuse-previous") ite->photonMapProcessing = PHOTONS_REUSE;
	else ite->photonMapProcessing = PHOTONS_GENERATE_ONLY;
	
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
