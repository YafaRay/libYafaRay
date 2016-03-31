// search for "todo" and "IMPLEMENT" and "<<" or ">>"...

#include <yafraycore/ray_kdtree.h>
#include <core_api/material.h>
#include <core_api/scene.h>
#include <stdexcept>
//#include <math.h>
#include <limits>
#include <set>
#if ( HAVE_PTHREAD && defined (__GNUC__) && !defined (__clang__))
#include <ext/mt_allocator.h>
#endif
#include <time.h>

__BEGIN_YAFRAY

#define LOWER_B 0
#define UPPER_B 2
#define BOTH_B  1

#define _TRI_CLIP 1 //tempoarily disabled
#define TRI_CLIP_THRESH 32
#define CLIP_DATA_SIZE (3*12*sizeof(double))
#define KD_BINS 1024

#define KD_MAX_STACK 64

// #define Y_MIN3(a,b,c) ( ((a)>(b)) ? ( ((b)>(c))?(c):(b)):( ((a)>(c))?(c):(a)) )
// #define Y_MAX3(a,b,c) ( ((a)<(b)) ? ( ((b)>(c))?(b):(c)):( ((a)>(c))?(a):(c)) )

#if (defined(_M_IX86) || defined(i386) || defined(_X86_))
	#define Y_FAST_INT 1
#else
	#define Y_FAST_INT 0
#endif

#define _doublemagicroundeps	      (0.5 - 1.4e-11)

inline int Y_Round2Int(double val) {
#if Y_FAST_INT > 0
	union { double f; int i[2]; } u;
	u.f	= val + 6755399441055744.0; //2^52 * 1.5,  uses limited precision to floor
	return u.i[0];
#else
	return int(val);
#endif
}

inline int Y_Float2Int(double val) {
#if Y_FAST_INT > 0
	return (val<0) ?  Y_Round2Int( val+_doublemagicroundeps ) :
		   Y_Round2Int(val-_doublemagicroundeps);
#else
	return (int)val;
#endif
}

//still in old file...
//int Kd_inodes=0, Kd_leaves=0, _emptyKd_leaves=0, Kd_prims=0, _clip=0, _bad_clip=0, _null_clip=0, _early_out=0;

//bound_t getTriBound(const triangle_t tri);
//int triBoxOverlap(double boxcenter[3],double boxhalfsize[3],double triverts[3][3]);
//int triBoxClip(const double b_min[3], const double b_max[3], const double triverts[3][3], bound_t &box);

template<class T>
kdTree_t<T>::kdTree_t(const T **v, int np, int depth, int leafSize,
			float cost_ratio, float emptyBonus)
	: costRatio(cost_ratio), eBonus(emptyBonus), maxDepth(depth)
{
	std::cout << "starting build of kd-tree ("<<np<<" prims, cr:"<<costRatio<<" eb:"<<eBonus<<")\n";
	clock_t c_start, c_end;
	c_start = clock();
	Kd_inodes=0, Kd_leaves=0, _emptyKd_leaves=0, Kd_prims=0, depthLimitReached=0, NumBadSplits=0,
		_clip=0, _bad_clip=0, _null_clip=0, _early_out=0;
	totalPrims = np;
	nextFreeNode = 0;
	allocatedNodesCount = 256;
	nodes = (rkdTreeNode<T>*)y_memalign(64, 256 * sizeof(rkdTreeNode<T>));
	if(maxDepth <= 0) maxDepth = int( 7.0f + 1.66f * log(float(totalPrims)) );
	double logLeaves = 1.442695f * log(double(totalPrims)); // = base2 log
	if(leafSize <= 0)
	{
		//maxLeafSize = int( 1.442695f * log(float(totalPrims)/62500.0) );
		int mls = int( logLeaves - 16.0 );
		if(mls <= 0) mls = 1;
		maxLeafSize = (unsigned int) mls;
	}
	else maxLeafSize = (unsigned int) leafSize;
	if(maxDepth>KD_MAX_STACK) maxDepth = KD_MAX_STACK; //to prevent our stack to overflow
	//experiment: add penalty to cost ratio to reduce memory usage on huge scenes
	if( logLeaves > 16.0 ) costRatio += 0.25*( logLeaves - 16.0 );
	allBounds = new bound_t[totalPrims + TRI_CLIP_THRESH+1];
	std::cout << "getting triangle bounds...";
	for(u_int32 i=0; i<totalPrims; i++)
	{
		allBounds[i] = v[i]->getBound();
		/* calc tree bound. Remember to upgrade bound_t class... */
		if(i) treeBound = bound_t(treeBound, allBounds[i]);
		else treeBound = allBounds[i];
	}
	//slightly(!) increase tree bound to prevent errors with prims
	//lying in a bound plane (still slight bug with trees where one dim. is 0)
	for(int i=0;i<3;i++)
	{
		double foo = (treeBound.g[i] - treeBound.a[i])*0.001;
		treeBound.a[i] -= foo, treeBound.g[i] += foo;
	}
	std::cout << "done!\n";
	// get working memory for tree construction
	boundEdge *edges[3];
	u_int32 rMemSize = 3*totalPrims; // (maxDepth+1)*totalPrims;
	u_int32 *leftPrims = new u_int32[std::max( (u_int32)2*TRI_CLIP_THRESH, totalPrims )];
	u_int32 *rightPrims = new u_int32[rMemSize]; //just a rough guess, allocating worst case is insane!
//	u_int32 *primNums = new u_int32[totalPrims]; //isn't this like...totaly unnecessary? use leftPrims?
	for (int i = 0; i < 3; ++i) edges[i] = new boundEdge[514/*2*totalPrims*/];
	clip = new int[maxDepth+2];
	cdata = (char*)y_memalign(64, (maxDepth+2)*TRI_CLIP_THRESH*CLIP_DATA_SIZE);
	
	// prepare data
	for (u_int32 i = 0; i < totalPrims; i++) leftPrims[i] = i;//primNums[i] = i;
	for (int i = 0; i < maxDepth+2; i++) clip[i] = -1;
	
	/* build tree */
	prims = v;
	std::cout << "starting recursive build...\n";
	buildTree(totalPrims, treeBound, leftPrims,
			  leftPrims, rightPrims, edges, // <= working memory
			  rMemSize, 0, 0 );
	
	// free working memory
	delete[] leftPrims;
	delete[] rightPrims;
	delete[] allBounds;
	for (int i = 0; i < 3; ++i) delete[] edges[i];
	delete[] clip;
	y_free(cdata);
	//print some stats:
	c_end = clock() - c_start;
	std::cout << "\n=== kd-tree stats ("<< float(c_end) / (float)CLOCKS_PER_SEC <<"s) ===\n";
	std::cout << "used/allocated kd-tree nodes: " << nextFreeNode << "/" << allocatedNodesCount
		<< " (" << 100.f * float(nextFreeNode)/allocatedNodesCount << "%)\n";
	std::cout << "primitives in tree: " << totalPrims << std::endl;
	std::cout << "interior nodes: " << Kd_inodes << " / " << "leaf nodes: " << Kd_leaves
		<< " (empty: " << _emptyKd_leaves << " = " << 100.f * float(_emptyKd_leaves)/Kd_leaves << "%)\n";
	std::cout << "leaf prims: " << Kd_prims << " (" << float(Kd_prims)/totalPrims << "x prims in tree, leaf size:"<< maxLeafSize<<")\n";
	std::cout << "   => " << float(Kd_prims)/ (Kd_leaves-_emptyKd_leaves) << " prims per non-empty leaf\n";
	std::cout << "leaves due to depth limit/bad splits: " << depthLimitReached << "/" << NumBadSplits << "\n";
	std::cout << "clipped triangles: " << _clip << " (" <<_bad_clip << " bad clips, "<<_null_clip
		<<" null clips)\n";
	//std::cout << "early outs: " << _early_out << "\n\n";
}

