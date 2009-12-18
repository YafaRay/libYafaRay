/****************************************************************************
 * 			xmlparser.cc: a libXML based parser for YafRay scenes
 *      This is part of the yafray package
 *      Copyright (C) 2006  Mathias Wein
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

#include <yafraycore/xmlparser.h>
#include <core_api/environment.h>
#include <core_api/scene.h>
#if HAVE_XML
#include <libxml/parser.h>
#endif
#include <cstring>


__BEGIN_YAFRAY

#if HAVE_XML
void startDocument(void *user_data)
{
	//std::cout<< "Start Docunent"<< std::endl;
}

void endDocument(void *user_data)
{
	//std::cout<< "End Docunent"<< std::endl;
}

void startElement(void * user_data, const xmlChar * name, const xmlChar **attrs)
{
	xmlParser_t &parser = *((xmlParser_t *)user_data);
	parser.startElement((const char *)name, (const char **)attrs);
}

void endElement(void *user_data, const xmlChar *name)
{
	xmlParser_t &parser = *((xmlParser_t *)user_data);
	parser.endElement((const char *)name);
}

static void my_warning(void *user_data, const char *msg, ...) 
{
    va_list args;

    va_start(args, msg);
    printf(msg, args);
    va_end(args);
}

static void my_error(void *user_data, const char *msg, ...) 
{
    va_list args;

    va_start(args, msg);
    printf(msg, args);
    va_end(args);
}

static void my_fatalError(void *user_data, const char *msg, ...) 
{
    va_list args;

    va_start(args, msg);
    printf(msg, args);
    va_end(args);
}

static xmlSAXHandler my_handler =
{
	NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  startDocument, //  startDocumentSAXFunc startDocument;
  endDocument, //  endDocumentSAXFunc endDocument;
  startElement, //  startElementSAXFunc startElement;
  endElement, //  endElementSAXFunc endElement;
  NULL,
  NULL, //  charactersSAXFunc characters;
  NULL,
  NULL,
  NULL,
  my_warning,
  my_error,
  my_fatalError
};
#endif // HAVE_XML

bool parse_xml_file(const char *filename, scene_t *scene, renderEnvironment_t *env, paraMap_t &render)
{
#if HAVE_XML
	xmlParser_t parser(env, scene, render);
	if (xmlSAXUserParseFile(&my_handler, &parser, filename) < 0)
	{
		std::cout << "Error parsing the file " << filename << std::endl;
		return false;
	}
	return true;
#else
	std::cout << "Warning, yafray was compiled without XML support, cannot parse file.\n";
	return false;
#endif
}

#if HAVE_XML
/*=============================================================
/ parser functions
=============================================================*/

xmlParser_t::xmlParser_t(renderEnvironment_t *renv, scene_t *sc, paraMap_t &r):
	env(renv), scene(sc), render(r), current(0), level(0)
{
	cparams = &params;
	pushState(startEl_document, endEl_document);
}

void xmlParser_t::pushState(startElement_cb start, endElement_cb end, void *userdata)
{
	parserState_t state;
	state.start = start;
	state.end = end;
	state.userdata = userdata;
	state.level = level;
	state_stack.push_back(state);
	current = &state_stack.back();
}

void xmlParser_t::popState()
{
	state_stack.pop_back();
	if(!state_stack.empty()) current = &state_stack.back();
	else current = 0;
}

/*=============================================================
/ utility functions...
=============================================================*/

inline bool str2bool(const char *s){ return strcmp(s, "true") ? false : true; }

static bool parsePoint(const char **attrs, point3d_t &p, point3d_t &op)
{
	for( ;attrs && attrs[0]; attrs += 2)
	{
		if(attrs[0][0] == 'o')
		{
			if(attrs[0][1] == 0 || attrs[0][2] != 0)
			{
				std::cout << "Ignored wrong attr " << attrs[0] << " in point (1)"<< std::endl;
				continue; //it is not a single character
			}
			switch(attrs[0][1])
			{
				case 'x' : op.x = atof(attrs[1]); break;
				case 'y' : op.y = atof(attrs[1]); break;
				case 'z' : op.z = atof(attrs[1]); break;
				default: std::cout << "Ignored wrong attr " << attrs[0] << " in point (2)"<< std::endl;
			}
			continue;
		}
		else if(attrs[0][1] != 0)
		{
			std::cout << "Ignored wrong attr " << attrs[0] << " in point (3)"<< std::endl;
			continue; //it is not a single character
		}
		switch(attrs[0][0])
		{
			case 'x' : p.x = atof(attrs[1]); break;
			case 'y' : p.y = atof(attrs[1]); break;
			case 'z' : p.z = atof(attrs[1]); break;
			default: std::cout << "Ignored wrong attr " << attrs[0] << " in point"<< std::endl;
		}
	}
	//cout << "GOT point "<< p <<endl;
	return true;
}

