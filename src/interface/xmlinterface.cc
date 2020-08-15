
#include <interface/xmlinterface.h>
#include <core_api/logging.h>
#include <core_api/environment.h>
#include <core_api/scene.h>
#include <core_api/params.h>

__BEGIN_YAFRAY

xmlInterface_t::xmlInterface_t(): last_mat(nullptr), nextObj(0), XMLGamma(1.f), XMLColorSpace(RAW_MANUAL_GAMMA)
{
	xmlName = "yafaray.xml";
}

// xml environment doesn't need any plugins...
void xmlInterface_t::loadPlugins(const char *path) { }


void xmlInterface_t::clearAll()
{
	Y_VERBOSE << "XMLInterface: cleaning up..." << yendl;
	env->clearAll();
	materials.clear();
	if(xmlFile.is_open())
	{
		xmlFile.flush();
		xmlFile.close();
	}
	params->clear();
	eparams->clear();
	cparams = params;
	nmat = 0;
	nextObj = 0;
}

bool xmlInterface_t::startScene(int type)
{
	xmlFile.open(xmlName.c_str());
	if(!xmlFile.is_open())
	{
		Y_ERROR << "XMLInterface: Couldn't open " << xmlName << yendl;
		return false;
	}
	else Y_INFO << "XMLInterface: Writing scene to: " << xmlName << yendl;
	xmlFile << std::boolalpha;
	xmlFile << "<?xml version=\"1.0\"?>" << yendl;
	xmlFile << "<scene type=\"";
	if(type == 0) xmlFile << "triangle";
	else 		xmlFile << "universal";
	xmlFile << "\">" << yendl;
	return true;
}

bool xmlInterface_t::setLoggingAndBadgeSettings()
{
	xmlFile << "\n<logging_badge name=\"logging_badge\">\n";
	writeParamMap(*params);
	params->clear();
	xmlFile << "</logging_badge>\n";
	return true;
}

bool xmlInterface_t::setupRenderPasses()
{
	xmlFile << "\n<render_passes name=\"render_passes\">\n";
	writeParamMap(*params);
	params->clear();
	xmlFile << "</render_passes>\n";
	return true;
}

void xmlInterface_t::setOutfile(const char *fname)
{
	xmlName = std::string(fname);
}

bool xmlInterface_t::startGeometry() { return true; }

bool xmlInterface_t::endGeometry() { return true; }

unsigned int xmlInterface_t::getNextFreeID()
{
	return ++nextObj;
}


bool xmlInterface_t::startTriMesh(unsigned int id, int vertices, int triangles, bool hasOrco, bool hasUV, int type, int obj_pass_index)
{
	last_mat = nullptr;
	n_uvs = 0;
	xmlFile << "\n<mesh id=\"" << id << "\" vertices=\"" << vertices << "\" faces=\"" << triangles
	        << "\" has_orco=\"" << hasOrco << "\" has_uv=\"" << hasUV << "\" type=\"" << type << "\" obj_pass_index=\"" << obj_pass_index << "\">\n";
	return true;
}

bool xmlInterface_t::startCurveMesh(unsigned int id, int vertices, int obj_pass_index)
{
	xmlFile << "\n<curve id=\"" << id << "\" vertices=\"" << vertices << "\" obj_pass_index=\"" << obj_pass_index << "\">\n";
	return true;
}

bool xmlInterface_t::startTriMeshPtr(unsigned int *id, int vertices, int triangles, bool hasOrco, bool hasUV, int type, int obj_pass_index)
{
	*id = ++nextObj;
	last_mat = nullptr;
	n_uvs = 0;
	xmlFile << "\n<mesh vertices=\"" << vertices << "\" faces=\"" << triangles
	        << "\" has_orco=\"" << hasOrco << "\" has_uv=\"" << hasUV << "\" type=\"" << type << "\" obj_pass_index=\"" << obj_pass_index << "\">\n";
	return true;
}

bool xmlInterface_t::endTriMesh()
{
	xmlFile << "</mesh>\n";
	return true;
}

bool xmlInterface_t::endCurveMesh(const material_t *mat, float strandStart, float strandEnd, float strandShape)
{
	auto i = materials.find(mat);
	if(i == materials.end()) return false;
	xmlFile << "\t\t\t<set_material sval=\"" << i->second << "\"/>\n"
	        << "\t\t\t<strand_start fval=\"" << strandStart << "\"/>\n"
	        << "\t\t\t<strand_end fval=\"" << strandEnd << "\"/>\n"
	        << "\t\t\t<strand_shape fval=\"" << strandShape << "\"/>\n"
	        << "</curve>\n";
	return true;
}