template<class T>
kdTree_t<T>::~kdTree_t()
{
//	std::cout << "kd-tree destructor: freeing nodes...";
	y_free(nodes);
//	std::cout << "done!\n";
	//y_free(prims); //überflüssig?
}

// ============================================================
/*!
	Faster cost function: Find the optimal split with SAH
	and binning => O(n)
*/

template<class T>
void kdTree_t<T>::pigeonMinCost(u_int32 nPrims, bound_t &nodeBound, u_int32 *primIdx, splitCost_t &split)
{
	bin_t bin[ KD_BINS+1 ];
	PFLOAT d[3];
	d[0] = nodeBound.longX();
	d[1] = nodeBound.longY();
	d[2] = nodeBound.longZ();
	split.oldCost = float(nPrims);
	split.bestCost = std::numeric_limits<PFLOAT>::infinity();
	float invTotalSA = 1.0f / (d[0]*d[1] + d[0]*d[2] + d[1]*d[2]);
	PFLOAT t_low, t_up;
	int b_left, b_right;
	
	for(int axis=0;axis<3;axis++)
	{
		PFLOAT s = KD_BINS/d[axis];
		PFLOAT min = nodeBound.a[axis];
		// pigeonhole sort:
		for(unsigned int i=0; i<nPrims; ++i)
		{
			const bound_t &bbox = allBounds[ primIdx[i] ];
			t_low = bbox.a[axis];
			t_up  = bbox.g[axis];
			b_left = (int)((t_low - min)*s);
			b_right = (int)((t_up - min)*s);
//			b_left = Y_Round2Int( ((t_low - min)*s) );
//			b_right = Y_Round2Int( ((t_up - min)*s) );
			if(b_left<0) b_left=0; else if(b_left > KD_BINS) b_left = KD_BINS;
			if(b_right<0) b_right=0; else if(b_right > KD_BINS) b_right = KD_BINS;
			
			if(t_low == t_up)
			{
				if(bin[b_left].empty() || (t_low >= bin[b_left].t && !bin[b_left].empty() ) )
				{
					bin[b_left].t = t_low;
					bin[b_left].c_both++;
				}
				else
				{
					bin[b_left].c_left++;
					bin[b_left].c_right++;
				}
				bin[b_left].n += 2;
			}
			else
			{	
				if(bin[b_left].empty() || (t_low > bin[b_left].t  && !bin[b_left].empty() ) )
				{
					bin[b_left].t = t_low;
					bin[b_left].c_left += bin[b_left].c_both + bin[b_left].c_bleft;
					bin[b_left].c_right += bin[b_left].c_both;
					bin[b_left].c_both = bin[b_left].c_bleft = 0;
					bin[b_left].c_bleft++;
				}
				else if(t_low == bin[b_left].t)
				{
					bin[b_left].c_bleft++;
				}
				else bin[b_left].c_left++;
				bin[b_left].n++;
				
				bin[b_right].c_right++;
				if(bin[b_right].empty() || t_up > bin[b_right].t)
				{
					bin[b_right].t = t_up;
					bin[b_right].c_left += bin[b_right].c_both + bin[b_right].c_bleft;
					bin[b_right].c_right += bin[b_right].c_both;
					bin[b_right].c_both = bin[b_right].c_bleft = 0;
				}
				bin[b_right].n++;
			}

		}
		
		const int axisLUT[3][3] = { {0,1,2}, {1,2,0}, {2,0,1} };
		float capArea = d[ axisLUT[1][axis] ] * d[ axisLUT[2][axis] ];
		float capPerim = d[ axisLUT[1][axis] ] + d[ axisLUT[2][axis] ];
		
		unsigned int nBelow=0, nAbove=nPrims;
		// cumulate prims and evaluate cost
		for(int i=0; i<KD_BINS+1; ++i)
		{
			if(!bin[i].empty())
			{	
				nBelow += bin[i].c_left;
				nAbove -= bin[i].c_right;
				// cost:
				PFLOAT edget = bin[i].t;
				if (edget > nodeBound.a[axis] &&
					edget < nodeBound.g[axis]) {
					// Compute cost for split at _i_th edge
					float l1 = edget - nodeBound.a[axis];
					float l2 = nodeBound.g[axis] - edget;
					float belowSA = capArea + l1*capPerim;
					float aboveSA = capArea + l2*capPerim;
					float rawCosts = (belowSA * nBelow + aboveSA * nAbove);
					//float eb = (nAbove == 0 || nBelow == 0) ? eBonus*rawCosts : 0.f;
					float eb;
					if(nAbove == 0) eb = (0.1f + l2/d[axis])*eBonus*rawCosts;
					else if(nBelow == 0) eb = (0.1f + l1/d[axis])*eBonus*rawCosts;
					else eb = 0.0f;
					float cost = costRatio + invTotalSA * (rawCosts - eb);
					// Update best split if this is lowest cost so far
					if (cost < split.bestCost)  {
						split.t = edget;
						split.bestCost = cost;
						split.bestAxis = axis;
						split.bestOffset = i; // kinda useless...
						split.nBelow = nBelow;
						split.nAbove = nAbove;
					}
				}
				nBelow += bin[i].c_both + bin[i].c_bleft;
				nAbove -= bin[i].c_both;
			}
		} // for all bins
		if(nBelow != nPrims || nAbove != 0)
		{
			int c1=0, c2=0, c3=0, c4=0, c5=0;
			std::cout << "SCREWED!!\n";
			for(int i=0;i<KD_BINS+1;i++){ c1+= bin[i].n; std::cout << bin[i].n << " ";}
			std::cout << "\nn total: "<< c1 << "\n";
			for(int i=0;i<KD_BINS+1;i++){ c2+= bin[i].c_left; std::cout << bin[i].c_left << " ";}
			std::cout << "\nc_left total: "<< c2 << "\n";
			for(int i=0;i<KD_BINS+1;i++){ c3+= bin[i].c_bleft; std::cout << bin[i].c_bleft << " ";}
			std::cout << "\nc_bleft total: "<< c3 << "\n";
			for(int i=0;i<KD_BINS+1;i++){ c4+= bin[i].c_both; std::cout << bin[i].c_both << " ";}
			std::cout << "\nc_both total: "<< c4 << "\n";
			for(int i=0;i<KD_BINS+1;i++){ c5+= bin[i].c_right; std::cout << bin[i].c_right << " ";}
			std::cout << "\nc_right total: "<< c5 << "\n";
			std::cout << "\nnPrims: "<<nPrims<<" nBelow: "<<nBelow<<" nAbove: "<<nAbove<<"\n";
			std::cout << "total left: " << c2 + c3 + c4 << "\ntotal right: " << c4 + c5 << "\n";
			std::cout << "n/2: " << c1/2 << "\n";
			throw std::logic_error("cost function mismatch");
		}
		for(int i=0;i<KD_BINS+1;i++) bin[i].reset();
	} // for all axis
}

