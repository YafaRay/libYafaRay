#include <integrators/photonintegr.h>
#include <core_api/camera.h>

__BEGIN_YAFRAY

class prepassWorker_t: public yafthreads::thread_t
{
	public:
		prepassWorker_t(photonIntegrator_t *it, threadControl_t *c, int id, int logsp):
			integrator(it), control(c), threadID(id), log_spacing(logsp){};
		virtual void body();
		std::vector<irradSample_t> samples;
	protected:
		photonIntegrator_t *integrator;
		threadControl_t *control;
		int threadID;
		int log_spacing;
};

void prepassWorker_t::body()
{
	renderArea_t a;
	while(integrator->imageFilm->nextArea(a))
	{
		integrator->progressiveTile2(a, log_spacing, log_spacing==3, samples, threadID);
		control->countCV.lock();
		control->areas.push_back(a);
		control->countCV.signal();
		control->countCV.unlock();
		int s=integrator->scene->getSignals();
		if(s & Y_SIG_ABORT) break;
	}
	control->countCV.lock();
	++(control->finishedThreads);
	control->countCV.signal();
	control->countCV.unlock();
}

bool photonIntegrator_t::renderIrradPass()
{
	std::vector<irradSample_t> samples;
	for(int log_spacing=3; log_spacing >=0; --log_spacing)
	{
		int nthreads = scene->getNumThreads();
	#ifdef USING_THREADS
		if(nthreads>1)
		{
			threadControl_t tc;
			std::vector<prepassWorker_t *> workers;
			for(int i=0;i<nthreads;++i) workers.push_back(new prepassWorker_t(this, &tc, i, log_spacing));
			for(int i=0;i<nthreads;++i)	workers[i]->run();
			//update finished tiles
			tc.countCV.lock();
			while(tc.finishedThreads < nthreads)
			{
				tc.countCV.wait();
				for(size_t i=0; i<tc.areas.size(); ++i) imageFilm->finishArea(tc.areas[i]);
				tc.areas.clear();
			}
			tc.countCV.unlock();
			//join all threads (although they probably have exited already, but not necessarily):
			for(int i=0;i<nthreads;++i) workers[i]->wait();
			// combine gathered samples in one vector:
			for(int i=0;i<nthreads;++i)
			{
				samples.insert(samples.end(), workers[i]->samples.begin(), workers[i]->samples.end());
				workers[i]->samples.clear();
			}
			//
			for(int i=0;i<nthreads;++i) delete workers[i];
		}
		else
		{
	#endif
			renderArea_t a;
			while(imageFilm->nextArea(a))
			{
				progressiveTile2(a, log_spacing, log_spacing==3, samples, 0);
				imageFilm->finishArea(a);
				int s = scene->getSignals();
				if(s & Y_SIG_ABORT) break;
			}
	#ifdef USING_THREADS
		}
	#endif
		imageFilm->nextPass(false);
		//octree code...
		for(unsigned int i=0; i<samples.size(); ++i) irCache.insert(samples[i]);
		samples.clear();
	}
	return true; //hm...quite useless the return value :)	
}

bool photonIntegrator_t::progressiveTile(renderArea_t &a, int log_spacing, bool first, std::vector<irradSample_t> &samples, int threadID) const
{
	int spacing = 1<<log_spacing;
	int spacing_1 = spacing-1; 
	int bitmask = ~(spacing_1);
	int dbl_spacing = 1<<(log_spacing+1);
	
	int x1_s = (a.X + spacing_1) & bitmask;
	int y_s = (a.Y + spacing_1) & bitmask;
	int x2_s = x1_s, spacing1 = spacing, spacing2 = spacing;
	// every second processed line already has every second pixel done if first != true
	// so we need to find out which line needs double spacing an with which offset
	if(!first)
	{
		int dbl_spacing_1 = dbl_spacing-1;
		int xd_s = (a.X + dbl_spacing_1) & (~dbl_spacing_1);
		int yd_s = (a.Y + dbl_spacing_1) & (~dbl_spacing_1);
		if(yd_s > y_s)
		{
			if(xd_s == x2_s) x2_s += spacing;
			spacing2 = dbl_spacing;
		}
		else
		{
			if(xd_s == x1_s) x1_s += spacing;
			spacing1 = dbl_spacing;
		}
	}
	int end_x=a.X+a.W, end_y=a.Y+a.H;
	
	//!TODO!
	int resx = scene->getCamera()->resX();
	random_t prng(/* offset* */(resx*a.Y+a.X)+123);
	renderState_t state(&prng);
	state.threadID = threadID;
	state.samplingOffs = 0; //TODO...
	for(int y=a.Y; y<end_y; y+=dbl_spacing)
	{
		color_t col(0.f);
		
		for(int x=x1_s; x<end_x; x+=spacing1)
		{
			//c_ray = camera->shootRay(x+dx, y+dy, lens_u, lens_v, wt);
			col = fillIrradCache(state, x, y, first, samples);
			imageFilm->addSample(col, x, y, .5f, .5f, &a);
		}
		int y2 = y+spacing;
		if(y2 >= end_y) break;
		//this line is still empty, so use current spacing
		for(int x=x2_s; x<end_x; x+=spacing2)
		{
			//c_ray = camera->shootRay(x+dx, y+dy, lens_u, lens_v, wt);
			col = fillIrradCache(state, x, y2, first, samples);
			imageFilm->addSample(col, x, y2, .5f, .5f, &a);
		}
	}
	return true;
}

