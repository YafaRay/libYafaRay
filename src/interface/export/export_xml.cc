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

ExportXml::ExportXml(const char *fname, const ::yafaray_LoggerCallback_t logger_callback, void *callback_data, ::yafaray_DisplayConsole_t logger_display_console) : Interface(logger_callback, callback_data, logger_display_console), file_name_(std::string(fname))
{
	file_.open(file_name_.c_str());
	if(!file_.is_open())
	{
		logger_->logError("XmlExport: Couldn't open ", file_name_);
		return;
	}
	else logger_->logInfo("XmlExport: Writing scene to: ", file_name_);
	file_ << std::boolalpha;
	file_ << "<?xml version=\"1.0\"?>\n";
}

void ExportXml::createScene() noexcept
{
	file_ << "<scene>\n\n";
	file_ << "<scene_parameters>\n";
	writeParamMap(*params_);
	params_->clear();
	file_ << "</scene_parameters>\n";
}

void ExportXml::clearAll() noexcept
{
	if(logger_->isVerbose()) logger_->logVerbose("XmlExport: cleaning up...");
	if(file_.is_open())
	{
		file_.flush();
		file_.close();
	}
	params_->clear();
	nodes_params_.clear();
	cparams_ = params_.get();
	next_obj_ = 0;
}

void ExportXml::defineLayer() noexcept
{
	file_ << "\n<layer>\n";
	writeParamMap(*params_);
	params_->clear();
	file_ << "</layer>\n";
}

bool ExportXml::startGeometry() noexcept { return true; }

bool ExportXml::endGeometry() noexcept { return true; }

unsigned int ExportXml::getNextFreeId() noexcept
{
	return ++next_obj_;
}

bool ExportXml::endObject() noexcept
{
	file_ << "</object>\n";
	return true;
}

int ExportXml::addVertex(double x, double y, double z) noexcept
{
	file_ << "\t<p x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\"/>\n";
	return 0;
}

int ExportXml::addVertex(double x, double y, double z, double ox, double oy, double oz) noexcept
{
	file_ << "\t<p x=\"" << x << "\" y=\"" << y << "\" z=\"" << z
			  << "\" ox=\"" << ox << "\" oy=\"" << oy << "\" oz=\"" << oz << "\"/>\n";
	return 0;
}

void ExportXml::addVertexNormal(double x, double y, double z) noexcept
{
	file_ << "\t<n x=\"" << x << "\" y=\"" << y << "\" z=\"" << z << "\"/>\n";
}

void ExportXml::setCurrentMaterial(const char *name) noexcept
{
	const std::string name_str(name);
	if(name_str != current_material_) //need to set current material
	{
		file_ << "\t<set_material sval=\"" << name_str << "\"/>\n";
		current_material_ = name_str;
	}
}

bool ExportXml::addFace(int a, int b, int c) noexcept
{
	file_ << "\t<f a=\"" << a << "\" b=\"" << b << "\" c=\"" << c << "\"/>\n";
	return true;
}

bool ExportXml::addFace(int a, int b, int c, int uv_a, int uv_b, int uv_c) noexcept
{
	file_ << "\t<f a=\"" << a << "\" b=\"" << b << "\" c=\"" << c
			  << "\" uv_a=\"" << uv_a << "\" uv_b=\"" << uv_b << "\" uv_c=\"" << uv_c << "\"/>\n";
	return true;
}

int ExportXml::addUv(float u, float v) noexcept
{
	file_ << "\t<uv u=\"" << u << "\" v=\"" << v << "\"/>\n";
	return n_uvs_++;
}

bool ExportXml::smoothVerticesNormals(const char *name, double angle) noexcept
{
	file_ << "<smooth object_name=\"" << name << "\" angle=\"" << angle << "\"/>\n";
	return true;
}

void ExportXml::writeMatrix(const std::string &name, const Matrix4 &m, std::ofstream &file) noexcept
{
	file << "<" << name << " m00=\"" << m[0][0] << "\" m01=\"" << m[0][1] << "\" m02=\"" << m[0][2] << "\" m03=\"" << m[0][3] << "\""
		 << " m10=\"" << m[1][0] << "\" m11=\"" << m[1][1] << "\" m12=\"" << m[1][2] << "\" m13=\"" << m[1][3] << "\""
		 << " m20=\"" << m[2][0] << "\" m21=\"" << m[2][1] << "\" m22=\"" << m[2][2] << "\" m23=\"" << m[2][3] << "\""
		 << " m30=\"" << m[3][0] << "\" m31=\"" << m[3][1] << "\" m32=\"" << m[3][2] << "\" m33=\"" << m[3][3] << "\"/>";
}