// ============================================================
/*!
	Cost function: Find the optimal split with SAH
*/

template<class T>
void kdTree_t<T>::minimalCost(u_int32 nPrims, bound_t &nodeBound, u_int32 *primIdx,
		const bound_t *pBounds, boundEdge *edges[3], splitCost_t &split)
{
	PFLOAT d[3];
	d[0] = nodeBound.longX();
	d[1] = nodeBound.longY();
	d[2] = nodeBound.longZ();
	split.oldCost = float(nPrims);
	split.bestCost = std::numeric_limits<PFLOAT>::infinity();
	float invTotalSA = 1.0f / (d[0]*d[1] + d[0]*d[2] + d[1]*d[2]);
	int nEdge;
	
	for(int axis=0;axis<3;axis++)
	{
		// << get edges for axis >>
		int pn;
		nEdge=0;
		//test!
		if(pBounds!=allBounds) for (unsigned int i=0; i < nPrims; i++)
		{
			pn = primIdx[i];
			const bound_t &bbox = pBounds[i];
			if(bbox.a[axis] == bbox.g[axis])
			{
				edges[axis][nEdge] = boundEdge(bbox.a[axis], i /* pn */, BOTH_B);
				++nEdge;
			}
			else
			{
				edges[axis][nEdge] = boundEdge(bbox.a[axis], i /* pn */, LOWER_B);
				edges[axis][nEdge+1] = boundEdge(bbox.g[axis], i /* pn */, UPPER_B);
				nEdge += 2;
			}
		}
		else for (unsigned int i=0; i < nPrims; i++)
		{
			pn = primIdx[i];
			const bound_t &bbox = pBounds[pn];
			if(bbox.a[axis] == bbox.g[axis])
			{
				edges[axis][nEdge] = boundEdge(bbox.a[axis], pn, BOTH_B);
				++nEdge;
			}
			else
			{
				edges[axis][nEdge] = boundEdge(bbox.a[axis], pn, LOWER_B);
				edges[axis][nEdge+1] = boundEdge(bbox.g[axis], pn, UPPER_B);
				nEdge += 2;
			}
		}
		std::sort(&edges[axis][0], &edges[axis][nEdge]);
		// Compute cost of all splits for _axis_ to find best
		const int axisLUT[3][3] = { {0,1,2}, {1,2,0}, {2,0,1} };
		float capArea = d[ axisLUT[1][axis] ] * d[ axisLUT[2][axis] ];
		float capPerim = d[ axisLUT[1][axis] ] + d[ axisLUT[2][axis] ];
		unsigned int nBelow = 0, nAbove = nPrims;
		//todo: early-out criteria: if l1 > l2*nPrims (l2 > l1*nPrims) => minimum is lowest (highest) edge!
		if(nPrims>5)
		{
			PFLOAT edget = edges[axis][0].pos;
			float l1 = edget - nodeBound.a[axis];
			float l2 = nodeBound.g[axis] - edget;
			if(l1 > l2*float(nPrims) && l2 > 0.f)
			{
				float rawCosts = (capArea + l2*capPerim) * nPrims;
				float cost = costRatio + invTotalSA * (rawCosts - eBonus); //todo: use proper ebonus...
				//optimal cost is definitely here, and nowhere else!
				if (cost < split.bestCost)  {
					split.bestCost = cost;
					split.bestAxis = axis;
					split.bestOffset = 0;
					split.nEdge = nEdge;
					++_early_out;
				}
				continue;
			}
			edget = edges[axis][nEdge-1].pos;
			l1 = edget - nodeBound.a[axis];
			l2 = nodeBound.g[axis] - edget;
			if(l2 > l1*float(nPrims) && l1 > 0.f)
			{
				float rawCosts = (capArea + l1*capPerim) * nPrims;
				float cost = costRatio + invTotalSA * (rawCosts - eBonus); //todo: use proper ebonus...
				if (cost < split.bestCost)  {
					split.bestCost = cost;
					split.bestAxis = axis;
					split.bestOffset = nEdge-1;
					split.nEdge = nEdge;
					++_early_out;
				}
				continue;
			}
		}
		
		for (int i = 0; i < nEdge; ++i) {
			if (edges[axis][i].end == UPPER_B) --nAbove;
			PFLOAT edget = edges[axis][i].pos;
			if (edget > nodeBound.a[axis] &&
				edget < nodeBound.g[axis]) {
				// Compute cost for split at _i_th edge
				float l1 = edget - nodeBound.a[axis];
				float l2 = nodeBound.g[axis] - edget;
				float belowSA = capArea + (l1)*capPerim;
				float aboveSA = capArea + (l2)*capPerim;
				float rawCosts = (belowSA * nBelow + aboveSA * nAbove);
				//float eb = (nAbove == 0 || nBelow == 0) ? eBonus*rawCosts : 0.f;
				float eb;
				if(nAbove == 0) eb = (0.1f + l2/d[axis])*eBonus*rawCosts;
				else if(nBelow == 0) eb = (0.1f + l1/d[axis])*eBonus*rawCosts;
				else eb = 0.0f;
				float cost = costRatio + invTotalSA * (rawCosts - eb);
				// Update best split if this is lowest cost so far
				if (cost < split.bestCost)  {
					split.bestCost = cost;
					split.bestAxis = axis;
					split.bestOffset = i;
					split.nEdge = nEdge;
					//delete again:
					split.nBelow = nBelow;
					split.nAbove = nAbove;
				}
			}
			if (edges[axis][i].end != UPPER_B)
			{
				++nBelow;
				if (edges[axis][i].end == BOTH_B) --nAbove;
			}
		}
		if(nBelow != nPrims || nAbove != 0) std::cout << "you screwed your new idea!\n";
//		Assert(nBelow == nPrims && nAbove == 0);
	}
}

