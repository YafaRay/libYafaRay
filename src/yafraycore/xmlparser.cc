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
#include <core_api/logging.h>
#include <core_api/environment.h>
#include <core_api/scene.h>
#include <core_api/color.h>
#include <core_api/matrix4.h>
#include <utilities/math_utils.h>
#include <iomanip>

#if HAVE_XML
#include <libxml/parser.h>
#endif
#include <cstring>


BEGIN_YAFRAY

#if HAVE_XML

void XmlParser::setLastElementName(const char *element_name)
{
	if(element_name) current_->last_element_ = std::string(element_name);
	else current_->last_element_.clear();
}

void XmlParser::setLastElementNameAttrs(const char **element_attrs)
{
	current_->last_element_attrs_.clear();
	if(element_attrs)
	{
		for(int n = 0; element_attrs[n]; ++n)
		{
			if(n > 0) current_->last_element_attrs_ += " ";
			current_->last_element_attrs_ += (std::string(element_attrs[n]));
		}
	}
}

void startDocument__(void *user_data)
{
	//Empty
}

void endDocument__(void *user_data)
{
	//Empty
}

void startElement__(void *user_data, const xmlChar *name, const xmlChar **attrs)
{
	XmlParser &parser = *((XmlParser *)user_data);
	parser.startElement((const char *)name, (const char **)attrs);
}

void endElement__(void *user_data, const xmlChar *name)
{
	XmlParser &parser = *((XmlParser *)user_data);
	parser.endElement((const char *)name);
}

static void myWarning__(void *user_data, const char *msg, ...)
{
	XmlParser &parser = *((XmlParser *)user_data);
	va_list args;
	va_start(args, msg);
	const size_t message_size = 1000;
	char message_buffer[message_size];
	vsnprintf(message_buffer, message_size, msg, args);
	Y_WARNING << "XMLParser warning: " << message_buffer;
	Y_WARNING << " in section '" << parser.getLastSection() << ", level " << parser.currLevel() << YENDL;
	Y_WARNING << " an element previous to the error: '" << parser.getLastElementName() << "', attrs: { " << parser.getLastElementNameAttrs() << " }" << YENDL;
	va_end(args);
}

static void myError__(void *user_data, const char *msg, ...)
{
	XmlParser &parser = *((XmlParser *)user_data);
	va_list args;
	va_start(args, msg);
	const size_t message_size = 1000;
	char message_buffer[message_size];
	vsnprintf(message_buffer, message_size, msg, args);
	Y_ERROR << "XMLParser error: " << message_buffer;
	Y_ERROR << " in section '" << parser.getLastSection() << ", level " << parser.currLevel() << YENDL;
	Y_ERROR << " an element previous to the error: '" << parser.getLastElementName() << "', attrs: { " << parser.getLastElementNameAttrs() << " }" << YENDL;
	va_end(args);
}

static void myFatalError__(void *user_data, const char *msg, ...)
{
	XmlParser &parser = *((XmlParser *)user_data);
	va_list args;
	va_start(args, msg);
	const size_t message_size = 1000;
	char message_buffer[message_size];
	vsnprintf(message_buffer, message_size, msg, args);
	Y_ERROR << "XMLParser fatal error: " << message_buffer;
	Y_ERROR << " in section '" << parser.getLastSection() << ", level " << parser.currLevel() << YENDL;
	Y_ERROR << " an element previous to the error: '" << parser.getLastElementName() << "', attrs: { " << parser.getLastElementNameAttrs() << " }" << YENDL;
	va_end(args);
}

static xmlSAXHandler my_handler__ =
{
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		startDocument__, //  startDocumentSAXFunc startDocument;
	endDocument__, //  endDocumentSAXFunc endDocument;
	startElement__, //  startElementSAXFunc startElement;
	endElement__, //  endElementSAXFunc endElement;
	nullptr,
		nullptr, //  charactersSAXFunc characters;
	nullptr,
		nullptr,
		nullptr,
		myWarning__,
		myError__,
		myFatalError__
};
#endif // HAVE_XML