bool photonIntegrator_t::progressiveTile2(renderArea_t &a, int log_spacing, bool first, std::vector<irradSample_t> &samples, int threadID) const
{
	int done = first ? 0 : (a.W*a.H) >> ((log_spacing+1)*2);
	int tot = (a.W*a.H)>>(log_spacing*2);
	
	//!TODO!
	int resx = scene->getCamera()->resX();
	random_t prng(/* offset* */(resx*a.Y+a.X)+123);
	renderState_t state(&prng);
	state.threadID = threadID;
	state.samplingOffs = 0; //TODO...
	for(int i=done; i<tot; ++i)
	{
		color_t col(0.f);
		PFLOAT x = a.X + (PFLOAT)a.W * RI_S(i);
		PFLOAT y = a.Y + (PFLOAT)a.H * RI_vdC(i);
		
		col = fillIrradCache(state, x, y, first, samples);
		imageFilm->addSample(col, x, y, .5f, .5f, &a);
	}
	return true;
}

colorA_t photonIntegrator_t::fillIrradCache(renderState_t &state, PFLOAT x, PFLOAT y, bool first, std::vector<irradSample_t> &samples) const
{
	//color_t col(0.0);
	//surfacePoint_t sp;
	state.raylevel = 0;
	PFLOAT wt, dx=0.5f, dy=0.5f; // not sure what to use yet...we're not doing subpixels here...
	PFLOAT lens_u=0.5f, lens_v=0.5f; // these are more tricky...i just hope it somewhat works
	const camera_t* camera = scene->getCamera();
	diffRay_t c_ray = camera->shootRay(x+dx, y+dy, lens_u, lens_v, wt);
	if(wt==0.0) return color_t(0.f);
	//setup ray differentials
	ray_t d_ray = camera->shootRay(x+1+dx, y+dy, lens_u, lens_v, wt);
	c_ray.xfrom = d_ray.from;
	c_ray.xdir = d_ray.dir;
	d_ray = camera->shootRay(x+dx, y+1+dy, lens_u, lens_v, wt);
	c_ray.yfrom = d_ray.from;
	c_ray.ydir = d_ray.dir;
	c_ray.time = state.time; // yet another questionmark...
	c_ray.hasDifferentials = true;
	return recFillCache(state, c_ray, first, samples);
}

colorA_t photonIntegrator_t::recFillCache(renderState_t &state, diffRay_t &c_ray, bool first, std::vector<irradSample_t> &samples) const
{
	color_t col(0.0);
	surfacePoint_t sp;
	
	if(scene->intersect(c_ray, sp))
	{
		state.userdata = alloca(USER_DATA_SIZE);
		spDifferentials_t spDiff(sp, c_ray);
		BSDF_t bsdfs;
		vector3d_t N_nobump = sp.N;
		vector3d_t wo = -c_ray.dir;
		const material_t *material = sp.material;
		material->initBSDF(state, sp, bsdfs);
		
		PFLOAT A_pix = spDiff.projectedPixelArea();
		std::swap(sp.N, N_nobump);
		if( ( first || ! irCache.enoughSamples(sp, A_pix) ) && bsdfs & BSDF_DIFFUSE )
		{
			irradSample_t irSample;
			sampleIrrad(state, sp, wo, irSample);
			irSample.Apix = A_pix;
			//irCache.insert(irSample, A_pix);
			samples.push_back(irSample);
			col += irSample.col;//color_t(1.f, 0.2f, 0.f);
		}
		std::swap(sp.N, N_nobump);
		
		state.raylevel++;
		if(state.raylevel <= rDepth)
		{
			// dispersive effects with recursive raytracing:
			if( (bsdfs & BSDF_DISPERSIVE) && state.chromatic )
			{
				//TODO
			}
			// glossy reflection with recursive raytracing:
			if( bsdfs & BSDF_GLOSSY )
			{
				//TODO
			}
			
			//...perfect specular reflection/refraction with recursive raytracing...
			if(bsdfs & (BSDF_SPECULAR | BSDF_FILTER))
			{
				bool reflect=false, refract=false;
				vector3d_t dir[2];
				color_t rcol[2], vcol;
				material->getSpecular(state, sp, wo, reflect, refract, &dir[0], &rcol[0]);
				if(reflect)
				{
					diffRay_t refRay(sp.P, dir[0], 0.0005);
					spDiff.reflectedRay(c_ray, refRay);
					col += recFillCache(state, refRay, first, samples);
				}
				if(refract)
				{
					diffRay_t refRay(sp.P, dir[1], 0.0005);
					spDiff.refractedRay(c_ray, refRay, 1.5f); //!TODO get IOR...
					col += recFillCache(state, refRay, first, samples);
				}
			}
			
		}
		--state.raylevel;
	}
	return col;
}

__END_YAFRAY
