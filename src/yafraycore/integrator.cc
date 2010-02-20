
#include <yafraycore/tiledintegrator.h>
#include <core_api/imagefilm.h>
#include <core_api/camera.h>
#include <yafraycore/timer.h>
#include <yafraycore/scr_halton.h>
#include <utilities/mcqmc.h>
#include <utilities/sample_utils.h>
#include <sstream>

__BEGIN_YAFRAY

#ifdef USING_THREADS

class renderWorker_t: public yafthreads::thread_t
{
	public:
		renderWorker_t(tiledIntegrator_t *it, scene_t *s, imageFilm_t *f, threadControl_t *c, int id, int smpls, int offs=0, bool adptv=false):
			integrator(it), scene(s), imageFilm(f), control(c), samples(smpls), offset(offs), threadID(id), adaptive(adptv)
		{ /* std::cout << "renderWorker_t::renderWorker_t(): *this="<<(void*)this<<std::endl; */ };
		virtual void body();
	protected:
		tiledIntegrator_t *integrator;
		scene_t *scene;
		imageFilm_t *imageFilm;
		threadControl_t *control;
		int samples, offset;
		int threadID;
		bool adaptive;
};

void renderWorker_t::body()
{
	renderArea_t a;
	while(imageFilm->nextArea(a))
	{
		if(scene->getSignals() & Y_SIG_ABORT) break;
		integrator->renderTile(a, samples, offset, adaptive, threadID);
//		imageFilm->finishArea(a);
		control->countCV.lock();
		control->areas.push_back(a);
		control->countCV.signal();
		control->countCV.unlock();
	}
	control->countCV.lock();
	++(control->finishedThreads);
	control->countCV.signal();
	control->countCV.unlock();
}
#endif

bool tiledIntegrator_t::render(imageFilm_t *image)
{
	std::stringstream passString;
	imageFilm = image;
	scene->getAAParameters(AA_samples, AA_passes, AA_inc_samples, AA_threshold);
	Y_INFO << integratorName << ": Rendering "<<AA_passes<<" passes\n";
	Y_INFO << integratorName << ": Min. " << AA_samples << " samples\n";
	Y_INFO << integratorName << ": "<< AA_inc_samples << " per additional pass\n";
	Y_INFO << integratorName << ": Max. "<<AA_samples + std::max(0,AA_passes-1)*AA_inc_samples<<" total samples\n";
	passString << "Rendering pass 1 of " << std::max(1, AA_passes) << "...";
	if(intpb) intpb->setTag(passString.str().c_str());

	gTimer.addEvent("rendert");
	gTimer.start("rendert");
	imageFilm->init(AA_passes);
	
	renderPass(AA_samples, 0, false);
	for(int i=1; i<AA_passes; ++i)
	{
		if(scene->getSignals() & Y_SIG_ABORT) break;
		imageFilm->setAAThreshold(AA_threshold);
		imageFilm->nextPass(true);
		renderPass(AA_inc_samples, AA_samples + (i-1)*AA_inc_samples, true);
	}

	gTimer.stop("rendert");
	Y_INFO << integratorName << ": Overall rendertime: "<< gTimer.getTime("rendert")<<"s\n";

	return true;
}


bool tiledIntegrator_t::renderPass(int samples, int offset, bool adaptive)
{
	int nthreads = scene->getNumThreads();
#ifdef USING_THREADS
	if(nthreads>1)
	{
		threadControl_t tc;
		std::vector<renderWorker_t *> workers;
		for(int i=0;i<nthreads;++i) workers.push_back(new renderWorker_t(this, scene, imageFilm, &tc, i, samples, offset, adaptive));
		for(int i=0;i<nthreads;++i)
		{
			workers[i]->run();
		}
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
		for(int i=0;i<nthreads;++i) delete workers[i];
	}
	else
	{
#endif
		renderArea_t a;
		while(imageFilm->nextArea(a))
		{
			if(scene->getSignals() & Y_SIG_ABORT) break;
			renderTile(a, samples, offset, adaptive,0);
			imageFilm->finishArea(a);
		}
#ifdef USING_THREADS
	}
#endif
	return true; //hm...quite useless the return value :)
}

