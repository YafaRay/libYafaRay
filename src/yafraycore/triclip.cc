/*
Sutherland-Hodgman triangle clipping
*/
#include<yafray_config.h>

/*
class bound_t
{
	public:
	bound_t(){};
	float a[3];
	float g[3];
};
*/

#include <core_api/bound.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include <iostream>

__BEGIN_YAFRAY

template <class T>
void _swap(T **a, T **b)
{
	T *x;
	x=*a;
	*a=*b;
	*b=x;
};
	
class DVector
{
	public:
		DVector()/*: x(0.0), y(0.0), z(0.0)*/ {};
		DVector& operator=(const DVector &v){x=v.x, y=v.y, z=v.z; return *this;};
		double   operator[] (int i) const{ return (&x)[i]; };
		double  &operator[] (int i) { return (&x)[i]; };
		~DVector(){};
		double x,y,z;
};

struct clipDump
{
	int nverts;
	DVector poly[10];
};

//bool triBoxClipDebug(const double b_min[3], const double b_max[3], const double triverts[3][3], bound_t &box);

#define Y_VCPY(a,b)       ( (a)[0] =(b)[0], (a)[1] =(b)[1], (a)[2] =(b)[2] )

/*! function to clip a triangle against an axis aligned bounding box and return new bound
	\param box the AABB of the clipped triangle
	\return 0: triangle was clipped successfully
			1: triangle didn't overlap the bound at all => disappeared
			2: fatal error occured (didn't ever happen to me :)
			3: resulting polygon degenerated to less than 3 edges (never happened either)
*/