// ============================================================
/*!
	recursively build the Kd-tree
	returns:	0 when leaf was created
				1 when either current or at least 1 subsequent split reduced cost
				2 when neither current nor subsequent split reduced cost
*/
template<class T>
int kdTree_t<T>::buildTree(u_int32 nPrims, bound_t &nodeBound, u_int32 *primNums,
		u_int32 *leftPrims, u_int32 *rightPrims, boundEdge *edges[3], //working memory
		u_int32 rightMemSize, int depth, int badRefines ) // status
{
//	std::cout << "tree level: " << depth << std::endl;
	if (nextFreeNode == allocatedNodesCount) {
		int newCount = 2*allocatedNodesCount;
		newCount = (newCount > 0x100000) ? allocatedNodesCount+0x80000 : newCount;
		rkdTreeNode<T> 	*n = (rkdTreeNode<T> *) y_memalign(64, newCount * sizeof(rkdTreeNode<T>));
		memcpy(n, nodes, allocatedNodesCount * sizeof(rkdTreeNode<T>));
		y_free(nodes);
		nodes = n;
		allocatedNodesCount = newCount;
	}	

#if _TRI_CLIP > 0
	if(nPrims <= TRI_CLIP_THRESH)
	{
//		exBound_t ebound(nodeBound);
		int oPrims[TRI_CLIP_THRESH], nOverl=0;
		double bHalfSize[3];
		double b_ext[2][3];
		for(int i=0; i<3; ++i)
		{
//			bCenter[i]   = ((double)nodeBound.a[i] + (double)nodeBound.g[i])*0.5;
			bHalfSize[i] = ((double)nodeBound.g[i] - (double)nodeBound.a[i]);
			double temp  = ((double)treeBound.g[i] - (double)treeBound.a[i]);
			b_ext[0][i] = nodeBound.a[i] - 0.021*bHalfSize[i] - 0.00001*temp;
			b_ext[1][i] = nodeBound.g[i] + 0.021*bHalfSize[i] + 0.00001*temp;
//			ebound.halfSize[i] *= 1.01;
		}
		char *c_old = cdata + (TRI_CLIP_THRESH * CLIP_DATA_SIZE * depth);
		char *c_new = cdata + (TRI_CLIP_THRESH * CLIP_DATA_SIZE * (depth+1));
		for(unsigned int i=0; i<nPrims; ++i)
		{
			const T *ct = prims[ primNums[i] ];
			u_int32 old_idx=0;
			if(clip[depth] >= 0) old_idx = primNums[i+nPrims];
//			if(old_idx > TRI_CLIP_THRESH){ std::cout << "ouch!\n"; }
//			std::cout << "parent idx: " << old_idx << std::endl;
			if(ct->clippingSupport())
			{
				if( ct->clipToBound(b_ext, clip[depth], allBounds[totalPrims+nOverl],
					c_old + old_idx*CLIP_DATA_SIZE, c_new + nOverl*CLIP_DATA_SIZE) )
				{
					++_clip;
					oPrims[nOverl] = primNums[i]; nOverl++;
				}
				else ++_null_clip;
			}
			else
			{
				// no clipping supported by prim, copy old bound:
				allBounds[totalPrims+nOverl] = allBounds[ primNums[i] ]; //really??
				oPrims[nOverl] = primNums[i]; nOverl++;
			}
		}
		//copy back
		memcpy(primNums, oPrims, nOverl*sizeof(u_int32));
		nPrims = nOverl;
	}
#endif
	//	<< check if leaf criteria met >>
	if(nPrims <= maxLeafSize || depth >= maxDepth)
	{
//		std::cout << "leaf\n";
		nodes[nextFreeNode].createLeaf(primNums, nPrims, prims, primsArena);
		nextFreeNode++;
		if( depth >= maxDepth ) depthLimitReached++; //stat
		return 0;
	}
	
	//<< calculate cost for all axes and chose minimum >>
	splitCost_t split;
	float baseBonus=eBonus;
	eBonus *= 1.1 - (float)depth/(float)maxDepth;
	if(nPrims > 128) pigeonMinCost(nPrims, nodeBound, primNums, split);
#if _TRI_CLIP > 0
	else if (nPrims > TRI_CLIP_THRESH) minimalCost(nPrims, nodeBound, primNums, allBounds, edges, split);
	else minimalCost(nPrims, nodeBound, primNums, allBounds+totalPrims, edges, split);
#else
	else minimalCost(nPrims, nodeBound, primNums, allBounds, edges, split);
#endif
	eBonus=baseBonus; //restore eBonus
	//<< if (minimum > leafcost) increase bad refines >>
	if (split.bestCost > split.oldCost) ++badRefines;
	if ((split.bestCost > 1.6f * split.oldCost && nPrims < 16) ||
		split.bestAxis == -1 || badRefines == 2) {
		nodes[nextFreeNode].createLeaf(primNums, nPrims, prims, primsArena);
		nextFreeNode++;
		if( badRefines == 2) ++NumBadSplits; //stat
		return 0;
	}
	
	//todo: check working memory for child recursive calls
	u_int32 remainingMem, *morePrims = nullptr, *nRightPrims;
	u_int32 *oldRightPrims = rightPrims;
	if(nPrims > rightMemSize || 2*TRI_CLIP_THRESH > rightMemSize ) // *possibly* not enough, get some more
	{
//		std::cout << "buildTree: more memory allocated!\n";
		remainingMem = nPrims * 3;
		morePrims = new u_int32[remainingMem];
		nRightPrims = morePrims;
	}
	else
	{
		nRightPrims = oldRightPrims;
		remainingMem = rightMemSize;
	}
	
	// Classify primitives with respect to split
	PFLOAT splitPos;
	int n0 = 0, n1 = 0;
	if(nPrims > 128) // we did pigeonhole
	{
		int pn;
		for (unsigned int i=0; i<nPrims; i++)
		{
			pn = primNums[i];
			if(allBounds[ pn ].a[split.bestAxis] >= split.t) nRightPrims[n1++] = pn;
			else
			{
				leftPrims[n0++] = pn;
				if(allBounds[ pn ].g[split.bestAxis] > split.t) nRightPrims[n1++] = pn;
			}
		}
		splitPos = split.t;
		if (n0!= split.nBelow || n1 != split.nAbove) std::cout << "oops!\n";
/*		static int foo=0;
		if(foo<10)
		{
			std::cout << "best axis:"<<split.bestAxis<<", rel. split:" <<(split.t - nodeBound.a[split.bestAxis])/(nodeBound.g[split.bestAxis]-nodeBound.a[split.bestAxis]);
			std::cout << "\nleft Prims:"<<n0<<", right Prims:"<<n1<<", total Prims:"<<nPrims<<" (level:"<<depth<<")\n";
			foo++;
		}*/
	}
	else if(nPrims <= TRI_CLIP_THRESH)
	{
		int cindizes[TRI_CLIP_THRESH];
		u_int32 oldPrims[TRI_CLIP_THRESH];
//		std::cout << "memcpy\n";
		memcpy(oldPrims, primNums, nPrims*sizeof(int));
		
		for (int i=0; i<split.bestOffset; ++i)
			if (edges[split.bestAxis][i].end != UPPER_B)
			{
				cindizes[n0] = edges[split.bestAxis][i].primNum;
//				if(cindizes[n0] >nPrims) std::cout << "error!!\n";
				leftPrims[n0] = oldPrims[cindizes[n0]];
				++n0;
			}
//		std::cout << "append n0\n";
		for(int i=0; i<n0; ++i){ leftPrims[n0+i] = cindizes[i]; /* std::cout << cindizes[i] << " "; */ }
		if (edges[split.bestAxis][split.bestOffset].end == BOTH_B)
		{
			cindizes[n1] = edges[split.bestAxis][split.bestOffset].primNum;
//			if(cindizes[n1] >nPrims) std::cout << "error!!\n";
			nRightPrims[n1] = oldPrims[cindizes[n1]];
			++n1;
		}
		for (int i=split.bestOffset+1; i<split.nEdge; ++i)
			if (edges[split.bestAxis][i].end != LOWER_B)
			{
				cindizes[n1] = edges[split.bestAxis][i].primNum;
//				if(cindizes[n1] >nPrims) std::cout << "error!!\n";
				nRightPrims[n1] = oldPrims[cindizes[n1]];
				++n1;
			}
//		std::cout << "\nappend n1\n";
		for(int i=0; i<n1; ++i){ nRightPrims[n1+i] = cindizes[i]; /* std::cout << cindizes[i] << " "; */ }
		splitPos = edges[split.bestAxis][split.bestOffset].pos;
//		std::cout << "\ndone\n";
	}
	else //we did "normal" cost function
	{	
		for (int i=0; i<split.bestOffset; ++i)
			if (edges[split.bestAxis][i].end != UPPER_B)
				leftPrims[n0++] = edges[split.bestAxis][i].primNum;
		if (edges[split.bestAxis][split.bestOffset].end == BOTH_B)
			nRightPrims[n1++] = edges[split.bestAxis][split.bestOffset].primNum;
		for (int i=split.bestOffset+1; i<split.nEdge; ++i)
			if (edges[split.bestAxis][i].end != LOWER_B)
				nRightPrims[n1++] = edges[split.bestAxis][i].primNum;
		splitPos = edges[split.bestAxis][split.bestOffset].pos;
	}
//	std::cout << "split axis: " << split.bestAxis << ", pos: " << splitPos << "\n";
	//advance right prims pointer
	remainingMem -= n1;
	
	
	u_int32 curNode = nextFreeNode;
	nodes[curNode].createInterior(split.bestAxis, splitPos);
	++nextFreeNode;
	bound_t boundL = nodeBound, boundR = nodeBound;
	switch(split.bestAxis){
		case 0: boundL.setMaxX(splitPos); boundR.setMinX(splitPos); break;
		case 1: boundL.setMaxY(splitPos); boundR.setMinY(splitPos); break;
		case 2: boundL.setMaxZ(splitPos); boundR.setMinZ(splitPos); break;
	}

#if _TRI_CLIP > 0
	if(nPrims <= TRI_CLIP_THRESH)
	{
		remainingMem -= n1;
		//<< recurse below child >>
		clip[depth+1] = split.bestAxis;
		buildTree(n0, boundL, leftPrims, leftPrims, nRightPrims+2*n1, edges, remainingMem, depth+1, badRefines);
		clip[depth+1] |= 1<<2;
		//<< recurse above child >>
		nodes[curNode].setRightChild (nextFreeNode);
		buildTree(n1, boundR, nRightPrims, leftPrims, nRightPrims+2*n1, edges, remainingMem, depth+1, badRefines);
		clip[depth+1] = -1;
	}
	else
	{
#endif
		//<< recurse below child >>
		buildTree(n0, boundL, leftPrims, leftPrims, nRightPrims+n1, edges, remainingMem, depth+1, badRefines);
		//<< recurse above child >>
		nodes[curNode].setRightChild (nextFreeNode);
		buildTree(n1, boundR, nRightPrims, leftPrims, nRightPrims+n1, edges, remainingMem, depth+1, badRefines);
#if _TRI_CLIP > 0
	}
#endif
	// free additional working memory, if present
	if(morePrims) delete[] morePrims;
	return 1;
}
	


