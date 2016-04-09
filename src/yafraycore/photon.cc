
#include <yafraycore/photon.h>

__BEGIN_YAFRAY

YAFRAYCORE_EXPORT dirConverter_t dirconverter;

dirConverter_t::dirConverter_t()
{
	for(int i=0;i<255;++i)
	{
		PFLOAT angle=(PFLOAT)i * cInv255Ratio;
		costheta[i]=fCos(angle);
		sintheta[i]=fSin(angle);
	}
	for(int i=0;i<256;++i)
	{
		PFLOAT angle=(PFLOAT)i * cInv256Ratio;
		cosphi[i]=fCos(angle);
		sinphi[i]=fSin(angle);
	}
}

photonGather_t::photonGather_t(u_int32 mp, const point3d_t &P): p(P)
{
	photons = 0;
	nLookup = mp;
	foundPhotons = 0;
}

void photonGather_t::operator()(const photon_t *photon, PFLOAT dist2, PFLOAT &maxDistSquared) const
{
	// Do usual photon heap management
	if (foundPhotons < nLookup) {
		// Add photon to unordered array of photons
		photons[foundPhotons++] = foundPhoton_t(photon, dist2);
		if (foundPhotons == nLookup) {
				std::make_heap(&photons[0], &photons[nLookup]);
				maxDistSquared = photons[0].distSquare;
		}
	}
	else {
		// Remove most distant photon from heap and add new photon
		std::pop_heap(&photons[0], &photons[nLookup]);
		photons[nLookup-1] = foundPhoton_t(photon, dist2);
		std::push_heap(&photons[0], &photons[nLookup]);
		maxDistSquared = photons[0].distSquare;
	}
}

bool photonMapLoad(photonMap_t &map, const std::string &filename, bool debugXMLformat)
{
	try
	{
		std::ifstream ifs(filename, std::fstream::binary);
		
		if(debugXMLformat)
		{
			boost::archive::xml_iarchive ia(ifs);
			map.clear();
			ia >> BOOST_SERIALIZATION_NVP(map);
			ifs.close();
		}
		else
		{
			boost::archive::binary_iarchive ia(ifs);
			map.clear();
			ia >> BOOST_SERIALIZATION_NVP(map);
			ifs.close();
		}
		return true;
	}
	catch(std::exception& ex){
        // elminate any dangling references
        map.clear();
        Y_WARNING << "PhotonMap: error '" << ex.what() << "' while loading photon map file: '" << filename << "'" << yendl;
		return false;
    }
}

bool photonMapSave(const photonMap_t &map, const std::string &filename, bool debugXMLformat)
{
	try
	{
		std::ofstream ofs(filename, std::fstream::binary);

		if(debugXMLformat)
		{
			boost::archive::xml_oarchive oa(ofs);
			oa << BOOST_SERIALIZATION_NVP(map);
			ofs.close();
		}
		else
		{
			boost::archive::binary_oarchive oa(ofs);
			oa << BOOST_SERIALIZATION_NVP(map);
			ofs.close();
		}
		return true;
	}
	catch(std::exception& ex){
        Y_WARNING << "PhotonMap: error '" << ex.what() << "' while saving photon map file: '" << filename << "'" << yendl;
		return false;
    }
}

void photonMap_t::updateTree()
{
	if(tree) delete tree;
	if(photons.size() > 0)
	{
		tree = new kdtree::pointKdTree<photon_t>(photons);
		updated = true;
	}
	else tree=0;
}

int photonMap_t::gather(const point3d_t &P, foundPhoton_t *found, unsigned int K, PFLOAT &sqRadius) const
{
	photonGather_t proc(K, P);
	proc.photons = found;
	tree->lookup(P, proc, sqRadius);
	return proc.foundPhotons;
}

const photon_t* photonMap_t::findNearest(const point3d_t &P, const vector3d_t &n, PFLOAT dist) const
{
	nearestPhoton_t proc(P, n);
	//PFLOAT dist=std::numeric_limits<PFLOAT>::infinity(); //really bad idea...
	tree->lookup(P, proc, dist);
	return proc.nearest;
}

__END_YAFRAY
