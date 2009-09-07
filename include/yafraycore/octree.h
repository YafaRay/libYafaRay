
#ifndef Y_OCTREE_H
#define Y_OCTREE_H

#include <core_api/bound.h>
#include <iostream>

__BEGIN_YAFRAY

template <class NodeData> struct octNode_t
{
	octNode_t() {
		for (int i = 0; i < 8; ++i)	children[i] = 0;
	}
	~octNode_t() {
		for (int i = 0; i < 8; ++i)	delete children[i];
	}
	octNode_t *children[8];
	std::vector<NodeData> data;
};

template <class NodeData> class octree_t
{
public:
	octree_t(const bound_t &b, int md = 16)	: treeBound(b)
	{
		maxDepth = md;
	}
	void add(const NodeData &dat, const bound_t &bound)
	{
		recursiveAdd(&root, treeBound, dat, bound,
			(bound.a - bound.g).lengthSqr() );
	}
	template <class LookupProc>
	void lookup(const point3d_t &p, LookupProc &process)
	{
		if (!treeBound.includes(p)) return;
		recursiveLookup(&root, treeBound, p, process);
	}
private:
	void recursiveAdd(octNode_t<NodeData> *node, const bound_t &nodeBound,
		const NodeData &dataItem, const bound_t &dataBound, float diag2,
		int depth = 0);
	template <class LookupProc>
	void recursiveLookup(octNode_t<NodeData> *node, const bound_t &nodeBound, const point3d_t &P,
			LookupProc &process);
	// octree_t Private Data
	int maxDepth;
	bound_t treeBound;
	octNode_t<NodeData> root;
};

// octree_t Method Definitions
template <class NodeData>
void octree_t<NodeData>::recursiveAdd(
		octNode_t<NodeData> *node, const bound_t &nodeBound,
		const NodeData &dataItem, const bound_t &dataBound,
		float diag2, int depth)
{
	// Possibly add data item to current octree node
	if( (nodeBound.a - nodeBound.g).lengthSqr() < diag2 || depth == maxDepth )
	{
		node->data.push_back(dataItem);
		return;
	}
	// Otherwise add data item to octree children
	point3d_t center = nodeBound.center();
	// Determine which children the item overlaps
	bool over[8];
	over[1] = over[3] = over[5] = over[7] = (dataBound.a.x <= center.x);
	over[0] = over[2] = over[4] = over[6] = (dataBound.g.x  > center.x);
	if(dataBound.a.y > center.y)  over[2] = over[3] = over[6] = over[7] = false;
	if(dataBound.g.y <= center.y) over[0] = over[1] = over[4] = over[5] = false;
	if(dataBound.a.z > center.z)  over[4] = over[5] = over[6] = over[7] = false;
	if(dataBound.g.z <= center.z) over[0] = over[1] = over[2] = over[3] = false;
	
	for (int child = 0; child < 8; ++child)
	{
		if (!over[child]) continue;
		if (!node->children[child])
			node->children[child] = new octNode_t<NodeData>;
		// Compute _childBound_ for octree child _child_
		bound_t childBound;
		childBound.a.x = (child & 1) ? nodeBound.a.x : center.x;
		childBound.g.x = (child & 1) ? center.x : nodeBound.g.x;
		childBound.a.y = (child & 2) ? nodeBound.a.y : center.y;
		childBound.g.y = (child & 2) ? center.y : nodeBound.g.y;
		childBound.a.z = (child & 4) ? nodeBound.a.z : center.z;
		childBound.g.z = (child & 4) ? center.z : nodeBound.g.z;
		recursiveAdd(node->children[child], childBound,
		           dataItem, dataBound, diag2, depth+1);
	}
}

template <class NodeData> template <class LookupProc>
void octree_t<NodeData>::recursiveLookup(
		octNode_t<NodeData> *node, const bound_t &nodeBound,
		const point3d_t &p, LookupProc &process)
{
	for (unsigned int i = 0; i < node->data.size(); ++i)
		if( ! process(p, node->data[i]) ) return;
	// Determine which octree child node _p_ is inside
	point3d_t center = nodeBound.center();
	int child = (p.x > center.x ? 0 : 1) +
				(p.y > center.y ? 0 : 2) + 
				(p.z > center.z ? 0 : 4);
	if (node->children[child])
	{
		// Compute _childBound_ for octree child _child_
		bound_t childBound;
		childBound.a.x = (child & 1) ? nodeBound.a.x : center.x;
		childBound.g.x = (child & 1) ? center.x : nodeBound.g.x;
		childBound.a.y = (child & 2) ? nodeBound.a.y : center.y;
		childBound.g.y = (child & 2) ? center.y : nodeBound.g.y;
		childBound.a.z = (child & 4) ? nodeBound.a.z : center.z;
		childBound.g.z = (child & 4) ? center.z : nodeBound.g.z;
		recursiveLookup(node->children[child], childBound, p, process);
	}
}

__END_YAFRAY

#endif // Y_OCTREE_H
