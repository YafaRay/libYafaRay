/****************************************************************************
 *      xmlparser.cc: a libXML based parser for YafRay scenes
 *      This is part of the libYafaRay package
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

#include "import/import_xml.h"
#include "common/logger.h"
#include "scene/scene.h"
#include "color/color.h"
#include "output/output.h"
#include "geometry/matrix4.h"

#if HAVE_XML
#include <libxml/parser.h>
#endif
#include <cstring>

BEGIN_YAFARAY

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

void startDocument_global(void *user_data)
{
	//Empty
}

void endDocument_global(void *user_data)
{
	//Empty
}

void startElement_global(void *user_data, const xmlChar *name, const xmlChar **attrs)
{
	XmlParser &parser = *((XmlParser *)user_data);
	parser.startElement((const char *)name, (const char **)attrs);
}

void endElement_global(void *user_data, const xmlChar *name)
{
	XmlParser &parser = *((XmlParser *)user_data);
	parser.endElement((const char *)name);
}

static void myWarning_global(void *user_data, const char *msg, ...)
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

static void myError_global(void *user_data, const char *msg, ...)
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

static void myFatalError_global(void *user_data, const char *msg, ...)
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

static xmlSAXHandler my_handler_global =
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
	startDocument_global, //  startDocumentSAXFunc startDocument;
	endDocument_global, //  endDocumentSAXFunc endDocument;
	startElement_global, //  startElementSAXFunc startElement;
	endElement_global, //  endElementSAXFunc endElement;
	nullptr,
	nullptr, //  charactersSAXFunc characters;
	nullptr,
	nullptr,
	nullptr,
	myWarning_global,
	myError_global,
	myFatalError_global
};
#endif // HAVE_XML

std::unique_ptr<Scene> parseXmlFile_global(const char *filename, ParamMap &render, const std::string &color_space_string, float input_gamma)
{
#if HAVE_XML

	ColorSpace input_color_space = Rgb::colorSpaceFromName(color_space_string);
	XmlParser parser(render, input_color_space, input_gamma);
	if(xmlSAXUserParseFile(&my_handler_global, &parser, filename) < 0)
	{
		Y_ERROR << "XMLParser: Parsing the file " << filename << YENDL;
		return nullptr;
	}
	return parser.getScene();
#else
	Y_WARNING << "XMLParser: yafray was compiled without XML support, cannot parse file." << YENDL;
	return nullptr;
#endif
}

#if HAVE_XML
/*=============================================================
/ parser functions
=============================================================*/

XmlParser::XmlParser(ParamMap &r, ColorSpace input_color_space, float input_gamma):
		render_(r), input_gamma_(input_gamma), input_color_space_(input_color_space)
{
	cparams_ = &params_;
	pushState(startElDocument_global, endElDocument_global);
}