//============================
/*! The standard intersect function,
	returns the closest hit within dist
*/
template<class T>
bool kdTree_t<T>::Intersect(const ray_t &ray, PFLOAT dist, T **tr, PFLOAT &Z, intersectData_t &data) const
{
	Z=dist;
	
	PFLOAT a, b, t; // entry/exit/splitting plane signed distance
	PFLOAT t_hit;
	
	if (!treeBound.cross(ray, a, b, dist))
	{ return false; }
	
	intersectData_t currentData, tempData;
	vector3d_t invDir(1.0/ray.dir.x, 1.0/ray.dir.y, 1.0/ray.dir.z); //was 1.f!
//	int rayId = curMailboxId++;
	bool hit = false;
	
	rKdStack<T> stack[KD_MAX_STACK];
	const rkdTreeNode<T> *farChild, *currNode;
	currNode = nodes;
	
	int enPt = 0;
	stack[enPt].t = a;
	
	//distinguish between internal and external origin
	if(a >= 0.0) // ray with external origin
		stack[enPt].pb = ray.from + ray.dir * a;
	else // ray with internal origin
		stack[enPt].pb = ray.from;
	
	// setup initial entry and exit poimt in stack
	int exPt = 1; // pointer to stack
	stack[exPt].t = b;
	stack[exPt].pb = ray.from + ray.dir * b;
	stack[exPt].node = nullptr; // "nowhere", termination flag
	
	//loop, traverse kd-Tree until object intersection or ray leaves tree bound
	while (currNode != nullptr)
	{
		if (dist < stack[enPt].t) break;
		// loop until leaf is found
		while( !currNode->IsLeaf() )
		{
			int axis = currNode->SplitAxis();
			PFLOAT splitVal = currNode->SplitPos();
			
			if(stack[enPt].pb[axis] <= splitVal){
				if(stack[exPt].pb[axis] <= splitVal)
				{
					currNode++;
					continue;
				}
				if(stack[exPt].pb[axis] == splitVal)
				{
					currNode = &nodes[currNode->getRightChild()];
					continue;
				}
				// case N4
				farChild = &nodes[currNode->getRightChild()];
				currNode ++;
			}
			else
			{
				if(splitVal < stack[exPt].pb[axis])
				{
					currNode = &nodes[currNode->getRightChild()];
					continue;
				}
				farChild = currNode+1;
				currNode = &nodes[currNode->getRightChild()];
			}
			// traverse both children
			
			t = (splitVal - ray.from[axis]) * invDir[axis];
			
			// setup the new exit point
			int tmp = exPt;
			exPt++;
			
			// possibly skip current entry point so not to overwrite the data
			if (exPt == enPt) exPt++;
			// push values onto the stack //todo: lookup table
			static const int npAxis[2][3] = { {1, 2, 0}, {2, 0, 1} };
			int nextAxis = npAxis[0][axis];//(axis+1)%3;
			int prevAxis = npAxis[1][axis];//(axis+2)%3;
			stack[exPt].prev = tmp;
			stack[exPt].t = t;
			stack[exPt].node = farChild;
			stack[exPt].pb[axis] = splitVal;
			stack[exPt].pb[nextAxis] = ray.from[nextAxis] + t * ray.dir[nextAxis];
			stack[exPt].pb[prevAxis] = ray.from[prevAxis] + t * ray.dir[prevAxis];
		}
				 
		u_int32 nPrimitives = currNode->nPrimitives();
		if (nPrimitives == 1)
		{
			T *mp = currNode->onePrimitive;
			if (mp->intersect(ray, &t_hit, tempData))
			{
				if(t_hit < Z && t_hit >= ray.tmin)
				{
					Z = t_hit;
					*tr = mp;
					currentData = tempData;
					hit = true;
				}
			}
		}
		else
		{
			T **prims = currNode->primitives;
			for (u_int32 i = 0; i < nPrimitives; ++i) {
				T *mp = prims[i];
				if (mp->intersect(ray, &t_hit, tempData))
				{
					if(t_hit < Z && t_hit >= ray.tmin)
					{
						Z = t_hit;
						*tr = mp;
						currentData = tempData;
						hit = true;
					}
				}
			}
		}
		
		if(hit && Z <= stack[exPt].t)
		{
			data = currentData;
			return true;
		}
		
		enPt = exPt;
		currNode = stack[exPt].node;
		exPt = stack[enPt].prev;
				
	} // while

	data = currentData;
	
	return hit;
}