bool parseXmlFile__(const char *filename, Scene *scene, RenderEnvironment *env, ParamMap &render, std::string color_space_string, float input_gamma)
{
#if HAVE_XML

	ColorSpace input_color_space = RawManualGamma;

	if(color_space_string == "sRGB") input_color_space = Srgb;
	else if(color_space_string == "XYZ") input_color_space = XyzD65;
	else if(color_space_string == "LinearRGB") input_color_space = LinearRgb;
	//else if(color_space_string == "Raw_Manual_Gamma") input_color_space = RAW_MANUAL_GAMMA; //not available for now
	else input_color_space = Srgb;

	XmlParser parser(env, scene, render, input_color_space, input_gamma);

	if(xmlSAXUserParseFile(&my_handler__, &parser, filename) < 0)
	{
		Y_ERROR << "XMLParser: Parsing the file " << filename << YENDL;
		return false;
	}
	return true;
#else
	Y_WARNING << "XMLParser: yafray was compiled without XML support, cannot parse file." << YENDL;
	return false;
#endif
}

#if HAVE_XML
/*=============================================================
/ parser functions
=============================================================*/

XmlParser::XmlParser(RenderEnvironment *renv, Scene *sc, ParamMap &r, ColorSpace input_color_space, float input_gamma):
		env_(renv), scene_(sc), render_(r), current_(0), level_(0), input_gamma_(input_gamma), input_color_space_(input_color_space)
{
	cparams_ = &params_;
	pushState(startElDocument__, endElDocument__);
}

void XmlParser::pushState(StartElementCb_t start, EndElementCb_t end, void *userdata)
{
	ParserStateT state;
	state.start_ = start;
	state.end_ = end;
	state.userdata_ = userdata;
	state.level_ = level_;
	state_stack_.push_back(state);
	current_ = &state_stack_.back();
}

void XmlParser::popState()
{
	state_stack_.pop_back();
	if(!state_stack_.empty()) current_ = &state_stack_.back();
	else current_ = nullptr;
}

/*=============================================================
/ utility functions...
=============================================================*/

inline bool str2Bool__(const char *s) { return strcmp(s, "true") ? false : true; }

static bool parsePoint__(const char **attrs, Point3 &p, Point3 &op)
{
	for(; attrs && attrs[0]; attrs += 2)
	{
		if(attrs[0][0] == 'o')
		{
			if(attrs[0][1] == 0 || attrs[0][2] != 0)
			{
				Y_WARNING << "XMLParser: Ignored wrong attribute " << attrs[0] << " in orco point (1)" << YENDL;
				continue; //it is not a single character
			}
			switch(attrs[0][1])
			{
				case 'x' : op.x_ = atof(attrs[1]); break;
				case 'y' : op.y_ = atof(attrs[1]); break;
				case 'z' : op.z_ = atof(attrs[1]); break;
				default: Y_WARNING << "XMLParser: Ignored wrong attribute " << attrs[0] << " in orco point (2)" << YENDL;
			}
			continue;
		}
		else if(attrs[0][1] != 0)
		{
			Y_WARNING << "XMLParser: Ignored wrong attribute " << attrs[0] << " in point" << YENDL;
			continue; //it is not a single character
		}
		switch(attrs[0][0])
		{
			case 'x' : p.x_ = atof(attrs[1]); break;
			case 'y' : p.y_ = atof(attrs[1]); break;
			case 'z' : p.z_ = atof(attrs[1]); break;
			default: Y_WARNING << "XMLParser: Ignored wrong attribute " << attrs[0] << " in point" << YENDL;
		}
	}

	return true;
}

static bool parseNormal__(const char **attrs, Normal &n)
{
	int compo_read = 0;
	for(; attrs && attrs[0]; attrs += 2)
	{
		if(attrs[0][1] != 0)
		{
			Y_WARNING << "XMLParser: Ignored wrong attribute " << attrs[0] << " in normal" << YENDL;
			continue; //it is not a single character
		}
		switch(attrs[0][0])
		{
			case 'x' : n.x_ = atof(attrs[1]); compo_read++; break;
			case 'y' : n.y_ = atof(attrs[1]); compo_read++; break;
			case 'z' : n.z_ = atof(attrs[1]); compo_read++; break;
			default: Y_WARNING << "XMLParser: Ignored wrong attribute " << attrs[0] << " in normal." << YENDL;
		}
	}

	return (compo_read == 3);
}

