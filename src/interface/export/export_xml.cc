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

#include "interface/export/export_xml.h"
#include "common/logger.h"
#include "scene/scene.h"
#include "geometry/matrix4.h"
#include "common/param.h"

BEGIN_YAFARAY

XmlExport::XmlExport(const char *fname, const ::yafaray_LoggerCallback_t logger_callback, void *callback_data, ::yafaray_DisplayConsole_t logger_display_console) : Interface(logger_callback, callback_data, logger_display_console), xml_name_(std::string(fname))
{
	xml_file_.open(xml_name_.c_str());
	if(!xml_file_.is_open())
	{
		logger_->logError("XmlExport: Couldn't open ", xml_name_);
		return;
	}
	else logger_->logInfo("XmlExport: Writing scene to: ", xml_name_);
	xml_file_ << std::boolalpha;
	xml_file_ << "<?xml version=\"1.0\"?>\n";
}

void XmlExport::createScene() noexcept
{
	xml_file_ << "<scene>\n\n";
	xml_file_ << "<scene_parameters>\n";
	writeParamMap(*params_);
	params_->clear();
	xml_file_ << "</scene_parameters>\n";
}

void XmlExport::clearAll() noexcept
{
	if(logger_->isVerbose()) logger_->logVerbose("XmlExport: cleaning up...");
	if(xml_file_.is_open())
	{
		xml_file_.flush();
		xml_file_.close();
	}
	params_->clear();
	eparams_->clear();
	cparams_ = params_.get();
	next_obj_ = 0;
}

void XmlExport::defineLayer() noexcept
{
	xml_file_ << "\n<layer>\n";
	writeParamMap(*params_);
	params_->clear();
	xml_file_ << "</layer>\n";
}

bool XmlExport::startGeometry() noexcept { return true; }

bool XmlExport::endGeometry() noexcept { return true; }

unsigned int XmlExport::getNextFreeId() noexcept
{
	return ++next_obj_;
}

bool XmlExport::endObject() noexcept
{
	xml_file_ << "</object>\n";
	return true;
}

int XmlExport::addVertex(double x, double y, double z) noexcept
{
	xml_file_ << "\t<p x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\"/>\n";
	return 0;
}

int XmlExport::addVertex(double x, double y, double z, double ox, double oy, double oz) noexcept
{
	xml_file_ << "\t<p x=\"" << x << "\" y=\"" << y << "\" z=\"" << z
			  << "\" ox=\"" << ox << "\" oy=\"" << oy << "\" oz=\"" << oz << "\"/>\n";
	return 0;
}

void XmlExport::addNormal(double x, double y, double z) noexcept
{
	xml_file_ << "\t<n x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\"/>\n";
}

void XmlExport::setCurrentMaterial(const char *name) noexcept
{
	const std::string name_str(name);
	if(name_str != current_material_) //need to set current material
	{
		xml_file_ << "\t<set_material sval=\"" << name_str << "\"/>\n";
		current_material_ = name_str;
	}
}

bool XmlExport::addFace(int a, int b, int c) noexcept
{
	xml_file_ << "\t<f a=\"" << a << "\" b=\"" << b << "\" c=\"" << c << "\"/>\n";
	return true;
}

bool XmlExport::addFace(int a, int b, int c, int uv_a, int uv_b, int uv_c) noexcept
{
	xml_file_ << "\t<f a=\"" << a << "\" b=\"" << b << "\" c=\"" << c
			  << "\" uv_a=\"" << uv_a << "\" uv_b=\"" << uv_b << "\" uv_c=\"" << uv_c << "\"/>\n";
	return true;
}

int XmlExport::addUv(float u, float v) noexcept
{
	xml_file_ << "\t<uv u=\"" << u << "\" v=\"" << v << "\"/>\n";
	return n_uvs_++;
}

bool XmlExport::smoothMesh(const char *name, double angle) noexcept
{
	xml_file_ << "<smooth object_name=\"" << name << "\" angle=\"" << angle << "\"/>\n";
	return true;
}

void XmlExport::writeMatrix(const std::string &name, const Matrix4 &m, std::ofstream &xml_file) noexcept
{
	xml_file << "<" << name << " m00=\"" << m[0][0] << "\" m01=\"" << m[0][1] << "\" m02=\"" << m[0][2] << "\" m03=\"" << m[0][3] << "\""
			 << " m10=\"" << m[1][0] << "\" m11=\"" << m[1][1] << "\" m12=\"" << m[1][2] << "\" m13=\"" << m[1][3] << "\""
			 << " m20=\"" << m[2][0] << "\" m21=\"" << m[2][1] << "\" m22=\"" << m[2][2] << "\" m23=\"" << m[2][3] << "\""
			 << " m30=\"" << m[3][0] << "\" m31=\"" << m[3][1] << "\" m32=\"" << m[3][2] << "\" m33=\"" << m[3][3] << "\"/>";
}