void parseParam(const char **attrs, parameter_t &param)
{
	if(!attrs[0]) return;
	if(!attrs[2]) // only one attribute => bool, integer or float value
	{
		std::string name(attrs[0]);
		if(name == "ival"){ int i = atoi(attrs[1]); param = parameter_t(i); return; }
		else if(name == "fval"){ double f = atof(attrs[1]); param = parameter_t(f); return; }
		else if(name == "bval"){ bool b = str2bool(attrs[1]); param = parameter_t(b); return; }
		else if(name == "sval"){ param = parameter_t(std::string(attrs[1])); return; }
	}
	colorA_t c(0.f); point3d_t p(0,0,0);
	int type=TYPE_NONE;
	for(int n=0; attrs[n]; ++n)
	{
		if(attrs[n][1] != '\0') continue;
		switch(attrs[n][0])
		{
			case 'x': p.x = atof(attrs[n+1]); type = TYPE_POINT; break;
			case 'y': p.y = atof(attrs[n+1]); type = TYPE_POINT; break;
			case 'z': p.z = atof(attrs[n+1]); type = TYPE_POINT; break;
			
			case 'r': c.R = (CFLOAT)atof(attrs[n+1]); type = TYPE_COLOR; break;
			case 'g': c.G = (CFLOAT)atof(attrs[n+1]); type = TYPE_COLOR; break;
			case 'b': c.B = (CFLOAT)atof(attrs[n+1]); type = TYPE_COLOR; break;
			case 'a': c.A = (CFLOAT)atof(attrs[n+1]); type = TYPE_COLOR; break;
		}
	}
	
	switch(type)
	{
		case TYPE_POINT: param = parameter_t(p); break;
		case TYPE_COLOR: param = parameter_t(c); break;
	}
}

/*=============================================================
/ start- and endElement callbacks for the different states
=============================================================*/

void endEl_dummy(xmlParser_t &parser, const char *element)
{	parser.popState();	}

void startEl_dummy(xmlParser_t &parser, const char *element, const char **attrs)
{	parser.pushState(startEl_dummy, endEl_dummy);	}

void startEl_document(xmlParser_t &parser, const char *element, const char **attrs)
{
	if( strcmp(element, "scene") ) std::cout << "skipping <"<<element<<">" << std::endl; /* parser.error("Expected scene definition"); */
	else
	{
		for( ;attrs && attrs[0]; attrs += 2)
		{
			if(!strcmp(attrs[0], "type") )
			{
				std::string val(attrs[1]);
				if		(val == "triangle")  parser.scene->setMode(0);
				else if	(val == "universal") parser.scene->setMode(1);
			}
		}
		parser.pushState(startEl_scene, endEl_scene);
		//std::cout << "starting <scene>" << std::endl;
	}
}

void endEl_document(xmlParser_t &parser, const char *element)
{
	std::cout << "finished document\n";
}

struct mesh_dat_t
{
	mesh_dat_t(): has_orco(false), has_uv(false), smooth(false), smooth_angle(0), ID(0), mat(0) {};
	bool has_orco, has_uv;
	bool smooth;
	float smooth_angle;
	objID_t ID;
	const material_t *mat;
};

// scene-state, i.e. expect only primary elements
// such as light, material, texture, object, integrator, render...

