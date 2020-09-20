/****************************************************************************
 *      This is part of the libYafaRay package
 *
 *      Sutherland-Hodgman triangle clipping
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
#include "constants.h"

#include "common/bound.h"
#include "common/logging.h"
#include <string.h>

BEGIN_YAFARAY

template <class T>
void swap__(T **a, T **b)
{
	T *x;
	x = *a;
	*a = *b;
	*b = x;
}

class DVector
{
	public:
		DVector() {}
		DVector &operator = (const DVector &v) { x_ = v.x_, y_ = v.y_, z_ = v.z_; return *this; }
		double   operator[](int i) const { return (&x_)[i]; }
		double  &operator[](int i) { return (&x_)[i]; }
		~DVector() {}
		double x_, y_, z_;
};

struct ClipDump
{
	int nverts_;
	DVector poly_[10];
};

#define Y_VCPY(a,b)       ( (a)[0] =(b)[0], (a)[1] =(b)[1], (a)[2] =(b)[2] )

/*! function to clip a triangle against an axis aligned bounding box and return new bound
	\param box the AABB of the clipped triangle
	\return 0: triangle was clipped successfully
			1: triangle didn't overlap the bound at all => disappeared
			2: fatal error occured (didn't ever happen to me :)
			3: resulting polygon degenerated to less than 3 edges (never happened either)
*/