void XmlExport::writeParam(const std::string &name, const Parameter &param, std::ofstream &xml_file, ColorSpace xml_color_space, float xml_gamma) noexcept
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
	else if(type == Parameter::Vector)
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
		writeMatrix(name, m, xml_file);
	}
	else
	{
		std::cerr << "unknown parameter type!\n";
	}
}

bool XmlExport::addInstance(const char *base_object_name, const Matrix4 &obj_to_world) noexcept
{
	xml_file_ << "\n<instance base_object_name=\"" << base_object_name << "\" >\n\t";
	writeMatrix("transform", obj_to_world, xml_file_);
	xml_file_ << "\n</instance>\n";
	return true;
}

void XmlExport::writeParamMap(const ParamMap &param_map, int indent) noexcept
{
	const std::string tabs(indent, '\t');
	for(const auto &param : param_map)
	{
		xml_file_ << tabs;
		writeParam(param.first, param.second, xml_file_, xml_color_space_, xml_gamma_);
	}
}

void XmlExport::writeParamList(int indent) noexcept
{
	const std::string tabs(indent, '\t');
	for(const auto &param : *eparams_)
	{
		xml_file_ << tabs << "<list_element>\n";
		writeParamMap(param, indent + 1);
		xml_file_ << tabs << "</list_element>\n";
	}
}

Light *XmlExport::createLight(const char *name) noexcept
{
	xml_file_ << "\n<light name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</light>\n";
	return nullptr;
}

Texture *XmlExport::createTexture(const char *name) noexcept
{
	xml_file_ << "\n<texture name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</texture>\n";
	return nullptr;
}

Material *XmlExport::createMaterial(const char *name) noexcept
{
	xml_file_ << "\n<material name=\"" << name << "\">\n";
	writeParamMap(*params_);
	writeParamList(1);
	xml_file_ << "</material>\n";
	return nullptr;
}
Camera *XmlExport::createCamera(const char *name) noexcept
{
	xml_file_ << "\n<camera name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</camera>\n";
	return nullptr;
}
Background *XmlExport::createBackground(const char *name) noexcept
{
	xml_file_ << "\n<background name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</background>\n";
	return nullptr;
}
Integrator *XmlExport::createIntegrator(const char *name) noexcept
{
	xml_file_ << "\n<integrator name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</integrator>\n";
	return nullptr;
}

VolumeRegion *XmlExport::createVolumeRegion(const char *name) noexcept
{
	xml_file_ << "\n<volumeregion name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</volumeregion>\n";
	return nullptr;
}

ImageOutput *XmlExport::createOutput(const char *name) noexcept
{
	xml_file_ << "\n<output name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</output>\n";
	return nullptr;
}

RenderView *XmlExport::createRenderView(const char *name) noexcept
{
	xml_file_ << "\n<render_view name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</render_view>\n";
	return nullptr;
}

Image *XmlExport::createImage(const char *name) noexcept
{
	xml_file_ << "\n<image name=\"" << name << "\">\n";
	writeParamMap(*params_);
	xml_file_ << "</image>\n";
	return nullptr;
}

Object *XmlExport::createObject(const char *name) noexcept
{
	n_uvs_ = 0;
	xml_file_ << "\n<object>\n";
	xml_file_ << "\t<object_parameters name=\"" << name << "\">\n";
	writeParamMap(*params_, 2);
	xml_file_ << "\t</object_parameters>\n";
	++next_obj_;
	return nullptr;
}

void XmlExport::setupRender() noexcept
{
	xml_file_ << "\n<render>\n";
	writeParamMap(*params_);
	xml_file_ << "</render>\n";
}

void XmlExport::render(std::shared_ptr<ProgressBar> progress_bar) noexcept
{
	xml_file_ << "</scene>\n";
	xml_file_.flush();
	xml_file_.close();
}

void XmlExport::setXmlColorSpace(std::string color_space_string, float gamma_val) noexcept
{
	xml_color_space_ = Rgb::colorSpaceFromName(color_space_string, ColorSpace::Srgb);
	xml_gamma_ = gamma_val;
}

END_YAFARAY