void startEl_scene(xmlParser_t &parser, const char *element, const char **attrs)
{
	std::string el(element), *name=0;
	if( el == "material" || el == "integrator" || el == "light" || el == "texture" ||
		el == "camera" || el == "background" || el == "object" || el == "volumeregion")
	{
		if(!attrs[0]){ std::cerr << "error: no attributes for scene element given!" << std::endl; return; }
		else if(!strcmp(attrs[0], "name")) name = new std::string(attrs[1]);
		else{ std::cerr << "error: attribute for scene element does not match 'name'!" << std::endl; return; }
		parser.pushState(startEl_parammap, endEl_parammap, name);
		//std::cout << "starting <" << el << ">" << std::endl;
	}
	else if(el == "mesh")
	{
		mesh_dat_t *md = new mesh_dat_t();
		int vertices=0, triangles=0, type=0;
		for(int n=0; attrs[n]; ++n)
		{
			std::string name(attrs[n]);
			if(name == "has_orco") md->has_orco = str2bool(attrs[n+1]);
			else if(name == "has_uv") md->has_uv = str2bool(attrs[n+1]);
			else if(name == "vertices") vertices = atoi(attrs[n+1]);
			else if(name == "faces") triangles = atoi(attrs[n+1]);
			//else if(name == "smooth"){ md->smooth=true; md->smooth_angle = atof(attrs[n+1]); }
			else if(name == "type")	type = atoi(attrs[n+1]);
		}
		parser.pushState(startEl_mesh, endEl_mesh, md);
		if(!parser.scene->startGeometry()) std::cerr << "invalid scene state on startGeometry()!" << std::endl;
		// Get a new object ID
		md->ID = parser.scene->getNextFreeID();
		if(!parser.scene->startTriMesh(md->ID, vertices, triangles, md->has_orco, md->has_uv, type))
		{	std::cerr << "invalid scene state on startTriMesh()!" << std::endl; }
		//std::cout << "starting <" << el << ">" << std::endl;
	}
	else if(el == "smooth")
	{
		unsigned int ID=0;
		float angle=181;
		for(int n=0; attrs[n]; ++n)
		{
			std::string name(attrs[n]);
			if(name == "ID") ID = atoi(attrs[n+1]);
			else if(name == "angle") angle = atof(attrs[n+1]);
		}
		//not optimal to take ID blind...
		parser.scene->startGeometry();
		bool success = parser.scene->smoothMesh(ID, angle);
		if(!success) std::cout << "couldn't smooth mesh ID=" << ID << ", angle=" << angle << std::endl;
		parser.scene->endGeometry();
		parser.pushState(startEl_dummy, endEl_dummy);
	}
	else if(el == "render")
	{
		parser.cparams = &parser.render;
		parser.pushState(startEl_parammap, endEl_render);
		//std::cout << "skipping <" << el << ">" << std::endl;
	}
	else std::cout << "skipping unrecognized scene element" << std::endl;
}

void endEl_scene(xmlParser_t &parser, const char *element)
{
	if(strcmp(element, "scene")) std::cerr << "warning: expected </scene> tag!" << std::endl;
	else
	{
		//std::cout << "finished </scene>\n";
		parser.popState();
	}
}

// mesh-state, i.e. expect only points (vertices), faces and material settings
// since we're supposed to be inside a mesh block, exit state on "mesh" element

void startEl_mesh(xmlParser_t &parser, const char *element, const char **attrs)
{
	std::string el(element);
	mesh_dat_t *dat = (mesh_dat_t *)parser.stateData();
	if(el == "p")
	{
		point3d_t p, op;
		if(!parsePoint(attrs, p, op)) return;
		if(dat->has_orco)	parser.scene->addVertex(p, op);
		else 				parser.scene->addVertex(p);
	}
	else if(el == "f")
	{
		int a=0, b=0, c=0, uv_a=0, uv_b=0, uv_c=0;
		for( ;attrs && attrs[0]; attrs += 2)
		{
			if(attrs[0][1]==0) switch(attrs[0][0])
			{
				case 'a' : a = atoi(attrs[1]); break;
				case 'b' : b = atoi(attrs[1]); break;
				case 'c' : c = atoi(attrs[1]); break;
				default: std::cout << "Ignored wrong attr " << attrs[0] << " in face"<< std::endl;
			}
			else
			{
				if(!strcmp(attrs[0], "uv_a")) 	   uv_a = atoi(attrs[1]);
				else if(!strcmp(attrs[0], "uv_b")) uv_b = atoi(attrs[1]);
				else if(!strcmp(attrs[0], "uv_c")) uv_c = atoi(attrs[1]);
			}
		}
		if(dat->has_uv) parser.scene->addTriangle(a, b, c, uv_a, uv_b, uv_c, dat->mat);
		else 			parser.scene->addTriangle(a, b, c, dat->mat);
	}
	else if(el == "uv")
	{
		float u=0, v=0;
		for( ;attrs && attrs[0]; attrs += 2)
		{
			switch(attrs[0][0])
			{
				case 'u': u = atof(attrs[1]); break;
				case 'v': v = atof(attrs[1]); break;
				default: std::cout << "Ignored wrong attr " << attrs[0] << " in uv"<< std::endl;
			}
		}
		parser.scene->addUV(u, v);
	}
	else if(el == "set_material")
	{
		std::string mat_name(attrs[1]);
		dat->mat = parser.env->getMaterial(mat_name);
		if(!dat->mat) std::cerr << "unkown material!" << std::endl;
	}
}