int  xmlInterface_t::addVertex(double x, double y, double z)
{
	xmlFile << "\t\t\t<p x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\"/>\n";
	return 0;
}

int  xmlInterface_t::addVertex(double x, double y, double z, double ox, double oy, double oz)
{
	xmlFile << "\t\t\t<p x=\"" << x << "\" y=\"" << y << "\" z=\"" << z
	        << "\" ox=\"" << ox << "\" oy=\"" << oy << "\" oz=\"" << oz << "\"/>\n";
	return 0;
}

void xmlInterface_t::addNormal(double x, double y, double z)
{
	xmlFile << "\t\t\t<n x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\"/>\n";
}

bool xmlInterface_t::addTriangle(int a, int b, int c, const material_t *mat)
{
	if(mat != last_mat) //need to set current material
	{
		auto i = materials.find(mat);
		if(i == materials.end()) return false;
		xmlFile << "\t\t\t<set_material sval=\"" << i->second << "\"/>\n";
		last_mat = mat;
	}
	xmlFile << "\t\t\t<f a=\"" << a << "\" b=\"" << b << "\" c=\"" << c << "\"/>\n";
	return true;
}

bool xmlInterface_t::addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const material_t *mat)
{
	if(mat != last_mat) //need to set current material
	{
		auto i = materials.find(mat);
		if(i == materials.end()) return false;
		xmlFile << "\t\t\t<set_material sval=\"" << i->second << "\"/>\n";
		last_mat = mat;
	}
	xmlFile << "\t\t\t<f a=\"" << a << "\" b=\"" << b << "\" c=\"" << c
	        << "\" uv_a=\"" << uv_a << "\" uv_b=\"" << uv_b << "\" uv_c=\"" << uv_c << "\"/>\n";
	return true;
}

int xmlInterface_t::addUV(float u, float v)
{
	xmlFile << "\t\t\t<uv u=\"" << u << "\" v=\"" << v << "\"/>\n";
	return n_uvs++;
}

bool xmlInterface_t::smoothMesh(unsigned int id, double angle)
{
	xmlFile << "<smooth ID=\"" << id << "\" angle=\"" << angle << "\"/>\n";
	return true;
}

inline void writeParam(const std::string &name, const parameter_t &param, std::ofstream &xmlFile, colorSpaces_t XMLColorSpace, float XMLGamma)
{
	int i = 0;
	bool b = false;
	double f = 0.0;
	const std::string *s = nullptr;
	colorA_t c(0.f);
	point3d_t p(0.f);
	switch(param.type())
	{
		case TYPE_INT:
			param.getVal(i);
			xmlFile << "<" << name << " ival=\"" << i << "\"/>\n"; break;
		case TYPE_BOOL:
			param.getVal(b);
			xmlFile << "<" << name << " bval=\"" << b << "\"/>\n"; break;
		case TYPE_FLOAT:
			param.getVal(f);
			xmlFile << "<" << name << " fval=\"" << f << "\"/>\n"; break;
		case TYPE_STRING:
			param.getVal(s);
			if(!s) break;
			xmlFile << "<" << name << " sval=\"" << *s << "\"/>\n"; break;
		case TYPE_POINT:
			param.getVal(p);
			xmlFile << "<" << name << " x=\"" << p.x << "\" y=\"" << p.y << "\" z=\"" << p.z << "\"/>\n"; break;
		case TYPE_COLOR:
			param.getVal(c);
			c.ColorSpace_from_linearRGB(XMLColorSpace, XMLGamma);	//Color values are encoded to the desired color space before saving them to the XML file
			xmlFile << "<" << name << " r=\"" << c.R << "\" g=\"" << c.G << "\" b=\"" << c.B << "\" a=\"" << c.A << "\"/>\n"; break;
		default:
			std::cerr << "unknown parameter type!\n";
	}
}

void writeMatrix(const std::string &name, const matrix4x4_t &m, std::ofstream &xmlFile)
{
	xmlFile << "<" << name << " m00=\"" << m[0][0] << "\" m01=\"" << m[0][1] << "\" m02=\"" << m[0][2]  << "\" m03=\"" << m[0][3] << "\""
	        << " m10=\"" << m[1][0] << "\" m11=\"" << m[1][1] << "\" m12=\"" << m[1][2]  << "\" m13=\"" << m[1][3] << "\""
	        << " m20=\"" << m[2][0] << "\" m21=\"" << m[2][1] << "\" m22=\"" << m[2][2]  << "\" m23=\"" << m[2][3] << "\""
	        << " m30=\"" << m[3][0] << "\" m31=\"" << m[3][1] << "\" m32=\"" << m[3][2]  << "\" m33=\"" << m[3][3] << "\"/>";
}