void parseParam__(const char **attrs, Parameter &param, XmlParser &parser)
{
	if(!attrs[0]) return;
	if(!attrs[2]) // only one attribute => bool, integer or float value
	{
		std::string name(attrs[0]);
		if(name == "ival") { int i = atoi(attrs[1]); param = Parameter(i); return; }
		else if(name == "fval") { double f = atof(attrs[1]); param = Parameter(f); return; }
		else if(name == "bval") { bool b = str2Bool__(attrs[1]); param = Parameter(b); return; }
		else if(name == "sval") { param = Parameter(std::string(attrs[1])); return; }
	}
	Rgba c(0.f); Point3 p(0, 0, 0); Matrix4 m;
	Parameter::Type type = Parameter::None;
	for(int n = 0; attrs[n]; ++n)
	{
		if(attrs[n][1] == '\0')
		{
			switch(attrs[n][0])
			{
				case 'x': p.x_ = atof(attrs[n + 1]); type = Parameter::Point; break;
				case 'y': p.y_ = atof(attrs[n + 1]); type = Parameter::Point; break;
				case 'z': p.z_ = atof(attrs[n + 1]); type = Parameter::Point; break;

				case 'r': c.r_ = (float)atof(attrs[n + 1]); type = Parameter::Color; break;
				case 'g': c.g_ = (float)atof(attrs[n + 1]); type = Parameter::Color; break;
				case 'b': c.b_ = (float)atof(attrs[n + 1]); type = Parameter::Color; break;
				case 'a': c.a_ = (float)atof(attrs[n + 1]); type = Parameter::Color; break;
			}
		}
		else if(attrs[n][3] == '\0' && attrs[n][0] == 'm' && attrs[n][1] >= '0' && attrs[n][1] <= '3' && attrs[n][2] >= '0' && attrs[n][2] <= '3') //"mij" where i and j are between 0 and 3 (inclusive)
		{
			type = Parameter::Matrix;
			const int i = attrs[n][1] - '0';
			const int j = attrs[n][2] - '0';
			m[i][j] = atof(attrs[n + 1]);
		}
	}

	if(type == Parameter::Point) param = Parameter(p);
	else if(type == Parameter::Matrix) param = Parameter(m);
	else if(type == Parameter::Color)
	{
		c.linearRgbFromColorSpace(parser.getInputColorSpace(), parser.getInputGamma());
		param = Parameter(c);
	}
}

/*=============================================================
/ start- and endElement callbacks for the different states
=============================================================*/

void endElDummy__(XmlParser &parser, const char *element)
{	parser.popState();	}

void startElDummy__(XmlParser &parser, const char *element, const char **attrs)
{ parser.pushState(startElDummy__, endElDummy__);	}

void startElDocument__(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Document");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);

	if(strcmp(element, "scene")) Y_WARNING << "XMLParser: skipping <" << element << ">" << YENDL;   /* parser.error("Expected scene definition"); */
	else
	{
		for(; attrs && attrs[0]; attrs += 2)
		{
			if(!strcmp(attrs[0], "type"))
			{
				std::string val(attrs[1]);
				if(val == "triangle")  parser.scene_->setMode(0);
				else if(val == "universal") parser.scene_->setMode(1);
			}
		}
		parser.pushState(startElScene__, endElScene__);
	}
}

void endElDocument__(XmlParser &parser, const char *element)
{
	Y_VERBOSE << "XMLParser: Finished document" << YENDL;
}

struct MeshDat
{
	MeshDat(): has_orco_(false), has_uv_(false), smooth_(false), smooth_angle_(0), id_(0), mat_(0) {};
	bool has_orco_, has_uv_;
	bool smooth_;
	float smooth_angle_;
	ObjId_t id_;
	const Material *mat_;
};

struct CurveDat
{
	CurveDat(): id_(0), mat_(nullptr), strand_start_(0), strand_end_(0), strand_shape_(0) {};
	ObjId_t id_;
	const Material *mat_;
	float strand_start_, strand_end_, strand_shape_;
};

// scene-state, i.e. expect only primary elements
// such as light, material, texture, object, integrator, render...

