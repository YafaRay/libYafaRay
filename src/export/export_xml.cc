/****************************************************************************
 *      This is part of the libYafaRay package
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

#include "export/export_xml.h"
#include "common/logger.h"
#include "scene/scene.h"
#include "geometry/matrix4.h"
#include "common/param.h"

BEGIN_YAFARAY

XmlExport::XmlExport(): last_mat_(nullptr), next_obj_(0), xml_gamma_(1.f), xml_color_space_(RawManualGamma)
{
	xml_name_ = "yafaray.xml";
}

void XmlExport::clearAll()
{
	Y_VERBOSE << "XmlExport: cleaning up..." << YENDL;
	scene_->clearAll();
	materials_.clear();
	if(xml_file_.is_open())
	{
		xml_file_.flush();
		xml_file_.close();
	}
	params_->clear();
	eparams_->clear();
	cparams_ = params_;
	nmat_ = 0;
	next_obj_ = 0;
}

bool XmlExport::setupLayers()
{
	xml_file_ << "\n<render_layers name=\"render_layers\">\n";
	writeParamMap(*params_);
	params_->clear();
	xml_file_ << "</render_layers>\n";
	return true;
}

void XmlExport::setOutfile(const char *fname)
{
	xml_name_ = std::string(fname);
}

bool XmlExport::startGeometry() { return true; }

bool XmlExport::endGeometry() { return true; }

unsigned int XmlExport::getNextFreeId()
{
	return ++next_obj_;
}


bool XmlExport::startTriMesh(const char *name, int vertices, int triangles, bool has_orco, bool has_uv, int type, int obj_pass_index)
{
	last_mat_ = nullptr;
	n_uvs_ = 0;
	xml_file_ << "\n<mesh name=\"" << name << "\" vertices=\"" << vertices << "\" faces=\"" << triangles
			  << "\" has_orco=\"" << has_orco << "\" has_uv=\"" << has_uv << "\" type=\"" << type << "\" obj_pass_index=\"" << obj_pass_index << "\">\n";
	return true;
}

bool XmlExport::startCurveMesh(const char *name, int vertices, int obj_pass_index)
{
	xml_file_ << "\n<curve name=\"" << name << "\" vertices=\"" << vertices << "\" obj_pass_index=\"" << obj_pass_index << "\">\n";
	return true;
}

bool XmlExport::endTriMesh()
{
	xml_file_ << "</mesh>\n";
	return true;
}

bool XmlExport::endCurveMesh(const Material *mat, float strand_start, float strand_end, float strand_shape)
{
	auto i = materials_.find(mat);
	if(i == materials_.end()) return false;
	xml_file_ << "\t\t\t<set_material sval=\"" << i->second << "\"/>\n"
			  << "\t\t\t<strand_start fval=\"" << strand_start << "\"/>\n"
			  << "\t\t\t<strand_end fval=\"" << strand_end << "\"/>\n"
			  << "\t\t\t<strand_shape fval=\"" << strand_shape << "\"/>\n"
			  << "</curve>\n";
	return true;
}

int  XmlExport::addVertex(double x, double y, double z)
{
	xml_file_ << "\t\t\t<p x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\"/>\n";
	return 0;
}

int  XmlExport::addVertex(double x, double y, double z, double ox, double oy, double oz)
{
	xml_file_ << "\t\t\t<p x=\"" << x << "\" y=\"" << y << "\" z=\"" << z
			  << "\" ox=\"" << ox << "\" oy=\"" << oy << "\" oz=\"" << oz << "\"/>\n";
	return 0;
}

void XmlExport::addNormal(double x, double y, double z)
{
	xml_file_ << "\t\t\t<n x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\"/>\n";
}

bool XmlExport::addTriangle(int a, int b, int c, const Material *mat)
{
	if(mat != last_mat_) //need to set current material
	{
		auto i = materials_.find(mat);
		if(i == materials_.end()) return false;
		xml_file_ << "\t\t\t<set_material sval=\"" << i->second << "\"/>\n";
		last_mat_ = mat;
	}
	xml_file_ << "\t\t\t<f a=\"" << a << "\" b=\"" << b << "\" c=\"" << c << "\"/>\n";
	return true;
}

bool XmlExport::addTriangle(int a, int b, int c, int uv_a, int uv_b, int uv_c, const Material *mat)
{
	if(mat != last_mat_) //need to set current material
	{
		auto i = materials_.find(mat);
		if(i == materials_.end()) return false;
		xml_file_ << "\t\t\t<set_material sval=\"" << i->second << "\"/>\n";
		last_mat_ = mat;
	}
	xml_file_ << "\t\t\t<f a=\"" << a << "\" b=\"" << b << "\" c=\"" << c
			  << "\" uv_a=\"" << uv_a << "\" uv_b=\"" << uv_b << "\" uv_c=\"" << uv_c << "\"/>\n";
	return true;
}

int XmlExport::addUv(float u, float v)
{
	xml_file_ << "\t\t\t<uv u=\"" << u << "\" v=\"" << v << "\"/>\n";
	return n_uvs_++;
}

bool XmlExport::smoothMesh(const char *name, double angle)
{
	xml_file_ << "<smooth name=\"" << name << "\" angle=\"" << angle << "\"/>\n";
	return true;
}

void writeMatrix__(const std::string &name, const Matrix4 &m, std::ofstream &xml_file)
{
	xml_file << "<" << name << " m00=\"" << m[0][0] << "\" m01=\"" << m[0][1] << "\" m02=\"" << m[0][2] << "\" m03=\"" << m[0][3] << "\""
			 << " m10=\"" << m[1][0] << "\" m11=\"" << m[1][1] << "\" m12=\"" << m[1][2] << "\" m13=\"" << m[1][3] << "\""
			 << " m20=\"" << m[2][0] << "\" m21=\"" << m[2][1] << "\" m22=\"" << m[2][2] << "\" m23=\"" << m[2][3] << "\""
			 << " m30=\"" << m[3][0] << "\" m31=\"" << m[3][1] << "\" m32=\"" << m[3][2] << "\" m33=\"" << m[3][3] << "\"/>";
}

inline void writeParam__(const std::string &name, const Parameter &param, std::ofstream &xml_file, ColorSpace xml_color_space, float xml_gamma)
{
	const Parameter::Type type = param.type();
	if(type == Parameter::Int)
	{
		int i = 0;
		param.getVal(i);
		xml_file << "<" << name << " ival=\"" << i << "\"/>\n";
	}
	else if(type == Parameter::Bool)
	{
		bool b = false;
		param.getVal(b);
		xml_file << "<" << name << " bval=\"" << b << "\"/>\n";
	}
	else if(type == Parameter::Float)
	{
		double f = 0.0;
		param.getVal(f);
		xml_file << "<" << name << " fval=\"" << f << "\"/>\n";
	}
	else if(type == Parameter::String)
	{
		std::string s;
		param.getVal(s);
		if(!s.empty()) xml_file << "<" << name << " sval=\"" << s << "\"/>\n";
	}
	else if(type == Parameter::Point)
	{
		Point3 p(0.f);
		param.getVal(p);
		xml_file << "<" << name << " x=\"" << p.x_ << "\" y=\"" << p.y_ << "\" z=\"" << p.z_ << "\"/>\n";
	}
	else if(type == Parameter::Color)
	{
		Rgba c(0.f);
		param.getVal(c);
		c.colorSpaceFromLinearRgb(xml_color_space, xml_gamma);    //Color values are encoded to the desired color space before saving them to the XML file
		xml_file << "<" << name << " r=\"" << c.r_ << "\" g=\"" << c.g_ << "\" b=\"" << c.b_ << "\" a=\"" << c.a_ << "\"/>\n";
	}
	else if(type == Parameter::Matrix)
	{
		Matrix4 m;
		param.getVal(m);
		writeMatrix__(name, m, xml_file);
	}
	else
	{
		std::cerr << "unknown parameter type!\n";
	}
}

bool XmlExport::addInstance(const char *base_object_name, Matrix4 obj_to_world)
{
	xml_file_ << "\n<instance base_object_name=\"" << base_object_name << "\" >\n\t";
	writeMatrix__("transform", obj_to_world, xml_file_);
	xml_file_ << "\n</instance>\n";
	return true;
}

void XmlExport::writeParamMap(const ParamMap &pmap, int indent)
{
	std::string tabs(indent, '\t');
	for(const auto &p : pmap)
	{
		xml_file_ << tabs;
		writeParam__(p.first, p.second, xml_file_, xml_color_space_, xml_gamma_);
	}
}

void XmlExport::writeParamList(int indent)
{
	std::string tabs(indent, '\t');

	auto ip = eparams_->begin();
	auto end = eparams_->end();

	for(; ip != end; ++ip)
	{
		xml_file_ << tabs << "<list_element>\n";
		writeParamMap(*ip, indent + 1);
		xml_file_ << tabs << "</list_element>\n";
	}
}

Light 		*XmlExport::createLight(const char *name)
{
	xml_file_ << "\n<light name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</light>\n";
	return nullptr;
}

Texture 		*XmlExport::createTexture(const char *name)
{
	xml_file_ << "\n<texture name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</texture>\n";
	return nullptr;
}

Material 	*XmlExport::createMaterial(const char *name)
{
	Material *matp = (Material *)++nmat_;
	materials_[matp] = std::string(name);
	xml_file_ << "\n<material name=\"" << name << "\">\n";
	writeParamMap(*params_);
	writeParamList(1);
	xml_file_ << "</material>\n";
	return matp;
}
Camera 		*XmlExport::createCamera(const char *name)
{
	xml_file_ << "\n<camera name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</camera>\n";
	return nullptr;
}
Background 	*XmlExport::createBackground(const char *name)
{
	xml_file_ << "\n<background name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</background>\n";
	return nullptr;
}
Integrator 	*XmlExport::createIntegrator(const char *name)
{
	xml_file_ << "\n<integrator name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</integrator>\n";
	return nullptr;
}

VolumeRegion 	*XmlExport::createVolumeRegion(const char *name)
{
	xml_file_ << "\n<volumeregion name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</volumeregion>\n";
	return nullptr;
}

ColorOutput *XmlExport::createOutput(const char *name)
{
	xml_file_ << "\n<output name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</output>\n";
	return nullptr;
}

RenderView *XmlExport::createRenderView(const char *name)
{
	xml_file_ << "\n<render_view name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</render_view>\n";
	return nullptr;
}

unsigned int 	XmlExport::createObject(const char *name)
{
	xml_file_ << "\n<object name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</object>\n";
	return ++next_obj_;
}

void XmlExport::render(ProgressBar *pb)
{
	xml_file_ << "\n<render>\n";
	writeParamMap(*params_);
	xml_file_ << "</render>\n";
	xml_file_ << "</scene>" << YENDL;
	xml_file_.flush();
	xml_file_.close();
}

void XmlExport::setXmlColorSpace(std::string color_space_string, float gamma_val)
{
	if(color_space_string == "sRGB") xml_color_space_ = Srgb;
	else if(color_space_string == "XYZ") xml_color_space_ = XyzD65;
	else if(color_space_string == "LinearRGB") xml_color_space_ = LinearRgb;
	else if(color_space_string == "Raw_Manual_Gamma") xml_color_space_ = RawManualGamma;
	else xml_color_space_ = Srgb;

	xml_gamma_ = gamma_val;
}

extern "C"
{
	XmlExport *getYafrayXml__()
	{
		return new XmlExport();
	}
}

END_YAFARAY
