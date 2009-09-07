//#include <rply.h>
#include "rply-1.01/rply.h"
#include <iostream>

#include <core_api/scene.h>

using namespace::yafray;

struct ply_dat_t
{
	scene_t *s;
	point3d_t p;
	material_t *mat;
	int idx[3];
	double scale;
};

extern "C"
{

static int vertex_cb(p_ply_argument argument) {
	long coord;
	ply_dat_t *dat;
	ply_get_argument_user_data(argument, (void**)&dat, &coord);
	//printf("%g", ply_get_argument_value(argument));
	double scale = dat->scale;
	switch(coord)
	{
		case 0: dat->p.x = scale*ply_get_argument_value(argument); break;
		case 1: dat->p.z = scale*ply_get_argument_value(argument); break;
		case 2: dat->p.y = scale*ply_get_argument_value(argument);
				dat->s->addVertex( dat->p ); break;
	}
	return 1;
}

static int face_cb(p_ply_argument argument) {
    long length, value_index;
   	ply_dat_t *dat;
   	ply_get_argument_user_data(argument, (void**)&dat, NULL);
    ply_get_argument_property(argument, NULL, &length, &value_index);
    switch (value_index) {
        case 0: dat->idx[0] = (int)ply_get_argument_value(argument); break;
        case 1: dat->idx[1] = (int)ply_get_argument_value(argument); break;
        case 2: dat->idx[2] = (int)ply_get_argument_value(argument);
        	dat->s->addTriangle(dat->idx[0], dat->idx[1], dat->idx[2], dat->mat);
            break;
        default: 
            break;
    }
    return 1;
}

}

bool loadPly(scene_t *s, material_t *mat, const char *plyfile, double scale)
{
	long nvertices, ntriangles;
	bool success=false;
	p_ply ply = ply_open(plyfile, NULL);
	if (!ply) return false;
	if (!ply_read_header(ply))
	{
		ply_close(ply);
		return false;
	}
	ply_dat_t dat = { s, point3d_t(0.0), mat, {0, 0, 0}, scale };
	nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, &dat, 0);
	ply_set_read_cb(ply, "vertex", "y", vertex_cb, &dat, 1);
	ply_set_read_cb(ply, "vertex", "z", vertex_cb, &dat, 2);
	ntriangles = ply_set_read_cb(ply, "face", "vertex_indices", face_cb, &dat, 0);
	
	objID_t id;
	if(s->startTriMesh(id,nvertices, ntriangles,false,false))
	{
		success = (ply_read(ply)) ? true : false;
		s->endTriMesh();
	}
	ply_close(ply);
	return success;
}