template<class T>
bool kdTree_t<T>::IntersectS(const ray_t &ray, PFLOAT dist, T **tr, PFLOAT shadow_bias) const
{
	PFLOAT a, b, t; // entry/exit/splitting plane signed distance
	PFLOAT t_hit;
	
	if (!treeBound.cross(ray, a, b, dist))
		return false;
	
	intersectData_t bary;
	vector3d_t invDir(1.f/ray.dir.x, 1.f/ray.dir.y, 1.f/ray.dir.z);
	
	rKdStack<T> stack[KD_MAX_STACK];
	const rkdTreeNode<T> *farChild, *currNode;
	currNode = nodes;
	
	int enPt = 0;
	stack[enPt].t = a;
	
	//distinguish between internal and external origin
	if(a >= 0.0) // ray with external origin
		stack[enPt].pb = ray.from + ray.dir * a;
	else // ray with internal origin
		stack[enPt].pb = ray.from;
	
	// setup initial entry and exit poimt in stack
	int exPt = 1; // pointer to stack
	stack[exPt].t = b;
	stack[exPt].pb = ray.from + ray.dir * b;
	stack[exPt].node = nullptr; // "nowhere", termination flag
	
	//loop, traverse kd-Tree until object intersection or ray leaves tree bound
	while (currNode != nullptr)
	{
		if (dist < stack[enPt].t /*a*/) break;
		// loop until leaf is found
		while( !currNode->IsLeaf() )
		{
			int axis = currNode->SplitAxis();
			PFLOAT splitVal = currNode->SplitPos();
			
			if(stack[enPt].pb[axis] <= splitVal){
				if(stack[exPt].pb[axis] <= splitVal)
				{
					currNode++;
					continue;
				}
				if(stack[exPt].pb[axis] == splitVal)
				{
					currNode = &nodes[currNode->getRightChild()];
					continue;
				}
				// case N4
				farChild = &nodes[currNode->getRightChild()];
				currNode ++;
			}
			else
			{
				if(splitVal < stack[exPt].pb[axis])
				{
					currNode = &nodes[currNode->getRightChild()];
					continue;
				}
				farChild = currNode+1;
				currNode = &nodes[currNode->getRightChild()];
			}
			// traverse both children
			
			t = (splitVal - ray.from[axis]) * invDir[axis];
			
			// setup the new exit point
			int tmp = exPt;
			exPt++;
			
			// possibly skip current entry point so not to overwrite the data
			if (exPt == enPt) exPt++;
			// push values onto the stack //todo: lookup table
			static const int npAxis[2][3] = { {1, 2, 0}, {2, 0, 1} };
			int nextAxis = npAxis[0][axis];//(axis+1)%3;
			int prevAxis = npAxis[1][axis];//(axis+2)%3;
			stack[exPt].prev = tmp;
			stack[exPt].t = t;
			stack[exPt].node = farChild;
			stack[exPt].pb[axis] = splitVal;
			stack[exPt].pb[nextAxis] = ray.from[nextAxis] + t * ray.dir[nextAxis];
			stack[exPt].pb[prevAxis] = ray.from[prevAxis] + t * ray.dir[prevAxis];
		}
				 
		// Check for intersections inside leaf node
		u_int32 nPrimitives = currNode->nPrimitives();
		if (nPrimitives == 1)
		{
			T *mp = currNode->onePrimitive;
			if (mp->intersect(ray, &t_hit, bary))
			{
				if(t_hit < dist && t_hit > ray.tmin )
				{
					*tr = mp;
					return true;
				}
			}
		}
		else
		{
			T **prims = currNode->primitives;
			for (u_int32 i = 0; i < nPrimitives; ++i)
			{
				T *mp = prims[i];
					if (mp->intersect(ray, &t_hit, bary))
					{
						if(t_hit < dist && t_hit > ray.tmin )
						{
							*tr = mp;
							return true;
						}
					}
			}
		}
		
		enPt = exPt;
		currNode = stack[exPt].node;
		exPt = stack[enPt].prev;
				
	} // while

	return false;
}

