
#include <yafraycore/photon.h>
#include <core_api/file.h>

__BEGIN_YAFRAY

YAFRAYCORE_EXPORT dirConverter_t dirconverter;

dirConverter_t::dirConverter_t()
{
	for(int i=0;i<255;++i)
	{
		float angle=(float)i * cInv255Ratio;
		costheta[i]=fCos(angle);
		sintheta[i]=fSin(angle);
	}
	for(int i=0;i<256;++i)
	{
		float angle=(float)i * cInv256Ratio;
		cosphi[i]=fCos(angle);
		sinphi[i]=fSin(angle);
	}
}

photonGather_t::photonGather_t(uint32_t mp, const point3d_t &P): p(P)
{
	photons = 0;
	nLookup = mp;
	foundPhotons = 0;
}

void photonGather_t::operator()(const photon_t *photon, float dist2, float &maxDistSquared) const
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

bool photonMap_t::load(const std::string &filename)
{
	clear();

	file_t file(filename);
	if (!file.open("rb"))
	{
		Y_WARNING << "PhotonMap file '" << filename << "' not found, aborting load operation";
		return false;
	}

	std::string header;
	file.read(header);
	if(header != "YAF_PHOTONMAPv1")
	{
		Y_WARNING << "PhotonMap file '" << filename << "' does not contain a valid YafaRay photon map";
		file.close();
		return false;
	}
	file.read(name);
	file.read<int>(paths);
	file.read<float>(searchRadius);
	file.read<int>(threadsPKDtree);
	unsigned int photons_size;
	file.read<unsigned int>(photons_size);
	photons.resize(photons_size);
	for(auto &p : photons)
	{
		file.read<float>(p.pos.x);
		file.read<float>(p.pos.y);
		file.read<float>(p.pos.z);
		file.read<float>(p.c.R);
		file.read<float>(p.c.G);
		file.read<float>(p.c.B);
	}
	file.close();

	updateTree();
	return true;
}

bool photonMap_t::save(const std::string &filename) const
{
	file_t file(filename);
	file.open("wb");
	file.append(std::string("YAF_PHOTONMAPv1"));
	file.append(name);
	file.append<int>(paths);
	file.append<float>(searchRadius);
	file.append<int>(threadsPKDtree);
	file.append<unsigned int>((unsigned int) photons.size());
	for(const auto &p : photons)
	{
		file.append<float>(p.pos.x);
		file.append<float>(p.pos.y);
		file.append<float>(p.pos.z);
		file.append<float>(p.c.R);
		file.append<float>(p.c.G);
		file.append<float>(p.c.B);
	}
	file.close();
	return true;
}

void photonMap_t::updateTree()
{
	if(tree) delete tree;
	if(photons.size() > 0)
	{
		tree = new kdtree::pointKdTree<photon_t>(photons, name, threadsPKDtree);
		updated = true;
	}
	else tree=0;
}

int photonMap_t::gather(const point3d_t &P, foundPhoton_t *found, unsigned int K, float &sqRadius) const
{
	photonGather_t proc(K, P);
	proc.photons = found;
	tree->lookup(P, proc, sqRadius);
	return proc.foundPhotons;
}

const photon_t* photonMap_t::findNearest(const point3d_t &P, const vector3d_t &n, float dist) const
{
	nearestPhoton_t proc(P, n);
	//float dist=std::numeric_limits<float>::infinity(); //really bad idea...
	tree->lookup(P, proc, dist);
	return proc.nearest;
}

__END_YAFRAY