void startElScene__(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Scene");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);

	std::string el(element), *name = 0;
	if(el == "material" || el == "integrator" || el == "light" || el == "texture" ||
	        el == "camera" || el == "background" || el == "object" || el == "volumeregion" || el == "render_passes" || el == "logging_badge")
	{
		if(!attrs[0])
		{
			Y_ERROR << "XMLParser: No attributes for scene element given!" << YENDL;
			return;
		}
		else if(!strcmp(attrs[0], "name")) name = new std::string(attrs[1]);
		else
		{
			Y_ERROR << "XMLParser: Attribute for scene element does not match 'name'!" << YENDL;
			return;
		}
		parser.pushState(startElParammap__, endElParammap__, name);
	}
	else if(el == "mesh")
	{
		MeshDat *md = new MeshDat();
		int vertices = 0, triangles = 0, type = 0, id = -1, obj_pass_index = 0;
		for(int n = 0; attrs[n]; ++n)
		{
			std::string name(attrs[n]);
			if(name == "has_orco") md->has_orco_ = str2Bool__(attrs[n + 1]);
			else if(name == "has_uv") md->has_uv_ = str2Bool__(attrs[n + 1]);
			else if(name == "vertices") vertices = atoi(attrs[n + 1]);
			else if(name == "faces") triangles = atoi(attrs[n + 1]);
			else if(name == "type")	type = atoi(attrs[n + 1]);
			else if(name == "id") id = atoi(attrs[n + 1]);
			else if(name == "obj_pass_index") obj_pass_index = atoi(attrs[n + 1]);
		}
		parser.pushState(startElMesh__, endElMesh__, md);
		if(!parser.scene_->startGeometry()) Y_ERROR << "XMLParser: Invalid scene state on startGeometry()!" << YENDL;

		// Get a new object ID if we did not get one
		if(id == -1) md->id_ = parser.scene_->getNextFreeId();
		else md->id_ = id;

		if(!parser.scene_->startTriMesh(md->id_, vertices, triangles, md->has_orco_, md->has_uv_, type, obj_pass_index))
		{
			Y_ERROR << "XMLParser: Invalid scene state on startTriMesh()!" << YENDL;
		}
	}
	else if(el == "smooth")
	{
		unsigned int id = 0;
		float angle = 181;
		for(int n = 0; attrs[n]; ++n)
		{
			std::string name(attrs[n]);
			if(name == "ID") id = atoi(attrs[n + 1]);
			else if(name == "angle") angle = atof(attrs[n + 1]);
		}
		//not optimal to take ID blind...
		parser.scene_->startGeometry();
		bool success = parser.scene_->smoothMesh(id, angle);
		if(!success) Y_ERROR << "XMLParser: Couldn't smooth mesh ID = " << id << ", angle = " << angle << YENDL;
		parser.scene_->endGeometry();
		parser.pushState(startElDummy__, endElDummy__);
	}
	else if(el == "render")
	{
		parser.cparams_ = &parser.render_;
		parser.pushState(startElParammap__, endElRender__);
	}
	else if(el == "instance")
	{
		ObjId_t *base_object_id = new ObjId_t();
		*base_object_id = -1;
		for(int n = 0; attrs[n]; n++)
		{
			std::string name(attrs[n]);
			if(name == "base_object_id") *base_object_id = atoi(attrs[n + 1]);
		}
		parser.pushState(startElInstance__, endElInstance__, base_object_id);
	}
	else if(el == "curve")
	{
		CurveDat *cvd = new CurveDat();
		int vertex = 0, idc = -1;
		// attribute's loop
		for(int n = 0; attrs[n]; ++n)
		{
			std::string name(attrs[n]);
			if(name == "vertices") vertex = atoi(attrs[n + 1]);
			else if(name == "id") idc = atoi(attrs[n + 1]);
		}
		parser.pushState(startElCurve__, endElCurve__, cvd);
		if(!parser.scene_->startGeometry()) Y_ERROR << "XMLParser: Invalid scene state on startGeometry()!" << YENDL;

		// Get a new object ID if we did not get one
		if(idc == -1) cvd->id_ = parser.scene_->getNextFreeId();
		else cvd->id_ = idc;

		if(!parser.scene_->startCurveMesh(cvd->id_, vertex))
		{
			Y_ERROR << "XMLParser: Invalid scene state on startCurveMesh()!" << YENDL;
		}
	}
	else Y_WARNING << "XMLParser: Skipping unrecognized scene element" << YENDL;
}