int triBoxClip(const double b_min[3], const double b_max[3], const double triverts[3][3], bound_t &box, void* n_dat)
{
	DVector dump1[11], dump2[11]; double t;
	DVector *poly = dump1, *cpoly = dump2;
	//for(int q=0;q<9;q++)poly[q/3][q%3]=triverts[q/3][q%3];
	for(int q=0;q<3;q++)
	{
		poly[q][0]=triverts[q][0], poly[q][1]=triverts[q][1], poly[q][2]=triverts[q][2];
		poly[3][q]=triverts[0][q];
	}
	int n=3, nc;
	bool p1_inside;
	//for each axis
	for(int axis=0;axis<3;axis++)
	{
		DVector *p1, *p2;
		int nextAxis = (axis+1)%3, prevAxis = (axis+2)%3;
		
		// === clip lower ===
		nc=0;
		p1_inside = (poly[0][axis] >= b_min[axis]) ? true : false;
		for(int i=0; i<n; i++) // for each poly edge
		{
			p1=&poly[i], p2=&poly[i+1];
			if( p1_inside ) // p1 inside
			{
				if((*p2)[axis] >= b_min[axis]) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					t = (b_min[axis] - (*p1)[axis]) / ((*p2)[axis]-(*p1)[axis]);
					cpoly[nc][axis] = b_min[axis];
					cpoly[nc][nextAxis] = (*p1)[nextAxis] + t * ((*p2)[nextAxis]-(*p1)[nextAxis]);
					cpoly[nc][prevAxis] = (*p1)[prevAxis] + t * ((*p2)[prevAxis]-(*p1)[prevAxis]);
					nc++;
					p1_inside = false;
				}
			}
			else //p1 < b_min -> outside
			{
				if((*p2)[axis] > b_min[axis]) //p2 inside, add s and p2
				{
					//t = (b_min[axis] - p1[axis]) / (p2[axis]-p1[axis]);
					t = (b_min[axis] - (*p2)[axis]) / ((*p1)[axis]-(*p2)[axis]);
					cpoly[nc][axis] = b_min[axis];
					//cpoly[nc][nextAxis] = p1[nextAxis] + t * (p2[nextAxis]-p1[nextAxis]);
					//cpoly[nc][prevAxis] = p1[prevAxis] + t * (p2[prevAxis]-p1[prevAxis]);
					cpoly[nc][nextAxis] = (*p2)[nextAxis] + t * ((*p1)[nextAxis]-(*p2)[nextAxis]);
					cpoly[nc][prevAxis] = (*p2)[prevAxis] + t * ((*p1)[prevAxis]-(*p2)[prevAxis]);
					nc++;
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else if((*p2)[axis] == b_min[axis]) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else p1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges
		if(nc>9){std::cout<<"after min n is now "<<nc<<", that's bad!\n"; return 2;}
		cpoly[nc] = cpoly[0];
		n = nc;
		//std::swap(cpoly, poly);
		_swap(&cpoly, &poly);
		
		
		// === clip upper ===
		nc=0;
		p1_inside = (poly[0][axis] <= b_max[axis]) ? true : false;
		for(int i=0; i<n; i++) // for each poly edge
		{
			p1=&poly[i], p2=&poly[i+1];
			if( p1_inside )
			{
				if((*p2)[axis] <= b_max[axis]) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					t = (b_max[axis] - (*p1)[axis]) / ((*p2)[axis]-(*p1)[axis]);
					cpoly[nc][axis] = b_max[axis];
					cpoly[nc][nextAxis] = (*p1)[nextAxis] + t * ((*p2)[nextAxis]-(*p1)[nextAxis]);
					cpoly[nc][prevAxis] = (*p1)[prevAxis] + t * ((*p2)[prevAxis]-(*p1)[prevAxis]);
					nc++;
					p1_inside = false;
				}
			}
			else //p1 > b_max -> outside
			{
				if((*p2)[axis] < b_max[axis]) //p2 inside, add s and p2
				{
					//t = (b_max[axis] - p1[axis]) / (p2[axis]-p1[axis]);
					t = (b_max[axis] - (*p2)[axis]) / ((*p1)[axis]-(*p2)[axis]);
					cpoly[nc][axis] = b_max[axis];
					//cpoly[nc][nextAxis] = p1[nextAxis] + t * (p2[nextAxis]-p1[nextAxis]);
					//cpoly[nc][prevAxis] = p1[prevAxis] + t * (p2[prevAxis]-p1[prevAxis]);
					cpoly[nc][nextAxis] = (*p2)[nextAxis] + t * ((*p1)[nextAxis]-(*p2)[nextAxis]);
					cpoly[nc][prevAxis] = (*p2)[prevAxis] + t * ((*p1)[prevAxis]-(*p2)[prevAxis]);
					nc++;
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else if((*p2)[axis] == b_max[axis]) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else p1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges
		if(nc>9){std::cout<<"after max n is now "<<nc<<", that's bad!\n"; return 2;}
		if(nc==0) return 1;
	//	Y_VCPY(cpoly[nc], cpoly[0]);
		cpoly[nc] = cpoly[0];
		n = nc;
		
//		for(int k=0;k<=nc;k++){ std::cout << "#"<<k<<" point: "<<cpoly[k][0]<<", "<<cpoly[k][1]<<", "<<cpoly[k][2]<<"\n"; }
//		for(int k=0;k<=nc;k++){ printf("*%dpoint: %2.7f, %2.7f, %2.7f\n", k, cpoly[k][0], cpoly[k][1], cpoly[k][2]); }
//		std::cout << "!"<<nc<<" point: "<<cpoly[nc][0]<<", "<<cpoly[nc][1]<<", "<<cpoly[nc][2]<<"\n";
//		std::cout << "!"<<0 <<" point: "<<cpoly[0][0]<<", "<<cpoly[0][1]<<", "<<cpoly[0][2]<<"\n";
		
		//std::swap(cpoly, poly);
		_swap(&cpoly, &poly);
	} //for all axis
//	std::cout<<"n is now "<<n<<"\n";
	
	if(n<2){
		static bool foobar=false;
		if(foobar) return 3;
		std::cout << "clip degenerated! n="<<n<<"\n";
		std::cout << "b_min:\t"<<b_min[0]<<",\t"<<b_min[1]<<",\t"<<b_min[2]<<"\n";
		std::cout << "b_max:\t"<<b_max[0]<<",\t"<<b_max[1]<<",\t"<<b_max[2]<<"\n";
		std::cout << "delta:\t"<<b_max[0]-b_min[0]<<",\t"<<b_max[1]-b_min[1]<<",\t"<<b_max[2]-b_min[2]<<"\n";
		for(int j=0;j<3;j++)
		{
			std::cout << "point"<<j<<": "<<
				triverts[j][0]<<",\t"<<triverts[j][1]<<",\t"<<triverts[j][2]<<"\n";
		}
		foobar=true;
		return 3;
	}
	double a[3], g[3];
	Y_VCPY(a, poly[0]);
	Y_VCPY(g, poly[0]);
	for(int i=1; i<n; i++)
	{
		a[0] = std::min(a[0], poly[i][0]);
		a[1] = std::min(a[1], poly[i][1]);
		a[2] = std::min(a[2], poly[i][2]);
		g[0] = std::max(g[0], poly[i][0]);
		g[1] = std::max(g[1], poly[i][1]);
		g[2] = std::max(g[2], poly[i][2]);
	}
	box.a[0] = a[0], box.g[0] = g[0];
	box.a[1] = a[1], box.g[1] = g[1];
	box.a[2] = a[2], box.g[2] = g[2];
	
	clipDump *output = (clipDump*)n_dat;
	output->nverts = n;
	memcpy(output->poly, poly, (n+1)*sizeof(DVector));
	
	return 0;
}

int triPlaneClip(double pos, int axis, bool lower, bound_t &box, void* o_dat, void* n_dat)
{
	clipDump *input = (clipDump*)o_dat;
	clipDump *output = (clipDump*)n_dat;
//	DVector dump1[11], dump2[11];
	double t;
	DVector *poly = input->poly/* dump1 */, *cpoly = output->poly/* dump2 */;
	int n=input->nverts, nc;
//	memcpy(poly, input->poly, (n+1)*sizeof(DVector));
	
	bool p1_inside;
	DVector *p1, *p2;
	int nextAxis = (axis+1)%3, prevAxis = (axis+2)%3;
	
	if(lower)
	{
		// === clip lower ===
		nc=0;
		p1_inside = (poly[0][axis] >= pos) ? true : false;
		for(int i=0; i<n; i++) // for each poly edge
		{
			p1=&poly[i], p2=&poly[i+1];
			if( p1_inside ) // p1 inside
			{
				if((*p2)[axis] >= pos) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					t = (pos - (*p1)[axis]) / ((*p2)[axis]-(*p1)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][nextAxis] = (*p1)[nextAxis] + t * ((*p2)[nextAxis]-(*p1)[nextAxis]);
					cpoly[nc][prevAxis] = (*p1)[prevAxis] + t * ((*p2)[prevAxis]-(*p1)[prevAxis]);
					nc++;
					p1_inside = false;
				}
			}
			else //p1 < b_min -> outside
			{
				if((*p2)[axis] > pos) //p2 inside, add s and p2
				{
					t = (pos - (*p2)[axis]) / ((*p1)[axis]-(*p2)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][nextAxis] = (*p2)[nextAxis] + t * ((*p1)[nextAxis]-(*p2)[nextAxis]);
					cpoly[nc][prevAxis] = (*p2)[prevAxis] + t * ((*p1)[prevAxis]-(*p2)[prevAxis]);
					nc++;
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else if((*p2)[axis] == pos) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else p1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges
		if(nc==0) return 1;
		if(nc>9){std::cout<<"after min n is now "<<nc<<", that's bad!\n"; return 2;}
		cpoly[nc] = cpoly[0];
		n = nc;
		//std::swap(cpoly, poly);
		_swap(&cpoly, &poly);
	}
	else
	{
		// === clip upper ===
		nc=0;
		p1_inside = (poly[0][axis] <= pos) ? true : false;
		for(int i=0; i<n; i++) // for each poly edge
		{
			p1=&poly[i], p2=&poly[i+1];
			if( p1_inside )
			{
				if((*p2)[axis] <= pos) //both "inside"
				{
					// copy p2 to new poly
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else
				{
					// clip line, add intersection to new poly
					t = (pos - (*p1)[axis]) / ((*p2)[axis]-(*p1)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][nextAxis] = (*p1)[nextAxis] + t * ((*p2)[nextAxis]-(*p1)[nextAxis]);
					cpoly[nc][prevAxis] = (*p1)[prevAxis] + t * ((*p2)[prevAxis]-(*p1)[prevAxis]);
					nc++;
					p1_inside = false;
				}
			}
			else //p1 > pos -> outside
			{
				if((*p2)[axis] < pos) //p2 inside, add s and p2
				{
					t = (pos - (*p2)[axis]) / ((*p1)[axis]-(*p2)[axis]);
					cpoly[nc][axis] = pos;
					cpoly[nc][nextAxis] = (*p2)[nextAxis] + t * ((*p1)[nextAxis]-(*p2)[nextAxis]);
					cpoly[nc][prevAxis] = (*p2)[prevAxis] + t * ((*p1)[prevAxis]-(*p2)[prevAxis]);
					nc++;
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else if((*p2)[axis] == pos) //p2 and s are identical, only add p2
				{
					cpoly[nc] = *p2;
					nc++;
					p1_inside = true;
				}
				else p1_inside = false;
			}
			//else: both outse, do nothing.
		} //for all edges
		if(nc==0) return 1;
		if(nc>9){std::cout<<"after max n is now "<<nc<<", that's bad!\n"; return 2;}
	//	Y_VCPY(cpoly[nc], cpoly[0]);
		cpoly[nc] = cpoly[0];
		n = nc;
				
		//std::swap(cpoly, poly);
		_swap(&cpoly, &poly);
	} //for all axis
	
	if(n<2){
		static bool foobar=false;
		if(foobar) return 3;
		std::cout << "clip degenerated! n="<<n<<"\n";
		foobar=true;
		return 3;
	}
	double a[3], g[3];
	Y_VCPY(a, poly[0]);
	Y_VCPY(g, poly[0]);
	for(int i=1; i<n; i++)
	{
		a[0] = std::min(a[0], poly[i][0]);
		a[1] = std::min(a[1], poly[i][1]);
		a[2] = std::min(a[2], poly[i][2]);
		g[0] = std::max(g[0], poly[i][0]);
		g[1] = std::max(g[1], poly[i][1]);
		g[2] = std::max(g[2], poly[i][2]);
	}
	box.a[0] = a[0], box.g[0] = g[0];
	box.a[1] = a[1], box.g[1] = g[1];
	box.a[2] = a[2], box.g[2] = g[2];
	
	output->nverts = n;
//	memcpy(output->poly, poly, (n+1)*sizeof(DVector));
	
	return 0;
}

/*
int triBoxClip(const double b_min[3], const double b_max[3], const double triverts[3][3], bound_t &box)
{
	double dump[20][3], t;
	double (*poly)[3] = dump, (*cpoly)[3] = dump+10;
	memcpy(poly, triverts, 9*sizeof(double) );
	memcpy(&poly[3][0], triverts, 3*sizeof(double) );
	int n=3, nc;
	//for each axis
	for(int axis=0;axis<3;axis++)
	{
		double *p1, *p2;
		int nextAxis = (axis+1)%3, prevAxis = (axis+2)%3;
		
//		std::cout << "\taxis: "<< axis<<", b_min:"<<b_min[axis]<<", b_max"<<b_max[axis]<<"\n";
		// === clip lower ===
		nc=0;
		for(int i=0; i<n; i++) // for each poly edge
		{
			p1=&poly[i][0], p2=&poly[i+1][0];
			if(p1[axis] >= b_min[axis]) // p1 inside
			{
				if(p2[axis] >= b_min[axis]) //both "inside"
				{
					// copy p2 to new poly
//					std::cout << "edge: "<<i<<" is inside!\n";
					Y_VCPY( cpoly[nc], p2);
					nc++;
				}
				else
				{
					// clip line, add intersection to new poly
//					std::cout << "edge: "<<i<<" goes out!\n";
					t = (b_min[axis] - p1[axis]) / (p2[axis]-p1[axis]);
					cpoly[nc][axis] = b_min[axis];
					cpoly[nc][nextAxis] = p1[nextAxis] + t * (p2[nextAxis]-p1[nextAxis]);
					cpoly[nc][prevAxis] = p1[prevAxis] + t * (p2[prevAxis]-p1[prevAxis]);
//					std::cout << " new point: "<<cpoly[nc][0]<<", "<<cpoly[nc][1]<<", "<<cpoly[nc][2]<<"\n";
					nc++;
				}
			}
			else //p1 < b_min -> outside
			{
				if(p2[axis] > b_min[axis]) //p2 inside, add s and p2
				{
//					std::cout << "edge: "<<i<<" goes in!\n";
					//t = (b_min[axis] - p1[axis]) / (p2[axis]-p1[axis]);
					t = (b_min[axis] - p2[axis]) / (p1[axis]-p2[axis]);
					cpoly[nc][axis] = b_min[axis];
					//cpoly[nc][nextAxis] = p1[nextAxis] + t * (p2[nextAxis]-p1[nextAxis]);
					//cpoly[nc][prevAxis] = p1[prevAxis] + t * (p2[prevAxis]-p1[prevAxis]);
					cpoly[nc][nextAxis] = p2[nextAxis] + t * (p1[nextAxis]-p2[nextAxis]);
					cpoly[nc][prevAxis] = p2[prevAxis] + t * (p1[prevAxis]-p2[prevAxis]);
//					std::cout << " new point: "<<cpoly[nc][0]<<", "<<cpoly[nc][1]<<", "<<cpoly[nc][2]<<"\n";
					nc++;
					Y_VCPY(cpoly[nc], p2);
					nc++;
				}
				else if(p2[axis] == b_min[axis]) //p2 and s are identical, only add p2
				{
//					std::cout << "edge: "<<i<<" touches in!\n";
					Y_VCPY(cpoly[nc], p2);
					nc++;
				}
//				else std::cout << "edge: "<<i<<" lies outside!\n";
			}
			//else: both outse, do nothing.
		} //for all edges
		if(nc>9){std::cout<<"after min n is now "<<nc<<", that's bad!\n";return 2;}
		Y_VCPY(cpoly[nc], cpoly[0]);
		n = nc;
		std::swap(cpoly, poly);
		
		
		// === clip upper ===
		nc=0;
		for(int i=0; i<n; i++) // for each poly edge
		{
			p1=&poly[i][0], p2=&poly[i+1][0];
			if(p1[axis] <= b_max[axis])
			{
				if(p2[axis] <= b_max[axis]) //both "inside"
				{
					// copy p2 to new poly
					Y_VCPY( cpoly[nc], p2);
					nc++;
				}
				else
				{
					// clip line, add intersection to new poly
					t = (b_max[axis] - p1[axis]) / (p2[axis]-p1[axis]);
					cpoly[nc][axis] = b_max[axis];
					cpoly[nc][nextAxis] = p1[nextAxis] + t * (p2[nextAxis]-p1[nextAxis]);
					cpoly[nc][prevAxis] = p1[prevAxis] + t * (p2[prevAxis]-p1[prevAxis]);
					nc++;
				}
			}
			else //p1 > b_max -> outside
			{
				if(p2[axis] < b_max[axis]) //p2 inside, add s and p2
				{
					//t = (b_max[axis] - p1[axis]) / (p2[axis]-p1[axis]);
					t = (b_max[axis] - p2[axis]) / (p1[axis]-p2[axis]);
					cpoly[nc][axis] = b_max[axis];
					//cpoly[nc][nextAxis] = p1[nextAxis] + t * (p2[nextAxis]-p1[nextAxis]);
					//cpoly[nc][prevAxis] = p1[prevAxis] + t * (p2[prevAxis]-p1[prevAxis]);
					cpoly[nc][nextAxis] = p2[nextAxis] + t * (p1[nextAxis]-p2[nextAxis]);
					cpoly[nc][prevAxis] = p2[prevAxis] + t * (p1[prevAxis]-p2[prevAxis]);
					nc++;
					Y_VCPY(cpoly[nc], p2);
					nc++;
				}
				else if(p2[axis] == b_max[axis]) //p2 and s are identical, only add p2
				{
					Y_VCPY(cpoly[nc], p2);
					nc++;
				}
			}
			//else: both outse, do nothing.
		} //for all edges
		if(nc>9){std::cout<<"after max n is now "<<nc<<", that's bad!\n"; return 2;}
		Y_VCPY(cpoly[nc], cpoly[0]);
		n = nc;
		std::swap(cpoly, poly);
	} //for all axis
//	std::cout<<"n is now "<<n<<"\n";
	
	if(n<2){
		static int foobar=0;
		if(foobar>1) return 1;
		std::cout << "\n\n//clip degenerated! n="<<n<<"\n";
		std::cout << "//b_min:\t"<<b_min[0]<<",\t"<<b_min[1]<<",\t"<<b_min[2]<<"\n";
		std::cout << "//b_max:\t"<<b_max[0]<<",\t"<<b_max[1]<<",\t"<<b_max[2]<<"\n";
		std::cout << "//delta:\t"<<b_max[0]-b_min[0]<<",\t"<<b_max[1]-b_min[1]<<",\t"<<b_max[2]-b_min[2]<<"\n";
		for(int j=0;j<3;j++)
		{
			std::cout << "//point"<<j<<": "<<
				triverts[j][0]<<",\t"<<triverts[j][1]<<",\t"<<triverts[j][2]<<"\n";
		}
		triBoxClipDebug(b_min, b_max, triverts, box);
		foobar++;
		return 1 ;
	}
	double a[3], g[3];
	Y_VCPY(a, poly[0]);
	Y_VCPY(g, poly[0]);
	for(int i=1; i<n; i++)
	{
		a[0] = std::min(a[0], poly[i][0]);
		a[1] = std::min(a[1], poly[i][1]);
		a[2] = std::min(a[2], poly[i][2]);
		g[0] = std::max(g[0], poly[i][0]);
		g[1] = std::max(g[1], poly[i][1]);
		g[2] = std::max(g[2], poly[i][2]);
	}
	box.a[0] = a[0], box.g[0] = g[0];
	box.a[1] = a[1], box.g[1] = g[1];
	box.a[2] = a[2], box.g[2] = g[2];
	
//	for(int j=0;j<n;j++)
//		std::cout << "cpoint"<<j<<": " <<poly[j][0]<<" "<<poly[j][1]<<" "<<poly[j][2]<<"\n";
	return 0;
}
*/
/*
int main()
{
	double a[3] = { -0.5, -0.5, -0.5}, g[3] = { 0.5, 0.5, 0.5};
	double tri[3][3] = { {-1.02, 0.52, -0.24}, {0.18, -0.8, 0.84}, {0.76, 0.0, -0.44} };
	bound_t bbox;
	bool ok = triBoxClip(a, g, tri, bbox);
	if(!ok) std::cout << "didn't work!\n";
	std::cout << "point1: " <<bbox.a[0]<<" "<<bbox.a[1]<<" "<<bbox.a[2]<<"\n";
	std::cout << "point2: " <<bbox.g[0]<<" "<<bbox.g[1]<<" "<<bbox.g[2]<<"\n";
}
*/

__END_YAFRAY