int triBoxClip__(const double b_min[3], const double b_max[3], const double triverts[3][3], Bound &box, void *n_dat)
{
	DVector dump_1[11], dump_2[11]; double t;
	DVector *poly = dump_1, *cpoly = dump_2;

	for(int q = 0; q < 3; q++)
	{
		poly[q][0] = triverts[q][0], poly[q][1] = triverts[q][1], poly[q][2] = triverts[q][2];
		poly[3][q] = triverts[0][q];
	}

	int n = 3, nc;
	bool p_1_inside;

	//for each axis
	for(int axis = 0; axis < 3; axis++)
	{
		DVector *p_1, *p_2;
		int next_axis = (axis + 1) % 3, prev_axis = (axis + 2) % 3;

		// === clip lower ===
		nc = 0;
		p_1_inside = (poly[0][axis] >= b_min[axis]) ? true : false;
		for(int i = 0; i < n; i++) // for each poly edge
		{
			p_1 = &poly[i], p_2 = &poly[i + 1];
			if(p_1_inside)   // p1 inside
			{
				if((*p_2)[axis] >= b_min[axis]) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					t = (b_min[axis] - (*p_1)[axis]) / ((*p_2)[axis] - (*p_1)[axis]);
					cpoly[nc][axis] = b_min[axis];
					cpoly[nc][next_axis] = (*p_1)[next_axis] + t * ((*p_2)[next_axis] - (*p_1)[next_axis]);
					cpoly[nc][prev_axis] = (*p_1)[prev_axis] + t * ((*p_2)[prev_axis] - (*p_1)[prev_axis]);
					nc++;
					p_1_inside = false;
				}
			}
			else //p1 < b_min -> outside
			{
				if((*p_2)[axis] > b_min[axis]) //p2 inside, add s and p2
				{
					t = (b_min[axis] - (*p_2)[axis]) / ((*p_1)[axis] - (*p_2)[axis]);
					cpoly[nc][axis] = b_min[axis];
					cpoly[nc][next_axis] = (*p_2)[next_axis] + t * ((*p_1)[next_axis] - (*p_2)[next_axis]);
					cpoly[nc][prev_axis] = (*p_2)[prev_axis] + t * ((*p_1)[prev_axis] - (*p_2)[prev_axis]);
					nc++;
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else if((*p_2)[axis] == b_min[axis]) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else p_1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges

		if(nc > 9)
		{
			Y_VERBOSE << "TriangleClip: after min n is now " << nc << ", that's bad!" << YENDL;
			return 2;
		}

		cpoly[nc] = cpoly[0];
		n = nc;
		swap__(&cpoly, &poly);


		// === clip upper ===
		nc = 0;
		p_1_inside = (poly[0][axis] <= b_max[axis]) ? true : false;
		for(int i = 0; i < n; i++) // for each poly edge
		{
			p_1 = &poly[i], p_2 = &poly[i + 1];
			if(p_1_inside)
			{
				if((*p_2)[axis] <= b_max[axis]) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					t = (b_max[axis] - (*p_1)[axis]) / ((*p_2)[axis] - (*p_1)[axis]);
					cpoly[nc][axis] = b_max[axis];
					cpoly[nc][next_axis] = (*p_1)[next_axis] + t * ((*p_2)[next_axis] - (*p_1)[next_axis]);
					cpoly[nc][prev_axis] = (*p_1)[prev_axis] + t * ((*p_2)[prev_axis] - (*p_1)[prev_axis]);
					nc++;
					p_1_inside = false;
				}
			}
			else //p1 > b_max -> outside
			{
				if((*p_2)[axis] < b_max[axis]) //p2 inside, add s and p2
				{
					t = (b_max[axis] - (*p_2)[axis]) / ((*p_1)[axis] - (*p_2)[axis]);
					cpoly[nc][axis] = b_max[axis];
					cpoly[nc][next_axis] = (*p_2)[next_axis] + t * ((*p_1)[next_axis] - (*p_2)[next_axis]);
					cpoly[nc][prev_axis] = (*p_2)[prev_axis] + t * ((*p_1)[prev_axis] - (*p_2)[prev_axis]);
					nc++;
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else if((*p_2)[axis] == b_max[axis]) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else p_1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges

		if(nc > 9)
		{
			Y_VERBOSE << "TriangleClip: After max n is now " << nc << ", that's bad!" << YENDL;
			return 2;
		}

		if(nc == 0) return 1;

		cpoly[nc] = cpoly[0];
		n = nc;
		swap__(&cpoly, &poly);
	} //for all axis

	if(n < 2)
	{
		static bool foobar = false;
		if(foobar) return 3;
		Y_VERBOSE << "TriangleClip: Clip degenerated! n=" << n << YENDL;
		Y_VERBOSE << "TriangleClip: b_min:\t" << b_min[0] << ",\t" << b_min[1] << ",\t" << b_min[2] << YENDL;
		Y_VERBOSE << "TriangleClip: b_max:\t" << b_max[0] << ",\t" << b_max[1] << ",\t" << b_max[2] << YENDL;
		Y_VERBOSE << "TriangleClip: delta:\t" << b_max[0] - b_min[0] << ",\t" << b_max[1] - b_min[1] << ",\t" << b_max[2] - b_min[2] << YENDL;

		for(int j = 0; j < 3; j++)
		{
			Y_VERBOSE << "TriangleClip: point" << j << ": " << triverts[j][0] << ",\t" << triverts[j][1] << ",\t" << triverts[j][2] << YENDL;
		}
		foobar = true;
		return 3;
	}

	double a[3], g[3];
	Y_VCPY(a, poly[0]);
	Y_VCPY(g, poly[0]);

	for(int i = 1; i < n; i++)
	{
		a[0] = std::min(a[0], poly[i][0]);
		a[1] = std::min(a[1], poly[i][1]);
		a[2] = std::min(a[2], poly[i][2]);
		g[0] = std::max(g[0], poly[i][0]);
		g[1] = std::max(g[1], poly[i][1]);
		g[2] = std::max(g[2], poly[i][2]);
	}

	box.a_[0] = a[0], box.g_[0] = g[0];
	box.a_[1] = a[1], box.g_[1] = g[1];
	box.a_[2] = a[2], box.g_[2] = g[2];

	ClipDump *output = (ClipDump *)n_dat;
	output->nverts_ = n;
	memcpy(output->poly_, poly, (n + 1) * sizeof(DVector));

	return 0;
}

int triPlaneClip__(double pos, int axis, bool lower, Bound &box, void *o_dat, void *n_dat)
{
	ClipDump *input = (ClipDump *)o_dat;
	ClipDump *output = (ClipDump *)n_dat;
	double t;
	DVector *poly = input->poly_, *cpoly = output->poly_;
	int n = input->nverts_, nc;

	bool p_1_inside;
	DVector *p_1, *p_2;
	int next_axis = (axis + 1) % 3, prev_axis = (axis + 2) % 3;

	if(lower)
	{
		// === clip lower ===
		nc = 0;
		p_1_inside = (poly[0][axis] >= pos) ? true : false;
		for(int i = 0; i < n; i++) // for each poly edge
		{
			p_1 = &poly[i], p_2 = &poly[i + 1];
			if(p_1_inside)   // p1 inside
			{
				if((*p_2)[axis] >= pos) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					t = (pos - (*p_1)[axis]) / ((*p_2)[axis] - (*p_1)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][next_axis] = (*p_1)[next_axis] + t * ((*p_2)[next_axis] - (*p_1)[next_axis]);
					cpoly[nc][prev_axis] = (*p_1)[prev_axis] + t * ((*p_2)[prev_axis] - (*p_1)[prev_axis]);
					nc++;
					p_1_inside = false;
				}
			}
			else //p1 < b_min -> outside
			{
				if((*p_2)[axis] > pos) //p2 inside, add s and p2
				{
					t = (pos - (*p_2)[axis]) / ((*p_1)[axis] - (*p_2)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][next_axis] = (*p_2)[next_axis] + t * ((*p_1)[next_axis] - (*p_2)[next_axis]);
					cpoly[nc][prev_axis] = (*p_2)[prev_axis] + t * ((*p_1)[prev_axis] - (*p_2)[prev_axis]);
					nc++;
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else if((*p_2)[axis] == pos) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else p_1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges

		if(nc == 0) return 1;

		if(nc > 9)
		{
			Y_VERBOSE << "TriangleClip: After min n is now " << nc << ", that's bad!" << YENDL;
			return 2;
		}

		cpoly[nc] = cpoly[0];
		n = nc;
		swap__(&cpoly, &poly);
	}
	else
	{
		// === clip upper ===
		nc = 0;
		p_1_inside = (poly[0][axis] <= pos) ? true : false;
		for(int i = 0; i < n; i++) // for each poly edge
		{
			p_1 = &poly[i], p_2 = &poly[i + 1];
			if(p_1_inside)
			{
				if((*p_2)[axis] <= pos) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					t = (pos - (*p_1)[axis]) / ((*p_2)[axis] - (*p_1)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][next_axis] = (*p_1)[next_axis] + t * ((*p_2)[next_axis] - (*p_1)[next_axis]);
					cpoly[nc][prev_axis] = (*p_1)[prev_axis] + t * ((*p_2)[prev_axis] - (*p_1)[prev_axis]);
					nc++;
					p_1_inside = false;
				}
			}
			else //p1 > pos -> outside
			{
				if((*p_2)[axis] < pos) //p2 inside, add s and p2
				{
					t = (pos - (*p_2)[axis]) / ((*p_1)[axis] - (*p_2)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][next_axis] = (*p_2)[next_axis] + t * ((*p_1)[next_axis] - (*p_2)[next_axis]);
					cpoly[nc][prev_axis] = (*p_2)[prev_axis] + t * ((*p_1)[prev_axis] - (*p_2)[prev_axis]);
					nc++;
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else if((*p_2)[axis] == pos) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p_2;
					nc++;
					p_1_inside = true;
				}
				else p_1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges

		if(nc == 0) return 1;

		if(nc > 9)
		{
			Y_VERBOSE << "TriangleClip: after max n is now " << nc << ", that's bad!" << YENDL;
			return 2;
		}

		cpoly[nc] = cpoly[0];
		n = nc;
		swap__(&cpoly, &poly);
	} //for all axis

	if(n < 2)
	{
		static bool foobar = false;
		if(foobar) return 3;
		Y_VERBOSE << "TriangleClip: Clip degenerated! n=" << n << YENDL;
		foobar = true;
		return 3;
	}

	double a[3], g[3];
	Y_VCPY(a, poly[0]);
	Y_VCPY(g, poly[0]);

	for(int i = 1; i < n; i++)
	{
		a[0] = std::min(a[0], poly[i][0]);
		a[1] = std::min(a[1], poly[i][1]);
		a[2] = std::min(a[2], poly[i][2]);
		g[0] = std::max(g[0], poly[i][0]);
		g[1] = std::max(g[1], poly[i][1]);
		g[2] = std::max(g[2], poly[i][2]);
	}

	box.a_[0] = a[0], box.g_[0] = g[0];
	box.a_[1] = a[1], box.g_[1] = g[1];
	box.a_[2] = a[2], box.g_[2] = g[2];

	output->nverts_ = n;

	return 0;
}

END_YAFARAY