void ExportXml::writeParam(const std::string &name, const Parameter &param, std::ofstream &file, ColorSpace color_space, float gamma) noexcept
{
	const Parameter::Type type = param.type();
	if(type == Parameter::Int)
	{
		int i = 0;
		param.getVal(i);
		file << "<" << name << " ival=\"" << i << "\"/>\n";
	}
	else if(type == Parameter::Bool)
	{
		bool b = false;
		param.getVal(b);
		file << "<" << name << " bval=\"" << b << "\"/>\n";
	}
	else if(type == Parameter::Float)
	{
		double f = 0.0;
		param.getVal(f);
		file << "<" << name << " fval=\"" << f << "\"/>\n";
	}
	else if(type == Parameter::String)
	{
		std::string s;
		param.getVal(s);
		if(!s.empty()) file << "<" << name << " sval=\"" << s << "\"/>\n";
	}
	else if(type == Parameter::Vector)
	{
		Point3 p{0.f, 0.f, 0.f};
		param.getVal(p);
		file << "<" << name << " x=\"" << p.x() << "\" y=\"" << p.y() << "\" z=\"" << p.z() << "\"/>\n";
	}
	else if(type == Parameter::Color)
	{
		Rgba c(0.f);
		param.getVal(c);
		c.colorSpaceFromLinearRgb(color_space, gamma);    //Color values are encoded to the desired color space before saving them to the XML file
		file << "<" << name << " r=\"" << c.r_ << "\" g=\"" << c.g_ << "\" b=\"" << c.b_ << "\" a=\"" << c.a_ << "\"/>\n";
	}
	else if(type == Parameter::Matrix)
	{
		Matrix4 m;
		param.getVal(m);
		writeMatrix(name, m, file);
	}
	else
	{
		std::cerr << "unknown parameter type!\n";
	}
}

bool ExportXml::addInstance(const char *base_object_name, const Matrix4 &obj_to_world) noexcept
{
	file_ << "\n<instance base_object_name=\"" << base_object_name << "\" >\n\t";
	writeMatrix("transform", obj_to_world, file_);
	file_ << "\n</instance>\n";
	return true;
}

void ExportXml::writeParamMap(const ParamMap &param_map, int indent) noexcept
{
	const std::string tabs(indent, '\t');
	for(const auto &[param_name, param] : param_map)
	{
		file_ << tabs;
		writeParam(param_name, param, file_, color_space_, gamma_);
	}
}

void ExportXml::writeParamList(int indent) noexcept
{
	const std::string tabs(indent, '\t');
	for(const auto &param : nodes_params_)
	{
		file_ << tabs << "<list_element>\n";
		writeParamMap(param, indent + 1);
		file_ << tabs << "</list_element>\n";
	}
}

Light *ExportXml::createLight(const char *name) noexcept
{
	file_ << "\n<light name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</light>\n";
	return nullptr;
}

Texture *ExportXml::createTexture(const char *name) noexcept
{
	file_ << "\n<texture name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</texture>\n";
	return nullptr;
}

const Material *ExportXml::createMaterial(const char *name) noexcept
{
	file_ << "\n<material name=\"" << name << "\">\n";
	writeParamMap(*params_);
	writeParamList(1);
	file_ << "</material>\n";
	return nullptr;
}
const Camera * ExportXml::createCamera(const char *name) noexcept
{
	file_ << "\n<camera name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</camera>\n";
	return nullptr;
}
const Background * ExportXml::createBackground(const char *name) noexcept
{
	file_ << "\n<background name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</background>\n";
	return nullptr;
}
Integrator *ExportXml::createIntegrator(const char *name) noexcept
{
	file_ << "\n<integrator name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</integrator>\n";
	return nullptr;
}

VolumeRegion *ExportXml::createVolumeRegion(const char *name) noexcept
{
	file_ << "\n<volumeregion name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</volumeregion>\n";
	return nullptr;
}

ImageOutput *ExportXml::createOutput(const char *name) noexcept
{
	file_ << "\n<output name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</output>\n";
	return nullptr;
}

RenderView *ExportXml::createRenderView(const char *name) noexcept
{
	file_ << "\n<render_view name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</render_view>\n";
	return nullptr;
}

Image *ExportXml::createImage(const char *name) noexcept
{
	file_ << "\n<image name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</image>\n";
	return nullptr;
}

Object *ExportXml::createObject(const char *name) noexcept
{
	n_uvs_ = 0;
	file_ << "\n<object>\n";
	file_ << "\t<object_parameters name=\"" << name << "\">\n";
	writeParamMap(*params_, 2);
	file_ << "\t</object_parameters>\n";
	++next_obj_;
	return nullptr;
}

void ExportXml::setupRender() noexcept
{
	file_ << "\n<render>\n";
	writeParamMap(*params_);
	file_ << "</render>\n";
}

void ExportXml::render(const std::shared_ptr<ProgressBar> &progress_bar) noexcept
{
	file_ << "</scene>\n";
	file_.flush();
	file_.close();
}

void ExportXml::setColorSpace(const std::string& color_space_string, float gamma_val) noexcept
{
	color_space_ = Rgb::colorSpaceFromName(color_space_string, ColorSpace::Srgb);
	gamma_ = gamma_val;
}

END_YAFARAY
