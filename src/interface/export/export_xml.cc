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
#include "geometry/matrix.h"
#include "param/param.h"
#include "render/progress_bar.h"
#include "common/version_build_info.h"

namespace yafaray {

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
	file_ << "<yafaray_xml format_version=\"" << buildinfo::getVersionMajor() << "." << buildinfo::getVersionMinor() << "." << buildinfo::getVersionPatch() << "\">\n\n";
}

void ExportXml::createScene() noexcept
{
	file_ << "<scene>\n";
	writeParamMap(*params_);
	params_->clear();
	file_ << "</scene>\n";
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

int ExportXml::addVertex(Point3f &&vertex, int time_step) noexcept
{
	file_ << "\t<p x=\"" << vertex[Axis::X] << "\" y=\"" << vertex[Axis::Y] << "\" z=\"" << vertex[Axis::Z];
	if(time_step > 0) file_ << "\" t=\"" << time_step;
	file_ << "\"/>\n";
	return 0;
}

int ExportXml::addVertex(Point3f &&vertex, Point3f &&orco, int time_step) noexcept
{
	file_ << "\t<p x=\"" << vertex[Axis::X] << "\" y=\"" << vertex[Axis::Y] << "\" z=\"" << vertex[Axis::Z]
			  << "\" ox=\"" << orco[Axis::X] << "\" oy=\"" << orco[Axis::Y] << "\" oz=\"" << orco[Axis::Z];
	if(time_step > 0) file_ << "\" t=\"" << time_step;
	file_ << "\"/>\n";
	return 0;
}

void ExportXml::addVertexNormal(Vec3f &&normal, int time_step) noexcept
{
	file_ << "\t<n x=\"" << normal[Axis::X] << "\" y=\"" << normal[Axis::Y] << "\" z=\"" << normal[Axis::Z];
	if(time_step > 0) file_ << "\" t=\"" << time_step;
	file_ << "\"/>\n";
}

void ExportXml::setCurrentMaterial(std::string &&name) noexcept
{
	if(name != current_material_) //need to set current material
	{
		file_ << "\t<set_material sval=\"" << name << "\"/>\n";
		current_material_ = std::move(name);
	}
}

bool ExportXml::addFace(std::vector<int> &&vertices, std::vector<int> &&uv_indices) noexcept
{
	file_ << "\t<f";
	const size_t num_vertices = vertices.size();
	for(size_t i = 0; i < num_vertices; ++i)
	{
		const unsigned char vertex_char = 'a' + i;
		file_ << " " << vertex_char << "=\"" << vertices[i] << "\"";
	}
	const size_t num_uv = uv_indices.size();
	for(size_t i = 0; i < num_uv; ++i)
	{
		const unsigned char vertex_char = 'a' + i;
		file_ << " uv_" << vertex_char << "=\"" << uv_indices[i] << "\"";
	}
	file_ << "/>\n";
	return true;
}

int ExportXml::addUv(Uv<float> &&uv) noexcept
{
	file_ << "\t<uv u=\"" << uv.u_ << "\" v=\"" << uv.v_ << "\"/>\n";
	return n_uvs_++;
}

bool ExportXml::smoothVerticesNormals(std::string &&name, double angle) noexcept
{
	file_ << "<smooth object_name=\"" << name << "\" angle=\"" << angle << "\"/>\n";
	return true;
}

void ExportXml::writeMatrix(const std::string &name, const Matrix4f &m, std::ofstream &file) noexcept
{
	file << "<" << name << " m00=\"" << m[0][0] << "\" m01=\"" << m[0][1] << "\" m02=\"" << m[0][2] << "\" m03=\"" << m[0][3] << "\""
		 << " m10=\"" << m[1][0] << "\" m11=\"" << m[1][1] << "\" m12=\"" << m[1][2] << "\" m13=\"" << m[1][3] << "\""
		 << " m20=\"" << m[2][0] << "\" m21=\"" << m[2][1] << "\" m22=\"" << m[2][2] << "\" m23=\"" << m[2][3] << "\""
		 << " m30=\"" << m[3][0] << "\" m31=\"" << m[3][1] << "\" m32=\"" << m[3][2] << "\" m33=\"" << m[3][3] << "\"/>";
}

void ExportXml::writeParam(const std::string &name, const Parameter &param, std::ofstream &file, ColorSpace color_space, float gamma) noexcept
{
	const Parameter::Type type = param.type();
	if(type == Parameter::Type::Int)
	{
		int i = 0;
		param.getVal(i);
		file << "<" << name << " ival=\"" << i << "\"/>\n";
	}
	else if(type == Parameter::Type::Bool)
	{
		bool b = false;
		param.getVal(b);
		file << "<" << name << " bval=\"" << b << "\"/>\n";
	}
	else if(type == Parameter::Type::Float)
	{
		double f = 0.0;
		param.getVal(f);
		file << "<" << name << " fval=\"" << f << "\"/>\n";
	}
	else if(type == Parameter::Type::String)
	{
		std::string s;
		param.getVal(s);
		file << "<" << name << " sval=\"" << s << "\"/>\n";
	}
	else if(type == Parameter::Type::Vector)
	{
		Point3f p{{0.f, 0.f, 0.f}};
		param.getVal(p);
		file << "<" << name << " x=\"" << p[Axis::X] << "\" y=\"" << p[Axis::Y] << "\" z=\"" << p[Axis::Z] << "\"/>\n";
	}
	else if(type == Parameter::Type::Color)
	{
		Rgba c(0.f);
		param.getVal(c);
		c.colorSpaceFromLinearRgb(color_space, gamma);    //Color values are encoded to the desired color space before saving them to the XML file
		file << "<" << name << " r=\"" << c.r_ << "\" g=\"" << c.g_ << "\" b=\"" << c.b_ << "\" a=\"" << c.a_ << "\"/>\n";
	}
	else if(type == Parameter::Type::Matrix)
	{
		Matrix4f m;
		param.getVal(m);
		writeMatrix(name, m, file);
	}
	else
	{
		std::cerr << "unknown parameter type!\n";
	}
}

int ExportXml::createInstance() noexcept
{
	file_ << "\n<createInstance></createInstance>\n";
	return current_instance_id_++;
}

bool ExportXml::addInstanceObject(int instance_id, std::string &&base_object_name) noexcept
{
	file_ << "\n<addInstanceObject instance_id=\"" << instance_id << "\" base_object_name=\"" << base_object_name << "\"></addInstanceObject>\n";
	return true;
}

bool ExportXml::addInstanceOfInstance(int instance_id, size_t base_instance_id) noexcept
{
	file_ << "\n<addInstanceOfInstance instance_id=\"" << instance_id << "\" base_instance_id=\"" << base_instance_id << "\"></addInstanceOfInstance>\n";
	return true;
}

bool ExportXml::addInstanceMatrix(int instance_id, Matrix4f &&obj_to_world, float time) noexcept
{
	file_ << "\n<addInstanceMatrix instance_id=\"" << instance_id << "\" time=\"" << time << "\">\n\t";
	writeMatrix("transform", obj_to_world, file_);
	file_ << "\n</addInstanceMatrix>\n";
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

std::pair<size_t, ParamError> ExportXml::createLight(std::string &&name) noexcept
{
	file_ << "\n<light name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</light>\n";
	return {};
}

std::pair<size_t, ParamError> ExportXml::createTexture(std::string &&name) noexcept
{
	file_ << "\n<texture name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</texture>\n";
	return {};
}

const Material *ExportXml::createMaterial(std::string &&name) noexcept
{
	file_ << "\n<material name=\"" << name << "\">\n";
	writeParamMap(*params_);
	writeParamList(1);
	file_ << "</material>\n";
	return nullptr;
}
std::pair<size_t, ParamError> ExportXml::createCamera(std::string &&name) noexcept
{
	file_ << "\n<camera name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</camera>\n";
	return {};
}

ParamError ExportXml::defineBackground() noexcept
{
	file_ << "\n<background>\n";
	writeParamMap(*params_);
	file_ << "</background>\n";
	return {};
}

ParamError ExportXml::defineSurfaceIntegrator() noexcept
{
	file_ << "\n<surface_integrator>\n";
	writeParamMap(*params_);
	file_ << "</surface_integrator>\n";
	return {};
}

ParamError ExportXml::defineVolumeIntegrator() noexcept
{
	file_ << "\n<volume_integrator>\n";
	writeParamMap(*params_);
	file_ << "</volume_integrator>\n";
	return {};
}

std::pair<size_t, ParamError> ExportXml::createVolumeRegion(std::string &&name) noexcept
{
	file_ << "\n<volumeregion name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</volumeregion>\n";
	return {};
}

ImageOutput *ExportXml::createOutput(std::string &&name) noexcept
{
	file_ << "\n<output name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</output>\n";
	return nullptr;
}

std::pair<size_t, ParamError> ExportXml::createRenderView(std::string &&name) noexcept
{
	std::string render_view_name{name};
	if(render_view_name.empty()) render_view_name = "render_view";
	file_ << "\n<render_view name=\"" << render_view_name << "\">\n";
	writeParamMap(*params_);
	file_ << "</render_view>\n";
	return {};
}

std::pair<Image *, ParamError> ExportXml::createImage(std::string &&name) noexcept
{
	file_ << "\n<image name=\"" << name << "\">\n";
	writeParamMap(*params_);
	file_ << "</image>\n";
	return {};
}

std::pair<size_t, ParamError> ExportXml::createObject(std::string &&name) noexcept
{
	n_uvs_ = 0;
	file_ << "\n<object>\n";
	file_ << "\t<object_parameters name=\"" << name << "\">\n";
	writeParamMap(*params_, 2);
	file_ << "\t</object_parameters>\n";
	++next_obj_;
	return {next_obj_ - 1, ParamError{}};
}

void ExportXml::setupRender() noexcept
{
	file_ << "\n<render>\n";
	writeParamMap(*params_);
	file_ << "</render>\n";
}

void ExportXml::render(std::unique_ptr<ProgressBar> progress_bar) noexcept
{
	file_ << "</yafaray_xml>\n";
	file_.flush();
	file_.close();
}

void ExportXml::setColorSpace(const std::string& color_space_string, float gamma_val) noexcept
{
	color_space_.initFromString(color_space_string);
	gamma_ = gamma_val;
}

} //namespace yafaray
