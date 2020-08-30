/********************************************************/
/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
/* Function: int triBoxOverlap(float boxcenter[3],      */
/*          float boxhalfsize[3],float triverts[3][3]); */
/* History:                                             */
/*   2001-03-05: released the code in its first version */
/*   2001-06-18: changed the order of the tests, faster */
/*                                                      */
/* Acknowledgement: Many thanks to Pierre Terdiman for  */
/* suggestions and discussions on how to optimize code. */
/* Thanks to David Hunt for finding a ">="-bug!         */
/********************************************************/
#include <cmath>
#include <stdio.h>
// #include <pbrt.h"

#include "constants.h"

BEGIN_YAFARAY


#define X 0
#define Y 1
#define Z 2

#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

#define SUB(dest,v1,v2) \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2];

#define FINDMINMAX(x0,x1,x2,min,max) \
  min = max = x0;   \
  if(x1<min) min=x1;\
  if(x1>max) max=x1;\
  if(x2<min) min=x2;\
  if(x2>max) max=x2;

int planeBoxOverlap__(double *normal, double *vert, double *maxbox)	// -NJMP-
{
	int q;
	double vmin[3], vmax[3], v;
	for(q = X; q <= Z; q++)
	{
		v = vert[q];					// -NJMP-
		if(normal[q] > 0.0f)
		{
			vmin[q] = -maxbox[q] - v;	// -NJMP-
			vmax[q] = maxbox[q] - v;	// -NJMP-
		}
		else
		{
			vmin[q] = maxbox[q] - v;	// -NJMP-
			vmax[q] = -maxbox[q] - v;	// -NJMP-
		}
	}
	if(DOT(normal, vmin) > 0.0f) return 0;	// -NJMP-
	if(DOT(normal, vmax) >= 0.0f) return 1;	// -NJMP-

	return 0;
}


/*======================== X-tests ========================*/
#define AXISTEST_X_01(a, b, fa, fb)			   \
	p_0 = a*v0[Y] - b*v0[Z];			       	   \
	p_2 = a*v2[Y] - b*v2[Z];			       	   \
        if(p_0<p_2) {min=p_0; max=p_2;} else {min=p_2; max=p_0;} \
	rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_X_2(a, b, fa, fb)			   \
	p_0 = a*v0[Y] - b*v0[Z];			           \
	p_1 = a*v1[Y] - b*v1[Z];			       	   \
        if(p_0<p_1) {min=p_0; max=p_1;} else {min=p_1; max=p_0;} \
	rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
	if(min>rad || max<-rad) return 0;

/*======================== Y-tests ========================*/
#define AXISTEST_Y_02(a, b, fa, fb)			   \
	p_0 = -a*v0[X] + b*v0[Z];		      	   \
	p_2 = -a*v2[X] + b*v2[Z];	       	       	   \
        if(p_0<p_2) {min=p_0; max=p_2;} else {min=p_2; max=p_0;} \
	rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_Y_1(a, b, fa, fb)			   \
	p_0 = -a*v0[X] + b*v0[Z];		      	   \
	p_1 = -a*v1[X] + b*v1[Z];	     	       	   \
        if(p_0<p_1) {min=p_0; max=p_1;} else {min=p_1; max=p_0;} \
	rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
	if(min>rad || max<-rad) return 0;

/*======================== Z-tests ========================*/

#define AXISTEST_Z_12(a, b, fa, fb)			   \
	p_1 = a*v1[X] - b*v1[Y];			           \
	p_2 = a*v2[X] - b*v2[Y];			       	   \
        if(p_2<p_1) {min=p_2; max=p_1;} else {min=p_1; max=p_2;} \
	rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
	if(min>rad || max<-rad) return 0;

#define AXISTEST_Z_0(a, b, fa, fb)			   \
	p_0 = a*v0[X] - b*v0[Y];				   \
	p_1 = a*v1[X] - b*v1[Y];			           \
        if(p_0<p_1) {min=p_0; max=p_1;} else {min=p_1; max=p_0;} \
	rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
	if(min>rad || max<-rad) return 0;

int triBoxOverlap__(double *boxcenter, double *boxhalfsize, double **triverts)
{

	/*    use separating axis theorem to test overlap between triangle and box */
	/*    need to test for overlap in these directions: */
	/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
	/*       we do not even need to test these) */
	/*    2) normal of the triangle */
	/*    3) crossproduct(edge from tri, {x,y,z}-directin) */
	/*       this gives 3x3=9 more tests */
	double v0[3], v1[3], v2[3];
	//   double axis[3];
	double min, max, p_0, p_1, p_2, rad, fex, fey, fez;		// -NJMP- "d" local variable removed
	double normal[3], e0[3], e1[3], e2[3];

	/* This is the fastest branch on Sun */
	/* move everything so that the boxcenter is in (0,0,0) */
	SUB(v0, triverts[0], boxcenter);
	SUB(v1, triverts[1], boxcenter);
	SUB(v2, triverts[2], boxcenter);

	/* compute triangle edges */
	SUB(e0, v1, v0);    /* tri edge 0 */
	SUB(e1, v2, v1);    /* tri edge 1 */
	SUB(e2, v0, v2);    /* tri edge 2 */

	/* Bullet 3:  */
	/*  test the 9 tests first (this was faster) */
	fex = std::fabs(e0[X]);
	fey = std::fabs(e0[Y]);
	fez = std::fabs(e0[Z]);
	AXISTEST_X_01(e0[Z], e0[Y], fez, fey);
	AXISTEST_Y_02(e0[Z], e0[X], fez, fex);
	AXISTEST_Z_12(e0[Y], e0[X], fey, fex);

	fex = std::fabs(e1[X]);
	fey = std::fabs(e1[Y]);
	fez = std::fabs(e1[Z]);
	AXISTEST_X_01(e1[Z], e1[Y], fez, fey);
	AXISTEST_Y_02(e1[Z], e1[X], fez, fex);
	AXISTEST_Z_0(e1[Y], e1[X], fey, fex);

	fex = std::fabs(e2[X]);
	fey = std::fabs(e2[Y]);
	fez = std::fabs(e2[Z]);
	AXISTEST_X_2(e2[Z], e2[Y], fez, fey);
	AXISTEST_Y_1(e2[Z], e2[X], fez, fex);
	AXISTEST_Z_12(e2[Y], e2[X], fey, fex);

	/* Bullet 1: */
	/*  first test overlap in the {x,y,z}-directions */
	/*  find min, max of the triangle each direction, and test for overlap in */
	/*  that direction -- this is equivalent to testing a minimal AABB around */
	/*  the triangle against the AABB */

	/* test in X-direction */
	FINDMINMAX(v0[X], v1[X], v2[X], min, max);
	if(min > boxhalfsize[X] || max < -boxhalfsize[X]) return 0;

	/* test in Y-direction */
	FINDMINMAX(v0[Y], v1[Y], v2[Y], min, max);
	if(min > boxhalfsize[Y] || max < -boxhalfsize[Y]) return 0;

	/* test in Z-direction */
	FINDMINMAX(v0[Z], v1[Z], v2[Z], min, max);
	if(min > boxhalfsize[Z] || max < -boxhalfsize[Z]) return 0;

	/* Bullet 2: */
	/*  test if the box intersects the plane of the triangle */
	/*  compute plane equation of triangle: normal*x+d=0 */
	CROSS(normal, e0, e1);
	// -NJMP- (line removed here)
	if(!planeBoxOverlap__(normal, v0, boxhalfsize)) return 0;	// -NJMP-

	return 1;   /* box and triangle overlaps */
}

END_YAFARAY
