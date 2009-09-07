
#include <yafraycore/old_photonmap.h>

__BEGIN_YAFRAY

static bound_t global_photon_calc_bound(const std::vector<const photon_t *> &v)
{
	int size=v.size();
	if(size==0) return bound_t(point3d_t(),point3d_t());
	PFLOAT maxx,maxy,maxz,minx,miny,minz;
	maxx=minx=v[0]->position().x;
	maxy=miny=v[0]->position().y;
	maxz=minz=v[0]->position().z;
	for(int i=0;i<size;++i)
	{
		const point3d_t &p=v[i]->position();
		if(p.x>maxx) maxx=p.x;
		if(p.y>maxy) maxy=p.y;
		if(p.z>maxz) maxz=p.z;
		if(p.x<minx) minx=p.x;
		if(p.y<miny) miny=p.y;
		if(p.z<minz) minz=p.z;
	}
	return bound_t(point3d_t(minx,miny,minz),point3d_t(maxx,maxy,maxz));
}

static bool global_photon_is_in_bound(const photon_t * const & p,bound_t &b)
{
	return b.includes(p->position());
}

static point3d_t global_photon_get_pos(const photon_t * const & p)
{
	return p->position();
}

globalPhotonMap_t::globalPhotonMap_t():maxradius(1.0), paths(0), tree(NULL), Y_LOOKUPS(0), Y_PROCS(0), Y_FIRST(0)
{
}

globalPhotonMap_t::~globalPhotonMap_t()
{
	if(tree) delete tree;
}

void globalPhotonMap_t::updateTree()
{
	std::vector<const photon_t *> lpho(photons.size());
	for(unsigned int i=0;i<photons.size();++i)
		lpho[i]=&photons[i];

	if(tree!=NULL) delete tree;
	tree=buildGenericTree(lpho,global_photon_calc_bound,global_photon_is_in_bound,
				global_photon_get_pos,8);
}

struct searchCircle_t
{
	searchCircle_t(const point3d_t &pp,PFLOAT r):point(pp),radius(r) {};
	point3d_t point;
	PFLOAT radius;
};


struct circleCross_f
{
	bool operator()(const searchCircle_t &c,const bound_t &b)const 
	{
		point3d_t a,g;
		b.get(a,g);
		a+=-c.radius;
		g+=c.radius;
		bound_t bt(a,g);
		return bt.includes(c.point);
	};
};


struct compareFound_f
{
	bool operator () (const foundPhoton_t &a,const foundPhoton_t &b)
	{
		return a.dis<b.dis;
	}
};

void globalPhotonMap_t::gather(const point3d_t &P,const vector3d_t &N,
		std::vector<foundPhoton_t> &found,
		unsigned int K,PFLOAT &radius,PFLOAT mincos)const
{
	++Y_LOOKUPS;
	if(Y_LOOKUPS == 159999)
	{
		std::cout << "average photons tested per lookup:" << double(Y_PROCS)/double(Y_LOOKUPS);
	}
	foundPhoton_t temp;
	compareFound_f cfound;
	//found.reserve(K+1);
	unsigned int reached=0;
	while((reached<K) && (radius<=maxradius))
	{
		reached=0;
		//found.clear();
		found.resize(0);
		searchCircle_t circle(P,radius);
		for(gObjectIterator_t<const photon_t *,searchCircle_t,circleCross_f> 
				i(tree,circle);!i;++i)
		{
			vector3d_t sep=(*i)->position()-P;
			CFLOAT D=sep.length();
			if((D>radius)/* || (((*i)->direction()*N)<=mincos)*/) continue;
			reached++;
			++Y_PROCS;
			temp.photon=*i;
			temp.dis=D;
			if((found.size()==K) && (temp.dis>found.front().dis)) continue;
			if(found.size()==K)
			{
				found.push_back(temp);
				push_heap(found.begin(),found.end(),cfound);
				pop_heap(found.begin(),found.end(),cfound);
				found.pop_back();
			}
			else
			{
				found.push_back(temp);
				push_heap(found.begin(),found.end(),cfound);
			}
		}
		if(reached<K) radius*=2;
	}
	if(reached>K)
	{
		PFLOAT f=(PFLOAT)K/(PFLOAT)reached;
		if(f<(0.7*0.7)) radius*=0.95;
	}
	if(radius>maxradius) radius=maxradius;
}

__END_YAFRAY