void endElScene__(XmlParser &parser, const char *element)
{
	if(strcmp(element, "scene")) Y_WARNING << "XMLParser: : expected </scene> tag!" << YENDL;
	else
	{
		parser.popState();
	}
}
void startElCurve__(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Curve");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);

	std::string el(element);
	CurveDat *dat = (CurveDat *)parser.stateData();

	if(el == "p")
	{
		Point3 p, op;
		if(!parsePoint__(attrs, p, op)) return;
		parser.scene_->addVertex(p);
	}
	else if(el == "strand_start")
	{
		dat->strand_start_ = atof(attrs[1]);
	}
	else if(el == "strand_end")
	{
		dat->strand_end_ = atof(attrs[1]);
	}
	else if(el == "strand_shape")
	{
		dat->strand_shape_ = atof(attrs[1]);

	}
	else if(el == "set_material")
	{
		std::string mat_name(attrs[1]);
		dat->mat_ = parser.env_->getMaterial(mat_name);
		if(!dat->mat_) Y_WARNING << "XMLParser: Unknown material!" << YENDL;
	}
}
void endElCurve__(XmlParser &parser, const char *element)
{
	if(std::string(element) == "curve")
	{
		CurveDat *cd = (CurveDat *)parser.stateData();
		if(!parser.scene_->endCurveMesh(cd->mat_, cd->strand_start_, cd->strand_end_, cd->strand_shape_))
		{
			Y_WARNING << "XMLParser: Invalid scene state on endCurveMesh()!" << YENDL;
		}
		if(!parser.scene_->endGeometry())
		{
			Y_WARNING << "XMLParser: Invalid scene state on endGeometry()!" << YENDL;
		}
		delete cd;
		parser.popState();
	}
}

// mesh-state, i.e. expect only points (vertices), faces and material settings
// since we're supposed to be inside a mesh block, exit state on "mesh" element
void startElMesh__(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Mesh");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);

	std::string el(element);
	MeshDat *dat = (MeshDat *)parser.stateData();
	if(el == "p")
	{
		Point3 p, op;
		if(!parsePoint__(attrs, p, op)) return;
		if(dat->has_orco_)	parser.scene_->addVertex(p, op);
		else 				parser.scene_->addVertex(p);
	}
	else if(el == "n")
	{
		Normal n(0.0, 0.0, 0.0);
		if(!parseNormal__(attrs, n)) return;
		parser.scene_->addNormal(n);
	}
	else if(el == "f")
	{
		int a = 0, b = 0, c = 0, uv_a = 0, uv_b = 0, uv_c = 0;
		for(; attrs && attrs[0]; attrs += 2)
		{
			if(attrs[0][1] == 0) switch(attrs[0][0])
				{
					case 'a' : a = atoi(attrs[1]); break;
					case 'b' : b = atoi(attrs[1]); break;
					case 'c' : c = atoi(attrs[1]); break;
					default: Y_WARNING << "XMLParser: Ignored wrong attribute " << attrs[0] << " in face" << YENDL;
				}
			else
			{
				if(!strcmp(attrs[0], "uv_a")) 	   uv_a = atoi(attrs[1]);
				else if(!strcmp(attrs[0], "uv_b")) uv_b = atoi(attrs[1]);
				else if(!strcmp(attrs[0], "uv_c")) uv_c = atoi(attrs[1]);
			}
		}
		if(dat->has_uv_) parser.scene_->addTriangle(a, b, c, uv_a, uv_b, uv_c, dat->mat_);
		else 			parser.scene_->addTriangle(a, b, c, dat->mat_);
	}
	else if(el == "uv")
	{
		float u = 0, v = 0;
		for(; attrs && attrs[0]; attrs += 2)
		{
			switch(attrs[0][0])
			{
				case 'u': u = atof(attrs[1]);
					if(!(isValidFloat__(u)))
					{
						Y_WARNING << std::scientific << std::setprecision(6) << "XMLParser: invalid value in \"" << el << "\" xml entry: " << attrs[0] << "=" << attrs[1] << ". Replacing with 0.0." << YENDL;
						u = 0.f;
					}
					break;
				case 'v': v = atof(attrs[1]);
					if(!(isValidFloat__(v)))
					{
						Y_WARNING << std::scientific << std::setprecision(6) << "XMLParser: invalid value in \"" << el << "\" xml entry: " << attrs[0] << "=" << attrs[1] << ". Replacing with 0.0." << YENDL;
						v = 0.f;
					}
					break;

				default: Y_WARNING << "XMLParser: Ignored wrong attribute " << attrs[0] << " in uv" << YENDL;
			}
		}
		parser.scene_->addUv(u, v);
	}
	else if(el == "set_material")
	{
		std::string mat_name(attrs[1]);
		dat->mat_ = parser.env_->getMaterial(mat_name);
		if(!dat->mat_) Y_WARNING << "XMLParser: Unknown material!" << YENDL;
	}
}