bool xmlInterface_t::addInstance(unsigned int baseObjectId, matrix4x4_t objToWorld)
{
	xmlFile << "\n<instance base_object_id=\"" << baseObjectId << "\" >\n\t";
	writeMatrix("transform", objToWorld, xmlFile);
	xmlFile << "\n</instance>\n";
	return true;
}

void xmlInterface_t::writeParamMap(const paraMap_t &pmap, int indent)
{
	std::string tabs(indent, '\t');
	const std::map< std::string, parameter_t > *dict = pmap.getDict();
	for(auto ip = dict->begin(); ip != dict->end(); ++ip)
	{
		xmlFile << tabs;
		writeParam(ip->first, ip->second, xmlFile, XMLColorSpace, XMLGamma);
	}
	const std::map< std::string, matrix4x4_t > *mdict = pmap.getMDict();
	for(auto im = mdict->begin(); im != mdict->end(); ++im)
	{
		xmlFile << tabs;
		writeMatrix(im->first, im->second, xmlFile);
	}
}

void xmlInterface_t::writeParamList(int indent)
{
	std::string tabs(indent, '\t');

	auto ip = eparams->begin();
	auto end = eparams->end();

	for(; ip != end; ++ip)
	{
		xmlFile << tabs << "<list_element>\n";
		writeParamMap(*ip, indent + 1);
		xmlFile << tabs << "</list_element>\n";
	}
}

light_t 		*xmlInterface_t::createLight(const char *name)
{
	xmlFile << "\n<light name=\"" << name << "\">\n";
	writeParamMap(*params);
	xmlFile << "</light>\n";
	return nullptr;
}

texture_t 		*xmlInterface_t::createTexture(const char *name)
{
	xmlFile << "\n<texture name=\"" << name << "\">\n";
	writeParamMap(*params);
	xmlFile << "</texture>\n";
	return nullptr;
}

material_t 	*xmlInterface_t::createMaterial(const char *name)
{
	material_t *matp = (material_t *)++nmat;
	materials[matp] = std::string(name);
	xmlFile << "\n<material name=\"" << name << "\">\n";
	writeParamMap(*params);
	writeParamList(1);
	xmlFile << "</material>\n";
	return matp;
}
camera_t 		*xmlInterface_t::createCamera(const char *name)
{
	xmlFile << "\n<camera name=\"" << name << "\">\n";
	writeParamMap(*params);
	xmlFile << "</camera>\n";
	return nullptr;
}
background_t 	*xmlInterface_t::createBackground(const char *name)
{
	xmlFile << "\n<background name=\"" << name << "\">\n";
	writeParamMap(*params);
	xmlFile << "</background>\n";
	return nullptr;
}
integrator_t 	*xmlInterface_t::createIntegrator(const char *name)
{
	xmlFile << "\n<integrator name=\"" << name << "\">\n";
	writeParamMap(*params);
	xmlFile << "</integrator>\n";
	return nullptr;
}

VolumeRegion 	*xmlInterface_t::createVolumeRegion(const char *name)
{
	xmlFile << "\n<volumeregion name=\"" << name << "\">\n";
	writeParamMap(*params);
	xmlFile << "</volumeregion>\n";
	return nullptr;
}

unsigned int 	xmlInterface_t::createObject(const char *name)
{
	xmlFile << "\n<object name=\"" << name << "\">\n";
	writeParamMap(*params);
	xmlFile << "</object>\n";
	return ++nextObj;
}

void xmlInterface_t::render(colorOutput_t &output, progressBar_t *pb)
{
	xmlFile << "\n<render>\n";
	writeParamMap(*params);
	xmlFile << "</render>\n";
	xmlFile << "</scene>" << yendl;
	xmlFile.flush();
	xmlFile.close();
}

void xmlInterface_t::setXMLColorSpace(std::string color_space_string, float gammaVal)
{
	if(color_space_string == "sRGB") XMLColorSpace = SRGB;
	else if(color_space_string == "XYZ") XMLColorSpace = XYZ_D65;
	else if(color_space_string == "LinearRGB") XMLColorSpace = LINEAR_RGB;
	else if(color_space_string == "Raw_Manual_Gamma") XMLColorSpace = RAW_MANUAL_GAMMA;
	else XMLColorSpace = SRGB;

	XMLGamma = gammaVal;
}

extern "C"
{
	YAFRAYPLUGIN_EXPORT xmlInterface_t *getYafrayXML()
	{
		return new xmlInterface_t();
	}
}

__END_YAFRAY