bool tiledIntegrator_t::renderTile(renderArea_t &a, int n_samples, int offset, bool adaptive, int threadID)
{	
	int x, y;
	const camera_t* camera = scene->getCamera();
	bool do_depth = scene->doDepth();
	x=camera->resX();
	y=camera->resY();
	diffRay_t c_ray;
	ray_t d_ray;
	PFLOAT dx=0.5, dy=0.5, d1=1.0/(PFLOAT)n_samples;
	float lens_u=0.5f, lens_v=0.5f;
	PFLOAT wt, wt_dummy;
	random_t prng(offset*(x*a.Y+a.X)+123);
	renderState_t rstate(&prng);
	rstate.threadID = threadID;
	rstate.cam = camera;
	bool sampleLns = camera->sampleLense();
	int pass_offs=offset, end_x=a.X+a.W, end_y=a.Y+a.H;
	for(int i=a.Y; i<end_y; ++i)
	{
		for(int j=a.X; j<end_x; ++j)
		{
			if(scene->getSignals() & Y_SIG_ABORT) break;
			if(adaptive)
			{
				if(!imageFilm->doMoreSamples(j, i)) continue;
			}
			rstate.pixelNumber = x*i+j;
			//rstate.screenpos = point3d_t(2.0f * j / x - 1.0f, -2.0f * i / y + 1.0f, 0.f); // screen position in -1..1 coordinates
			rstate.samplingOffs = fnv_32a_buf(i*fnv_32a_buf(j));//fnv_32a_buf(rstate.pixelNumber);
			float toff = scrHalton(5, pass_offs+rstate.samplingOffs); // **shall be just the pass number...**
			for(int sample=0; sample<n_samples; ++sample)
			{
				rstate.setDefaults();
				rstate.pixelSample = pass_offs+sample;
				rstate.time = addMod1((PFLOAT)sample*d1, toff);//(0.5+(PFLOAT)sample)*d1;
				// the (1/n, Larcher&Pillichshammer-Seq.) only gives good coverage when total sample count is known
				// hence we use scrambled (Sobol, van-der-Corput) for multipass AA
				if(AA_passes>1)
				{
					dx = RI_S(rstate.pixelSample, rstate.samplingOffs);
					dy = RI_vdC(rstate.pixelSample, rstate.samplingOffs);
				}
				else if(n_samples > 1)
				{
					dx = (0.5+(PFLOAT)sample)*d1;
					dy = RI_LP(sample+rstate.samplingOffs);
				}
				if(sampleLns)
				{
					lens_u = scrHalton(3, rstate.pixelSample+rstate.samplingOffs);
					lens_v = scrHalton(4, rstate.pixelSample+rstate.samplingOffs);
				}
				c_ray = camera->shootRay(j+dx, i+dy, lens_u, lens_v, wt);
				if(wt==0.0)
				{
					imageFilm->addSample(colorA_t(0.f), j, i, dx, dy, &a);
					continue;
				}
				//setup ray differentials
				d_ray = camera->shootRay(j+1+dx, i+dy, lens_u, lens_v, wt_dummy);
				c_ray.xfrom = d_ray.from;
				c_ray.xdir = d_ray.dir;
				d_ray = camera->shootRay(j+dx, i+1+dy, lens_u, lens_v, wt_dummy);
				c_ray.yfrom = d_ray.from;
				c_ray.ydir = d_ray.dir;
				c_ray.time = rstate.time;
				c_ray.hasDifferentials = true;
				// col = T * L_o + L_v
				colorA_t col = integrate(rstate, c_ray); // L_o
				// I really don't like this here, bert...
				col *= scene->volIntegrator->transmittance(rstate, c_ray); // T
				col += scene->volIntegrator->integrate(rstate, c_ray); // L_v
				imageFilm->addSample(wt * col, j, i, dx, dy,/*.5f, .5f,*/ &a);
			}
			if(do_depth) imageFilm->setChanPixel(c_ray.tmax, 0, j, i);
		}
	}
	return true;
}

__END_YAFRAY