void endElMesh__(XmlParser &parser, const char *element)
{
	if(std::string(element) == "mesh")
	{
		MeshDat *md = (MeshDat *)parser.stateData();
		if(!parser.scene_->endTriMesh()) Y_ERROR << "XMLParser: Invalid scene state on endTriMesh()!" << YENDL;
		if(!parser.scene_->endGeometry()) Y_ERROR << "XMLParser: Invalid scene state on endGeometry()!" << YENDL;
		delete md;
		parser.popState();
	}
}

void startElInstance__(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Instance");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);

	std::string el(element);
	ObjId_t boi = *(ObjId_t *)parser.stateData();
	if(el == "transform")
	{
		float m[4][4];
		for(int n = 0; attrs[n]; ++n)
		{
			if(attrs[n][3] == '\0' && attrs[n][0] == 'm' && attrs[n][1] >= '0' && attrs[n][1] <= '3' && attrs[n][2] >= '0' && attrs[n][2] <= '3') //"mij" where i and j are between 0 and 3 (inclusive)
			{
				const int i = attrs[n][1] - '0';
				const int j = attrs[n][2] - '0';
				m[i][j] = atof(attrs[n + 1]);
			}
		}
		Matrix4 *m_4 = new Matrix4(m);
		parser.scene_->addInstance(boi, *m_4);
	}
}

void endElInstance__(XmlParser &parser, const char *element)
{
	if(std::string(element) == "instance")
	{
		parser.popState();
	}
}
// read a parameter map; take any tag as parameter name
// again, exit when end-element is on of the elements that caused to enter state
// depending on exit element, create appropriate scene element

void startElParammap__(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Params map");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);
	// support for lists of paramMaps
	if(std::string(element) == "list_element")
	{
		parser.eparams_.push_back(ParamMap());
		parser.cparams_ = &parser.eparams_.back();
		parser.pushState(startElParamlist__, endElParamlist__);
		return;
	}
	Parameter p;
	parseParam__(attrs, p, parser);
	parser.setParam(std::string(element), p);
}

void endElParammap__(XmlParser &p, const char *element)
{
	bool exit_state = (p.currLevel() == p.stateLevel());
	if(exit_state)
	{
		std::string el(element);
		std::string *name = (std::string *)p.stateData();
		if(!name) Y_ERROR << "XMLParser: No name for scene element available!" << YENDL;
		else
		{
			if(el == "material")
			{ p.env_->createMaterial(*name, p.params_, p.eparams_); }
			else if(el == "integrator")
			{ p.env_->createIntegrator(*name, p.params_); }
			else if(el == "light")
			{
				Light *light = p.env_->createLight(*name, p.params_);
				if(light) p.scene_->addLight(light);
			}
			else if(el == "texture")
			{ p.env_->createTexture(*name, p.params_); }
			else if(el == "camera")
			{ p.env_->createCamera(*name, p.params_); }
			else if(el == "background")
			{ p.env_->createBackground(*name, p.params_); }
			else if(el == "object")
			{
				ObjId_t id;
				Object3D *obj = p.env_->createObject(*name, p.params_);
				if(obj) p.scene_->addObject(obj, id);
			}
			else if(el == "volumeregion")
			{
				VolumeRegion *vr = p.env_->createVolumeRegion(*name, p.params_);
				if(vr) p.scene_->addVolumeRegion(vr);
			}
			else if(el == "render_passes")
			{
				p.env_->setupRenderPasses(p.params_);
			}
			else if(el == "logging_badge")
			{
				p.env_->setupLoggingAndBadge(p.params_);
			}
			else Y_WARNING << "XMLParser: Unexpected end-tag of scene element!" << YENDL;
		}

		if(name) delete name;
		p.popState(); p.params_.clear(); p.eparams_.clear();
	}
}

void startElParamlist__(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Params list");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);
	Parameter p;
	parseParam__(attrs, p, parser);
	parser.setParam(std::string(element), p);
}

void endElParamlist__(XmlParser &parser, const char *element)
{
	if(std::string(element) == "list_element")
	{
		parser.popState();
		parser.cparams_ = &parser.params_;
	}
}

void endElRender__(XmlParser &parser, const char *element)
{
	parser.setLastSection("render");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(nullptr);

	if(!strcmp(element, "render"))
	{
		parser.cparams_ = &parser.params_;
		parser.popState();
	}
}

#endif // HAVE_XML

END_YAFRAY