/*=============================================================
	allow for transparent shadows.
=============================================================*/

template<class T>
bool kdTree_t<T>::IntersectTS(renderState_t &state, const ray_t &ray, int maxDepth, PFLOAT dist, T **tr, color_t &filt, PFLOAT shadow_bias) const
{
	PFLOAT a, b, t; // entry/exit/splitting plane signed distance
	PFLOAT t_hit;
	
	if (!treeBound.cross(ray, a, b, dist))
		return false;
	
	intersectData_t bary;
	vector3d_t invDir(1.f/ray.dir.x, 1.f/ray.dir.y, 1.f/ray.dir.z);

	int depth=0;
#if ( HAVE_PTHREAD && defined (__GNUC__)  && !defined (__clang__))
	std::set<const T *, std::less<const T *>, __gnu_cxx::__mt_alloc<const T *> > filtered;
#else
	std::set<const T *> filtered;
#endif
	rKdStack<T> stack[KD_MAX_STACK];
	const rkdTreeNode<T> *farChild, *currNode;
	currNode = nodes;
	
	int enPt = 0;
	stack[enPt].t = a;
	
	//distinguish between internal and external origin
	if(a >= 0.0) // ray with external origin
		stack[enPt].pb = ray.from + ray.dir * a;
	else // ray with internal origin
		stack[enPt].pb = ray.from;
	
	// setup initial entry and exit poimt in stack
	int exPt = 1; // pointer to stack
	stack[exPt].t = b;
	stack[exPt].pb = ray.from + ray.dir * b;
	stack[exPt].node = nullptr; // "nowhere", termination flag
	
	//loop, traverse kd-Tree until object intersection or ray leaves tree bound
	while (currNode != nullptr)
	{
		if (dist < stack[enPt].t /*a*/) break;
		// loop until leaf is found
		while( !currNode->IsLeaf() )
		{
			int axis = currNode->SplitAxis();
			PFLOAT splitVal = currNode->SplitPos();
			
			if(stack[enPt].pb[axis] <= splitVal){
				if(stack[exPt].pb[axis] <= splitVal)
				{
					currNode++;
					continue;
				}
				if(stack[exPt].pb[axis] == splitVal)
				{
					currNode = &nodes[currNode->getRightChild()];
					continue;
				}
				// case N4
				farChild = &nodes[currNode->getRightChild()];
				currNode ++;
			}
			else
			{
				if(splitVal < stack[exPt].pb[axis])
				{
					currNode = &nodes[currNode->getRightChild()];
					continue;
				}
				farChild = currNode+1;
				currNode = &nodes[currNode->getRightChild()];
			}
			// traverse both children
			
			t = (splitVal - ray.from[axis]) * invDir[axis];
			
			// setup the new exit point
			int tmp = exPt;
			exPt++;
			
			// possibly skip current entry point so not to overwrite the data
			if (exPt == enPt) exPt++;
			// push values onto the stack //todo: lookup table
			static const int npAxis[2][3] = { {1, 2, 0}, {2, 0, 1} };
			int nextAxis = npAxis[0][axis];//(axis+1)%3;
			int prevAxis = npAxis[1][axis];//(axis+2)%3;
			stack[exPt].prev = tmp;
			stack[exPt].t = t;
			stack[exPt].node = farChild;
			stack[exPt].pb[axis] = splitVal;
			stack[exPt].pb[nextAxis] = ray.from[nextAxis] + t * ray.dir[nextAxis];
			stack[exPt].pb[prevAxis] = ray.from[prevAxis] + t * ray.dir[prevAxis];
		}
				 
		// Check for intersections inside leaf node
		u_int32 nPrimitives = currNode->nPrimitives();
		if (nPrimitives == 1)
		{
			T *mp = currNode->onePrimitive;
			if (mp->intersect(ray, &t_hit, bary))
			{
				if(t_hit < dist && t_hit >= ray.tmin )
				{
					const material_t *mat = mp->getMaterial();
					if(!mat->isTransparent() ) return true;
					if(filtered.insert(mp).second)
					{
						if(depth>=maxDepth) return true;
						point3d_t h=ray.from + t_hit*ray.dir;
						surfacePoint_t sp;
						mp->getSurface(sp, h, bary);
						filt *= mat->getTransparency(state, sp, ray.dir);
						++depth;
					}
				}
			}
		}
		else 
		{
			T **prims = currNode->primitives;
			for (u_int32 i = 0; i < nPrimitives; ++i) {
				T *mp = prims[i];
				if (mp->intersect(ray, &t_hit, bary))
				{
					if(t_hit < dist && t_hit >= ray.tmin )
					{
						const material_t *mat = mp->getMaterial();
						if(!mat->isTransparent() ) return true;
						if(filtered.insert(mp).second)
						{
							if(depth>=maxDepth) return true;
							point3d_t h=ray.from + t_hit*ray.dir;
							surfacePoint_t sp;
							mp->getSurface(sp, h, bary);
							filt *= mat->getTransparency(state, sp, ray.dir);
							++depth;
						}
					}
				}
			}
		}
		
		enPt = exPt;
		currNode = stack[exPt].node;
		exPt = stack[enPt].prev;
				
	} // while

	return false;
}

// explicit instantiation of template:
template class kdTree_t<triangle_t>;
template class kdTree_t<primitive_t>;

__END_YAFRAY