void endEl_mesh(xmlParser_t &parser, const char *element)
{
	if(std::string(element) == "mesh")
	{
		mesh_dat_t *md = (mesh_dat_t *)parser.stateData();
		if(!parser.scene->endTriMesh()) std::cerr << "invalid scene state on endTriMesh()!" << std::endl;
//		if(md->smooth) parser.scene->smoothMesh(md->ID, md->smooth_angle);
		if(!parser.scene->endGeometry()) std::cerr << "invalid scene state on endGeometry()!" << std::endl;
		delete md;
		parser.popState();
	}
}

// read a parameter map; take any tag as parameter name
// again, exit when end-element is on of the elements that caused to enter state
// depending on exit element, create appropriate scene element

void startEl_parammap(xmlParser_t &parser, const char *element, const char **attrs)
{
	// support for lists of paramMaps
	if(std::string(element) == "list_element")
	{
		parser.eparams.push_back(paraMap_t());
		parser.cparams = &parser.eparams.back();
		parser.pushState(startEl_paramlist, endEl_paramlist);
		return;
	}
	parameter_t p;
	parseParam(attrs, p);
	parser.setParam(std::string(element), p);
	//std::cout << "adding parameter '" << element << "'" << std::endl;
}

void endEl_parammap(xmlParser_t &p, const char *element)
{
	bool exit_state= (p.currLevel() == p.stateLevel());	
	if(exit_state)
	{
		std::string el(element);
		std::string *name = (std::string *)p.stateData();
		if(!name) std::cerr << "error! no name for scene element available!" << std::endl;
		else
		{
			if(el == "material")
			{ p.env->createMaterial(*name, p.params, p.eparams); }
			else if(el == "integrator")
			{ p.env->createIntegrator(*name, p.params); }
			else if(el == "light")
			{
				light_t *light = p.env->createLight(*name, p.params);
				if(light) p.scene->addLight(light);
			}
			else if(el == "texture")
			{ p.env->createTexture(*name, p.params); }
			else if(el == "camera")
			{ p.env->createCamera(*name, p.params); }
			else if(el == "background")
			{ p.env->createBackground(*name, p.params); }
			else if(el == "object")
			{
				objID_t id;
				object3d_t *obj = p.env->createObject(*name, p.params);
				if(obj) p.scene->addObject(obj, id);
			}
			else if(el == "volumeregion")
			{
				VolumeRegion* vr = p.env->createVolumeRegion(*name, p.params);
				if (vr) p.scene->addVolumeRegion(vr);
			}
			else std::cerr << "warning: unexpected end-tag of scene element!\n";
		}
		
		if(name) delete name;
		p.popState(); p.params.clear(); p.eparams.clear();
		//std::cout << "ending <" << el << ">" << std::endl;
	}
}

void startEl_paramlist(xmlParser_t &parser, const char *element, const char **attrs)
{
	parameter_t p;
	parseParam(attrs, p);
	parser.setParam(std::string(element), p);
	//std::cout << "adding parameter '" << element << "'" << std::endl;
}

void endEl_paramlist(xmlParser_t &p, const char *element)
{
	if(std::string(element) == "list_element")
	{
		p.popState();
		p.cparams = &p.params;
	}
}

void endEl_render(xmlParser_t &p, const char *element)
{
	if(!strcmp(element, "render"))
	{
		p.cparams = &p.params;
		p.popState();
		//std::cout << "ending <render>" << std::endl;
	}
}

#endif // HAVE_XML

__END_YAFRAY