void XmlParser::pushState(StartElementCb_t start, EndElementCb_t end, void *userdata)
{
	ParserState state;
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

inline bool str2Bool_global(const char *s) { return strcmp(s, "true") ? false : true; }

static bool parsePoint_global(const char **attrs, Point3 &p, Point3 &op, bool &has_orco)
{
	for(; attrs && attrs[0]; attrs += 2)
	{
		if(attrs[0][0] == 'o')
		{
			has_orco = true;
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

static bool parseNormal_global(const char **attrs, Vec3 &n)
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

void parseParam_global(const char **attrs, Parameter &param, XmlParser &parser)
{
	if(!attrs[0]) return;
	if(!attrs[2]) // only one attribute => bool, integer or float value
	{
		std::string attr(attrs[0]);
		if(attr == "ival") { int i = atoi(attrs[1]); param = Parameter(i); return; }
		else if(attr == "fval") { double f = atof(attrs[1]); param = Parameter(f); return; }
		else if(attr == "bval") { bool b = str2Bool_global(attrs[1]); param = Parameter(b); return; }
		else if(attr == "sval") { param = Parameter(std::string(attrs[1])); return; }
	}
	Rgba c(0.f); Vec3 v(0, 0, 0); Matrix4 m;
	Parameter::Type type = Parameter::None;
	for(int n = 0; attrs[n]; ++n)
	{
		if(attrs[n][1] == '\0')
		{
			switch(attrs[n][0])
			{
				case 'x': v.x_ = atof(attrs[n + 1]); type = Parameter::Vector; break;
				case 'y': v.y_ = atof(attrs[n + 1]); type = Parameter::Vector; break;
				case 'z': v.z_ = atof(attrs[n + 1]); type = Parameter::Vector; break;

				case 'r': c.r_ = atof(attrs[n + 1]); type = Parameter::Color; break;
				case 'g': c.g_ = atof(attrs[n + 1]); type = Parameter::Color; break;
				case 'b': c.b_ = atof(attrs[n + 1]); type = Parameter::Color; break;
				case 'a': c.a_ = atof(attrs[n + 1]); type = Parameter::Color; break;
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

	if(type == Parameter::Vector) param = Parameter(v);
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

void endElDummy_global(XmlParser &parser, const char *element)
{
	parser.popState();
}

void startElDummy_global(XmlParser &parser, const char *element, const char **attrs)
{
	parser.pushState(startElDummy_global, endElDummy_global);
}

void startElDocument_global(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Document");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);

	if(strcmp(element, "scene")) Y_WARNING << "XMLParser: skipping <" << element << ">" << YENDL;   /* parser.error("Expected scene definition"); */
	else parser.pushState(startElScene_global, endElScene_global);
}

void endElDocument_global(XmlParser &parser, const char *element)
{
	Y_VERBOSE << "XMLParser: Finished document" << YENDL;
}

// scene-state, i.e. expect only primary elements
// such as light, material, texture, object, integrator, render...

void startElScene_global(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Scene");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);

	std::string el(element), *name = nullptr;
	if(el == "material" || el == "integrator" || el == "light" || el == "texture" || el == "camera" || el == "background" || el == "volumeregion" || el == "logging_badge" || el == "output" || el == "render_view")
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
		parser.pushState(startElParammap_global, endElParammap_global, name);
	}
	else if(el == "layer" || el == "layers_parameters" || el == "scene_parameters")
	{
		name = new std::string("");
		parser.pushState(startElParammap_global, endElParammap_global, name);
	}
	else if(el == "object")
	{
		name = new std::string("Object_" + std::to_string(parser.scene_->getNextFreeId()));
		parser.pushState(startElObject_global, endElObject_global, name);
		if(!parser.scene_->startObjects()) Y_ERROR << "XMLParser: Invalid scene state on startGeometry()!" << YENDL;
	}
	else if(el == "smooth")
	{
		float angle = 181;
		std::string object_name;
		for(int n = 0; attrs[n]; ++n)
		{
			std::string attr_name(attrs[n]);
			if(attr_name == "object_name") object_name = std::string(attrs[n + 1]);
			else if(attr_name == "angle") angle = atof(attrs[n + 1]);
		}
		//not optimal to take ID blind...
		parser.scene_->startObjects();
		bool success = parser.scene_->smoothNormals(object_name, angle);
		if(!success) Y_ERROR << "XMLParser: Couldn't smooth object with object_name='" << object_name << "', angle = " << angle << YENDL;

		parser.scene_->endObjects();
		parser.pushState(startElDummy_global, endElDummy_global);
	}
	else if(el == "render")
	{
		parser.cparams_ = &parser.render_;
		parser.pushState(startElParammap_global, endElRender_global);
	}
	else if(el == "instance")
	{
		std::string *base_object_name = new std::string;
		for(int n = 0; attrs[n]; n++)
		{
			std::string attr(attrs[n]);
			if(attr == "base_object_name") *base_object_name = attrs[n + 1];
		}
		parser.pushState(startElInstance_global, endElInstance_global, base_object_name);
	}
	else Y_WARNING << "XMLParser: Skipping unrecognized scene element" << YENDL;
}

void endElScene_global(XmlParser &parser, const char *element)
{
	if(strcmp(element, "scene")) Y_WARNING << "XMLParser: : expected </scene> tag!" << YENDL;
	else
	{
		parser.popState();
	}
}

// object-state, i.e. expect only points (vertices), faces and material settings
// since we're supposed to be inside an object block, exit state on "object" element
void startElObject_global(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Object");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);

	std::string el(element);
	if(el == "p")
	{
		Point3 p, op;
		bool has_orco = false;
		if(!parsePoint_global(attrs, p, op, has_orco)) return;
		if(has_orco) parser.scene_->addVertex(p, op);
		else parser.scene_->addVertex(p);
	}
	else if(el == "n")
	{
		Vec3 n(0.0, 0.0, 0.0);
		if(!parseNormal_global(attrs, n)) return;
		parser.scene_->addNormal(n);
	}
	else if(el == "f")
	{
		std::vector<int> vertices_indices, uv_indices;
		vertices_indices.reserve(3);
		vertices_indices.reserve(3);
		for(; attrs && attrs[0]; attrs += 2)
		{
			const std::string attribute = attrs[0];
			if(attribute.size() == 1) switch(attribute[0])
			{
				case 'a' :
				case 'b' :
				case 'c' :
				case 'd' : vertices_indices.push_back(atoi(attrs[1])); break;
				default: Y_WARNING << "XMLParser: Ignored wrong attribute " << attrs[0] << " in face" << YENDL;
			}
			else
			{
				if(attribute.substr(0, 3) == "uv_") uv_indices.push_back(atoi(attrs[1]));
			}
		}
		parser.scene_->addFace(vertices_indices, uv_indices);
	}
	else if(el == "uv")
	{
		float u = 0, v = 0;
		for(; attrs && attrs[0]; attrs += 2)
		{
			switch(attrs[0][0])
			{
				case 'u': u = atof(attrs[1]);
					if(!(math::isValid(u)))
					{
						Y_WARNING << std::scientific << std::setprecision(6) << "XMLParser: invalid value in \"" << el << "\" xml entry: " << attrs[0] << "=" << attrs[1] << ". Replacing with 0.0." << YENDL;
						u = 0.f;
					}
					break;
				case 'v': v = atof(attrs[1]);
					if(!(math::isValid(v)))
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
		const Material *material = parser.scene_->getMaterial(mat_name);
		if(!material) Y_WARNING << "XMLParser: Unknown material, using default!" << YENDL;
		parser.scene_->setCurrentMaterial(material);
	}
	else if(el == "object_parameters")
	{
		std::string *name = nullptr;
		if(!strcmp(attrs[0], "name")) name = new std::string(attrs[1]);
		parser.pushState(startElParammap_global, endElParammap_global, name);
	}
}

void endElObject_global(XmlParser &parser, const char *element)
{
	if(std::string(element) == "object")
	{
		if(!parser.scene_->endObject()) Y_ERROR << "XMLParser: Invalid scene state on endObject()!" << YENDL;
		if(!parser.scene_->endObjects()) Y_ERROR << "XMLParser: Invalid scene state on endGeometry()!" << YENDL;
		parser.popState();
	}
}

void startElInstance_global(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Instance");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);

	std::string el(element);
	const std::string base_object_name = *(std::string *)parser.stateData();
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
		parser.scene_->addInstance(base_object_name, m);
	}
}

void endElInstance_global(XmlParser &parser, const char *element)
{
	if(std::string(element) == "instance")
	{
		parser.popState();
	}
}
// read a parameter map; take any tag as parameter name
// again, exit when end-element is on of the elements that caused to enter state
// depending on exit element, create appropriate scene element

void startElParammap_global(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Params map");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);
	// support for lists of paramMaps
	if(std::string(element) == "list_element")
	{
		parser.eparams_.push_back(ParamMap());
		parser.cparams_ = &parser.eparams_.back();
		parser.pushState(startElParamlist_global, endElParamlist_global);
		return;
	}
	Parameter param;
	parseParam_global(attrs, param, parser);
	parser.setParam(std::string(element), param);
}

void endElParammap_global(XmlParser &parser, const char *element)
{
	bool exit_state = (parser.currLevel() == parser.stateLevel());
	if(exit_state)
	{
		std::string el(element);
		std::string *name = (std::string *)parser.stateData();
		if(!name) Y_ERROR << "XMLParser: No name for scene element available!" << YENDL;
		else
		{
			if(el == "scene_parameters")
			{
				parser.scene_ = Scene::factory(parser.params_);
				if(!parser.scene_)
				{
					Y_ERROR << "XML Loader: scene could not be created." << YENDL;
				}
			}
			else if(el == "material") parser.scene_->createMaterial(*name, parser.params_, parser.eparams_);
			else if(el == "integrator") parser.scene_->createIntegrator(*name, parser.params_);
			else if(el == "light") parser.scene_->createLight(*name, parser.params_);
			else if(el == "texture") parser.scene_->createTexture(*name, parser.params_);
			else if(el == "camera") parser.scene_->createCamera(*name, parser.params_);
			else if(el == "background") parser.scene_->createBackground(*name, parser.params_);
			else if(el == "object_parameters") parser.scene_->createObject(*name, parser.params_);
			else if(el == "volumeregion") parser.scene_->createVolumeRegion(*name, parser.params_);
			else if(el == "layers_parameters") parser.scene_->setupLayersParameters(parser.params_);
			else if(el == "layer") parser.scene_->defineLayer(parser.params_);
			else if(el == "output") parser.scene_->createOutput(*name, parser.params_);
			else if(el == "render_view") parser.scene_->createRenderView(*name, parser.params_);
			else Y_WARNING << "XMLParser: Unexpected end-tag of scene element!" << YENDL;
		}
		delete name;
		parser.popState(); parser.params_.clear(); parser.eparams_.clear();
	}
}

void startElParamlist_global(XmlParser &parser, const char *element, const char **attrs)
{
	parser.setLastSection("Params list");
	parser.setLastElementName(element);
	parser.setLastElementNameAttrs(attrs);
	Parameter param;
	parseParam_global(attrs, param, parser);
	parser.setParam(std::string(element), param);
}

void endElParamlist_global(XmlParser &parser, const char *element)
{
	if(std::string(element) == "list_element")
	{
		parser.popState();
		parser.cparams_ = &parser.params_;
	}
}

void endElRender_global(XmlParser &parser, const char *element)
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

END_YAFARAY
